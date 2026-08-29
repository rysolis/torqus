// "Formula" tests for tfhe/utility/analysis/noise.hpp: each
// NoisePolicy<Op>::compute() is called directly, with a Tracker fed
// hand-picked bounds instead of ones observed from a real
// encrypt/exec/decrypt run. Each test computes its expected value from the
// same named coefficients (B, l, N, n, t, K, eps, ...) noise.hpp's own NOTE
// comments describe -- via a plain double expression written independently
// of noise.hpp's ExactBound machinery -- rather than a pre-collapsed
// decimal, so the test still fails if a coefficient in noise.hpp drifts
// from that spec. Every input is chosen so the whole computation stays an
// exact binary fraction, so ExpectedBound's one-ulp nudge (see below) is
// the only rounding in play and EXPECT_DOUBLE_EQ is exact.
//
// This only confirms compute() implements its intended formula correctly
// (right coefficients, right operand, max vs sum, ...). It says nothing
// about whether that formula actually bounds a real ciphertext's error --
// that is a separate, statistical question left to each Op's own runtime
// test (e.g. key_switch_test.cpp's EXPECT_LE(norm, tracker->get(...))).

#include "tfhe/utility/analysis/noise.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <limits>

#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/utility/analysis/tracker/noise_tracker.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

namespace noise_test {

// NoisePolicy<...>::compute() takes its Tracker as an explicit argument for
// most ops, but BlindRotate/GateBootstrap/KeySwitch instead read key noise
// straight off the process-global get_key_noise_tracker_if() singleton (see
// noise.hpp). This substitutes that singleton with a local NoiseTracker for
// one test and puts back whatever was installed before it.
class ScopedKeyNoiseTracker {
 public:
  ScopedKeyNoiseTracker() : saved_(get_key_noise_tracker_if()) {
    set_key_noise_tracker_if(&tracker_);
  }
  ~ScopedKeyNoiseTracker() { set_key_noise_tracker_if(saved_); }

  template <typename Key>
  void set(const Key& key, double bound) {
    tracker_.update(key, bound);
  }

 private:
  NoiseTracker tracker_;
  NoiseTrackerInterface* saved_;
};

// to_round_up() (exact_bound.hpp) always nudges its result one ulp past the
// exact rational value, even when that value is already an exactly
// representable double -- so the expected value for an exact-binary-fraction
// hand computation is one nextafter() past it, not the value itself.
double ExpectedBound(double exact_value) {
  return std::nextafter(exact_value, std::numeric_limits<double>::infinity());
}

// eps() as every NoisePolicy specialization defines it:
// 2^-(digit_bits*rounds+1), where digit_bits is the decomposition base's bit
// width. Reimplemented here with plain double arithmetic, independently of
// noise.hpp's ExactBound-based eps(), so a mistake in that eps() (wrong
// exponent, wrong digit-bit count) still shows up as a mismatch.
double Eps(uint32_t digit_bits, uint32_t rounds) {
  return std::ldexp(1.0, -static_cast<int>(digit_bits * rounds + 1));
}

}  // namespace noise_test

using noise_test::Eps;
using noise_test::ExpectedBound;
using noise_test::ScopedKeyNoiseTracker;

TEST(NoiseFormulaTest, AddSumsBothInputBounds) {
  using Torus = ModTorus<16>;
  using Rlwe = trlwe_core_params<Torus, 4>;

  const double e_lhs = 0.5;
  const double e_rhs = 0.25;

  TRLWE<Torus, 4> lhs, rhs;
  NoiseTracker tracker;
  tracker.update(lhs, e_lhs);
  tracker.update(rhs, e_rhs);

  double bound =
      NoisePolicy<tfhe::leveled::Add<Rlwe>>::compute(&tracker, lhs, rhs);
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(e_lhs + e_rhs));
}

TEST(NoiseFormulaTest, SubSumsBothInputBounds) {
  // |e1 - e2| <= |e1| + |e2|: Sub's bound is a sum too, not a difference.
  using Torus = ModTorus<16>;
  using Rlwe = trlwe_core_params<Torus, 4>;

  const double e_lhs = 0.5;
  const double e_rhs = 0.25;

  TRLWE<Torus, 4> lhs, rhs;
  NoiseTracker tracker;
  tracker.update(lhs, e_lhs);
  tracker.update(rhs, e_rhs);

  double bound =
      NoisePolicy<tfhe::leveled::Sub<Rlwe>>::compute(&tracker, lhs, rhs);
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(e_lhs + e_rhs));
}

TEST(NoiseFormulaTest, ExternalProductAppliesDecompositionCoefficients) {
  // bound = 2*l*N*(B/2)*e_gsw + (1+N)*eps + e_rhs
  constexpr uint32_t N = 4, B = 4, l = 2;
  constexpr uint32_t Bbit = std::bit_width(B - 1);
  using Torus = ModTorus<16>;
  using Rlwe = trlwe_core_params<Torus, N>;
  using Decomp = dcp_params<B, l>;

  const double e_gsw = 0.5;
  const double e_rhs = 0.25;

  TRGSW<Torus, N, l> gsw;
  TRLWE<Torus, N> rhs;
  NoiseTracker tracker;
  tracker.update(gsw, e_gsw);
  tracker.update(rhs, e_rhs);

  double bound =
      NoisePolicy<tfhe::bootstrap::ExternalProduct<Rlwe, Decomp>>::compute(
          &tracker, gsw, rhs);

  double expected = (2.0 * l * N * static_cast<double>(B) / 2.0) * e_gsw +
                    (1 + N) * Eps(Bbit, l) + e_rhs;
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(expected));
}

TEST(NoiseFormulaTest, CMuxUsesMaxOfInputsWhenRhsLarger) {
  // bound = 2*l*N*(B/2)*e_selector + (1+N)*eps + max(e_lhs, e_rhs)
  constexpr uint32_t N = 4, B = 4, l = 2;
  constexpr uint32_t Bbit = std::bit_width(B - 1);
  using Torus = ModTorus<16>;
  using Rlwe = trlwe_core_params<Torus, N>;
  using Decomp = dcp_params<B, l>;

  const double e_selector = 0.5;
  const double e_lhs = 0.125;
  const double e_rhs = 0.375;  // larger than e_lhs

  TRGSW<Torus, N, l> selector;
  TRLWE<Torus, N> lhs, rhs;
  NoiseTracker tracker;
  tracker.update(selector, e_selector);
  tracker.update(lhs, e_lhs);
  tracker.update(rhs, e_rhs);

  double bound = NoisePolicy<tfhe::bootstrap::CMux<Rlwe, Decomp>>::compute(
      &tracker, selector, lhs, rhs);

  double expected = (2.0 * l * N * static_cast<double>(B) / 2.0) * e_selector +
                    (1 + N) * Eps(Bbit, l) + std::max(e_lhs, e_rhs);
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(expected));
}

TEST(NoiseFormulaTest, CMuxUsesMaxOfInputsWhenLhsLarger) {
  // Same as above with lhs/rhs bounds swapped, so a max() bug that always
  // picks one fixed argument (rather than the actual larger one) shows up
  // as a wrong result in at least one of this pair of tests.
  constexpr uint32_t N = 4, B = 4, l = 2;
  constexpr uint32_t Bbit = std::bit_width(B - 1);
  using Torus = ModTorus<16>;
  using Rlwe = trlwe_core_params<Torus, N>;
  using Decomp = dcp_params<B, l>;

  const double e_selector = 0.5;
  const double e_lhs = 0.375;  // larger than e_rhs
  const double e_rhs = 0.125;

  TRGSW<Torus, N, l> selector;
  TRLWE<Torus, N> lhs, rhs;
  NoiseTracker tracker;
  tracker.update(selector, e_selector);
  tracker.update(lhs, e_lhs);
  tracker.update(rhs, e_rhs);

  double bound = NoisePolicy<tfhe::bootstrap::CMux<Rlwe, Decomp>>::compute(
      &tracker, selector, lhs, rhs);

  double expected = (2.0 * l * N * static_cast<double>(B) / 2.0) * e_selector +
                    (1 + N) * Eps(Bbit, l) + std::max(e_lhs, e_rhs);
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(expected));
}

namespace {

// Shared by BlindRotate and GateBootstrap below: bound = n*2*l*N*(B/2)*e_bk +
// n*(1+N)*eps. GateBootstrap's own noise contribution is exactly
// BlindRotate's, since its Add(offset, SampleExtract(rot)) step is otherwise
// noise-free.
double ExpectedBlindRotateBound(uint32_t n, uint32_t N, uint32_t B, uint32_t l,
                                double e_bk) {
  uint32_t Bbit = static_cast<uint32_t>(std::bit_width(B - 1));
  return (static_cast<double>(n) * 2.0 * l * N * static_cast<double>(B) / 2.0) *
             e_bk +
         n * (1 + N) * Eps(Bbit, l);
}

}  // namespace

TEST(NoiseFormulaTest, BlindRotateIgnoresTvAndScalesKeyNoiseByN) {
  // (the accumulation window's own tv/amount noise is assumed to be 0, per
  // noise.hpp's "assume that ||Err(tv)|| = 0" comment, so their contents
  // must not affect the result at all.)
  constexpr uint32_t n = 3, N = 4, B = 4, l = 2;
  using Torus = ModTorus<16>;
  using Lwe = tlwe_core_params<Torus, n>;
  using Rlwe = trlwe_core_params<Torus, N>;
  using Decomp = dcp_params<B, l>;

  const double e_bk = 0.5;

  TRLWE<Torus, N> tv;
  Vector<ModInt<2 * N>, n + 1> amount;
  BootstrapKey<Torus, N, l, n> bk;

  ScopedKeyNoiseTracker key_tracker;
  key_tracker.set(bk, e_bk);

  // compute()'s Tracker argument is unnamed in noise.hpp's
  // NoisePolicy<BlindRotate> (key noise comes from get_key_noise_tracker_if()
  // instead), so it's never dereferenced -- a typed nullptr only exists to
  // let Tracker be deduced.
  double bound =
      NoisePolicy<tfhe::bootstrap::BlindRotate<Lwe, Rlwe, Decomp>>::compute(
          static_cast<NoiseTrackerInterface*>(nullptr), tv, amount, bk);
  EXPECT_DOUBLE_EQ(bound,
                   ExpectedBound(ExpectedBlindRotateBound(n, N, B, l, e_bk)));
}

TEST(NoiseFormulaTest, GateBootstrapMatchesBlindRotateFormula) {
  constexpr uint32_t n = 3, N = 4, B = 4, l = 2;
  using Torus = ModTorus<16>;
  using Lwe = tlwe_core_params<Torus, n>;
  using Rlwe = trlwe_core_params<Torus, N>;
  using Decomp = dcp_params<B, l>;

  const double e_bk = 0.5;

  Torus mu{};
  TRLWE<Torus, N> tv;
  TLWE<Torus, n> tlwe;
  BootstrapKey<Torus, N, l, n> bk;

  ScopedKeyNoiseTracker key_tracker;
  key_tracker.set(bk, e_bk);

  // Same reasoning as BlindRotate's compute() call above: this argument is
  // never dereferenced, only used to deduce Tracker.
  double bound =
      NoisePolicy<tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>>::compute(
          static_cast<NoiseTrackerInterface*>(nullptr), mu, tv, tlwe, bk);
  EXPECT_DOUBLE_EQ(bound,
                   ExpectedBound(ExpectedBlindRotateBound(n, N, B, l, e_bk)));
}

TEST(NoiseFormulaTest, KeySwitchCombinesSrcBoundAndKeyNoise) {
  // bound = e_src + N*t*(K-1)*e_ksk + N*eps, N = Src::n here (not Dst::n)
  constexpr uint32_t srcN = 5, dstN = 3, K = 4, t = 2;
  constexpr uint32_t Kbit = std::bit_width(K - 1);
  using Torus = ModTorus<16>;
  using Src = tlwe_core_params<Torus, srcN>;
  using Dst = tlwe_core_params<Torus, dstN>;
  using Kst = kst_params<K, t>;

  const double e_src = 0.5;
  const double e_ksk = 0.25;

  TLWE<Torus, srcN> src;
  KeySwitchKey<Torus, dstN, t, srcN> ksk;

  NoiseTracker tracker;
  tracker.update(src, e_src);

  ScopedKeyNoiseTracker key_tracker;
  key_tracker.set(ksk[0][0], e_ksk);

  double bound = NoisePolicy<tfhe::leveled::KeySwitch<Src, Dst, Kst>>::compute(
      &tracker, src, ksk);

  double expected = e_src + (static_cast<double>(srcN) * t * (K - 1)) * e_ksk +
                    srcN * Eps(Kbit, t);
  EXPECT_DOUBLE_EQ(bound, ExpectedBound(expected));
}
