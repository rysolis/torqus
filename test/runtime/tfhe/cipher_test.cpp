#include <gtest/gtest.h>

#include "tfhe/cipher.hpp"
#include "tfhe/circuit/reslot.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/gate/hom_or.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace cipher_test {
// Real noise enabled -- same params as tfhe/gate/hom_and_test.cpp's own
// Context, plus binary_expansion_test.cpp's Context2 kst_params (base
// kept small since KeySwitch noise grows with the base; see that file's
// own comment).
using Lwe = lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>;
using Rlwe =
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>;
using Decomp = dcp_params<16, 7>;
using Kst = kst_params<2, 11>;
}  // namespace cipher_test

class CipherTest : public ::testing::Test {
 protected:
  using Lwe = cipher_test::Lwe;
  using Rlwe = cipher_test::Rlwe;
  using Decomp = cipher_test::Decomp;
  using Kst = cipher_test::Kst;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> bk_;
  KeySwitchKey<Torus, n, t, N> ksk_;
  Relay<Lwe, Rlwe, Kst> relay_;

  void SetUp() override {
    lwe_runtime_ = Runtime<Lwe>(eng_);
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>>(eng_);

    bk_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.secret());
    ksk_ = lwe_runtime_
               .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                   rlwe_runtime_.secret());
    relay_ = Relay<Lwe, Rlwe, Kst>(ksk_);
  }

  // Thin Cipher-in/Cipher-out adapters over tfhe::gate::Hom* -- gate::HomAnd
  // et al. already do the actual work as static exec_impl calls, so these
  // just extract .ready() and rewrap the (Rlwe-shaped) result as Cipher.
  Cipher<Lwe, Rlwe> and_(const Cipher<Lwe, Rlwe>& lhs,
                         const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(tfhe::gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }
  Cipher<Lwe, Rlwe> or_(const Cipher<Lwe, Rlwe>& lhs,
                        const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(tfhe::gate::HomOr<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }
  Cipher<Lwe, Rlwe> and_not_(const Cipher<Lwe, Rlwe>& lhs,
                             const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(lhs.ready(),
                                                            rhs.ready(), bk_));
  }
  Cipher<Lwe, Rlwe> xor_(const Cipher<Lwe, Rlwe>& lhs,
                         const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(tfhe::gate::HomXor<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }

  // A caller needing several (InResolution, OutResolution) pairs -- like
  // these tests -- builds one tfhe::circuit::Reslot per pair, same as
  // holding several parameter sets for the Hom* adapters above.
  template <uint32_t InResolution, uint32_t OutResolution>
  Cipher<Lwe, Rlwe> reslot(const Cipher<Lwe, Rlwe>& bit) const {
    return tfhe::circuit::Reslot<InResolution, OutResolution, Lwe, Rlwe, Decomp,
                                 Kst>(bk_, ksk_)
        .exec(bit);
  }
};

// A freshly-encrypted Cipher starts out Lwe-shaped.
TEST_F(CipherTest, FreshlyEncryptedBitIsReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> a_ct = boundary.lift(true);

  EXPECT_TRUE(a_ct.is_ready());
}

// hom_and's result is Rlwe-shaped -- exactly what every tfhe/gate/Hom*
// returns -- until something materializes it back down.
TEST_F(CipherTest, GateResultIsNotReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> a_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> b_ct = boundary.lift(true);

  Cipher<Lwe, Rlwe> result_ct = and_(a_ct, b_ct);

  EXPECT_FALSE(result_ct.is_ready());
  EXPECT_TRUE(drop(boundary, result_ct));
}

// Relay::materialize() is how a caller normalizes a Cipher back to
// Lwe-shaped -- HomAnd/HomOr/HomAndNot/HomXor never do this on their own.
TEST_F(CipherTest, ExplicitMaterializeMakesItReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> a_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> b_ct = boundary.lift(true);

  Cipher<Lwe, Rlwe> result_ct = and_(a_ct, b_ct);
  relay_.materialize(result_ct);

  EXPECT_TRUE(result_ct.is_ready());
  // drop(boundary, bit) handles either shape -- result_ct is now
  // Lwe-shaped, but it still decodes the same way (see boundary.hpp).
  EXPECT_TRUE(drop(boundary, result_ct));

  // A second call is a harmless no-op.
  relay_.materialize(result_ct);
  EXPECT_TRUE(result_ct.is_ready());
}

// Reslot<N, N> bootstraps a Cipher back to fresh noise without changing
// its value -- the pure-refresh case of Reslot (see
// tfhe/operation/bootstrap/reslot.hpp).
TEST_F(CipherTest, ReslotWithSameResolutionPreservesValue) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> t_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> f_ct = boundary.lift(false);

  Cipher<Lwe, Rlwe> t_refreshed = reslot<4, 4>(t_ct);
  Cipher<Lwe, Rlwe> f_refreshed = reslot<4, 4>(f_ct);

  EXPECT_FALSE(t_refreshed.is_ready());
  EXPECT_TRUE(drop(boundary, t_refreshed));
  EXPECT_FALSE(drop(boundary, f_refreshed));
}

// A Cipher lifted at Dial<2, Torus> (0 or 1/2) moved into Dial<4, Torus> (0 or
// 1/4) -- the step HomAnd/HomOr/HomAndNot/HomXor expect.
TEST_F(CipherTest, ReslotMovesValueToNewResolution) {
  Boundary<2, Lwe, Rlwe, Decomp> in_boundary(lwe_runtime_, rlwe_runtime_);
  Boundary<4, Lwe, Rlwe, Decomp> out_boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> t_ct = in_boundary.lift(true);
  Cipher<Lwe, Rlwe> f_ct = in_boundary.lift(false);

  Cipher<Lwe, Rlwe> t_resloted = reslot<2, 4>(t_ct);
  Cipher<Lwe, Rlwe> f_resloted = reslot<2, 4>(f_ct);

  EXPECT_TRUE(drop(out_boundary, t_resloted));
  EXPECT_FALSE(drop(out_boundary, f_resloted));
}

// OutResolution has no power-of-two constraint (only InResolution does, so
// scaling up to exactly 1/2 by repeated self-addition lands on an integer
// number of doublings) -- Dial<4, Torus> (0 or 1/4) moved into Dial<100,
// Torus> (0 or 1/100) exercises that.
TEST_F(CipherTest, ReslotOutResolutionNeedNotBeAPowerOfTwo) {
  Boundary<4, Lwe, Rlwe, Decomp> in_boundary(lwe_runtime_, rlwe_runtime_);
  Boundary<100, Lwe, Rlwe, Decomp> out_boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> t_ct = in_boundary.lift(true);
  Cipher<Lwe, Rlwe> f_ct = in_boundary.lift(false);

  Cipher<Lwe, Rlwe> t_resloted = reslot<4, 100>(t_ct);
  Cipher<Lwe, Rlwe> f_resloted = reslot<4, 100>(f_ct);

  // Dial<100,...>'s indices 0/1 are still 0 and 1/100 -- same true/false
  // reading as Dial<4,...>'s 0/1, just on a finer grid.
  EXPECT_EQ(drop(out_boundary, t_resloted), 1u);
  EXPECT_EQ(drop(out_boundary, f_resloted), 0u);
}

TEST_F(CipherTest, HomOrHomAndNotHomXorAllWork) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> t_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> f_ct = boundary.lift(false);

  Cipher<Lwe, Rlwe> or_result_ct = or_(t_ct, f_ct);
  Cipher<Lwe, Rlwe> and_not_result_ct = and_not_(t_ct, f_ct);
  Cipher<Lwe, Rlwe> xor_result_ct = xor_(t_ct, f_ct);

  EXPECT_TRUE(drop(boundary, or_result_ct));
  EXPECT_TRUE(drop(boundary, and_not_result_ct));
  EXPECT_TRUE(drop(boundary, xor_result_ct));
}

// A Boundary built with only the Lwe-side secret can still drop()
// Lwe-shaped ciphertexts -- e.g. a party that only ever decodes
// already-materialized results and has no lasting need to keep the
// Rlwe-side Runtime alive as a member.
TEST_F(CipherTest, LweOnlyBoundaryDropsLweShapedCiphertexts) {
  Boundary<4, Lwe, Rlwe, Decomp> full_boundary(lwe_runtime_, rlwe_runtime_);
  Boundary<4, Lwe, Rlwe, Decomp> lwe_only_boundary(lwe_runtime_);

  Cipher<Lwe, Rlwe> a_ct = full_boundary.lift(true);
  Cipher<Lwe, Rlwe> b_ct = full_boundary.lift(true);

  Cipher<Lwe, Rlwe> result_ct = and_(a_ct, b_ct);
  relay_.materialize(result_ct);
  ASSERT_TRUE(result_ct.is_ready());

  EXPECT_TRUE(drop(lwe_only_boundary, result_ct));
}

// PublicBoundary can lift() without ever touching the secret -- built from
// a PublicKey generated under lwe_runtime_'s own secret, its output still
// decodes correctly through a secret-holding Boundary.
TEST_F(CipherTest, PublicBoundaryLiftsWithoutTheSecret) {
  static constexpr uint32_t kPkSamples = 8;
  PublicKey<typename Lwe::torus_type, Lwe::n, kPkSamples> pk =
      lwe_runtime_.template generate_public_key<kPkSamples>();
  PublicRuntime<Lwe, kPkSamples> pub(pk, eng_);
  PublicBoundary<4, Lwe, kPkSamples> public_boundary(pub);

  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> t_ct = public_boundary.lift(true);
  Cipher<Lwe, Rlwe> f_ct = public_boundary.lift(false);

  EXPECT_TRUE(t_ct.is_ready());
  EXPECT_TRUE(drop(boundary, t_ct));
  EXPECT_FALSE(drop(boundary, f_ct));
}

// HomAnd/HomOr/HomAndNot/HomXor require both operands already Lwe-shaped --
// chaining a gate's own (Rlwe-shaped) output into another gate call needs an
// explicit Relay::materialize() first.
TEST_F(CipherTest, ChainingTwoGatesNeedsExplicitMaterialize) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Cipher<Lwe, Rlwe> a_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> b_ct = boundary.lift(true);
  Cipher<Lwe, Rlwe> c_ct = boundary.lift(false);

  // (a AND b) AND c == false
  Cipher<Lwe, Rlwe> ab_ct = and_(a_ct, b_ct);
  ASSERT_FALSE(ab_ct.is_ready());
  relay_.materialize(ab_ct);
  ASSERT_TRUE(ab_ct.is_ready());

  Cipher<Lwe, Rlwe> abc_ct = and_(ab_ct, c_ct);

  EXPECT_FALSE(drop(boundary, abc_ct));
}
