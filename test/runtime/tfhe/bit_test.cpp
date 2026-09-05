#include <gtest/gtest.h>

#include "tfhe/bit.hpp"
#include "tfhe/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace bit_test {
// Real noise enabled -- same params as tfhe/gate/hom_and_test.cpp's own
// Context, plus binary_expansion_test.cpp's Context2 kst_params (base
// kept small since KeySwitch noise grows with the base; see that file's
// own comment).
using Lwe = lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>;
using Rlwe =
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>;
using Decomp = dcp_params<16, 7>;
using Kst = kst_params<2, 11>;
}  // namespace bit_test

class BitTest : public ::testing::Test {
 protected:
  using Lwe = bit_test::Lwe;
  using Rlwe = bit_test::Rlwe;
  using Decomp = bit_test::Decomp;
  using Kst = bit_test::Kst;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t t = Kst::t;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>> rlwe_runtime_;

  BootstrapKey<rTorus, N, Decomp::l, n> BK_;
  KeySwitchKey<Torus, n, t, N> KSK_;

  void SetUp() override {
    lwe_runtime_ = Runtime<Lwe>(eng_);
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>>(eng_);

    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.holder().get());
    KSK_ = lwe_runtime_
               .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                   rlwe_runtime_.holder().get());
  }
};

// A freshly-encrypted Bit starts out Lwe-shaped.
TEST_F(BitTest, FreshlyEncryptedBitIsReady) {
  Bit<Lwe, Rlwe> a(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));

  EXPECT_TRUE(a.is_ready());
}

// hom_and's result is Rlwe-shaped -- exactly what every tfhe/gate/Hom*
// returns -- until something materializes it back down.
TEST_F(BitTest, GateResultIsNotReady) {
  Bit<Lwe, Rlwe> a(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> b(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));

  Bit<Lwe, Rlwe> result = tfhe::bit::And<Kst, Decomp>(a, b, BK_, KSK_);

  EXPECT_FALSE(result.is_ready());
  bool decoded = Dial<4, rTorus>::decode(
      rlwe_runtime_.decrypt(result.pending_ciphertext()));
  EXPECT_TRUE(decoded);
}

// Calling materialize() explicitly is a valid way to normalize a Bit --
// the caller's own way to control when that step happens rather than
// leaving it to the next gate call.
TEST_F(BitTest, ExplicitMaterializeMakesItReady) {
  Bit<Lwe, Rlwe> a(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> b(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));

  Bit<Lwe, Rlwe> result = tfhe::bit::And<Kst, Decomp>(a, b, BK_, KSK_);
  result.materialize<Kst>(KSK_);

  EXPECT_TRUE(result.is_ready());
  bool decoded =
      Dial<4, Torus>::decode(lwe_runtime_.decrypt(result.ready_ciphertext()));
  EXPECT_TRUE(decoded);

  // A second call is a harmless no-op.
  result.materialize<Kst>(KSK_);
  EXPECT_TRUE(result.is_ready());
}

TEST_F(BitTest, HomOrHomAndNotHomXorAllWork) {
  Bit<Lwe, Rlwe> t(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> f(lwe_runtime_.encrypt(Dial<4, Torus>::at(false)));

  Bit<Lwe, Rlwe> or_result = tfhe::bit::Or<Kst, Decomp>(t, f, BK_, KSK_);
  Bit<Lwe, Rlwe> and_not_result =
      tfhe::bit::AndNot<Kst, Decomp>(t, f, BK_, KSK_);
  Bit<Lwe, Rlwe> xor_result = tfhe::bit::Xor<Kst, Decomp>(t, f, BK_, KSK_);

  bool or_decoded = Dial<4, rTorus>::decode(
      rlwe_runtime_.decrypt(or_result.pending_ciphertext()));
  bool and_not_decoded = Dial<4, rTorus>::decode(
      rlwe_runtime_.decrypt(and_not_result.pending_ciphertext()));
  bool xor_decoded = Dial<4, rTorus>::decode(
      rlwe_runtime_.decrypt(xor_result.pending_ciphertext()));

  EXPECT_TRUE(or_decoded);
  EXPECT_TRUE(and_not_decoded);
  EXPECT_TRUE(xor_decoded);
}

// The whole point of Bit: chaining gates needs no explicit KeySwitch call
// from the caller -- hom_and here materializes its not-yet-ready operand
// (the output of the first hom_and) automatically.
TEST_F(BitTest, ChainingTwoGatesRequiresNoExplicitMaterialize) {
  Bit<Lwe, Rlwe> a(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> b(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> c(lwe_runtime_.encrypt(Dial<4, Torus>::at(false)));

  // (a AND b) AND c == false
  Bit<Lwe, Rlwe> ab = tfhe::bit::And<Kst, Decomp>(a, b, BK_, KSK_);
  ASSERT_FALSE(ab.is_ready());

  Bit<Lwe, Rlwe> abc = tfhe::bit::And<Kst, Decomp>(ab, c, BK_, KSK_);

  bool decoded =
      Dial<4, rTorus>::decode(rlwe_runtime_.decrypt(abc.pending_ciphertext()));
  EXPECT_FALSE(decoded);
}

// hom_and takes both operands by const& and never mutates them --
// materialize()'s effect inside detail::combine() stays local to that
// call.
TEST_F(BitTest, GateCallsDoNotMutateTheirOperands) {
  Bit<Lwe, Rlwe> a0(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> b0(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> a = tfhe::bit::And<Kst, Decomp>(a0, b0, BK_, KSK_);
  ASSERT_FALSE(a.is_ready());

  Bit<Lwe, Rlwe> b(lwe_runtime_.encrypt(Dial<4, Torus>::at(true)));
  Bit<Lwe, Rlwe> unused = tfhe::bit::And<Kst, Decomp>(a, b, BK_, KSK_);
  (void)unused;

  EXPECT_FALSE(a.is_ready());
}
