#include <gtest/gtest.h>

#include "tfhe/bit.hpp"
#include "tfhe/boundary.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/scope.hpp"
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

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>> rlwe_runtime_;

  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;

  void SetUp() override {
    lwe_runtime_ = Runtime<Lwe>(eng_);
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>>(eng_);

    circuit_ = Circuit<Lwe, Rlwe, Decomp>(
        rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
            lwe_runtime_.holder().get()));
    relay_ = Relay<Lwe, Rlwe, Kst>(
        lwe_runtime_
            .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                rlwe_runtime_.holder().get()));
  }
};

// A freshly-encrypted Bit starts out Lwe-shaped.
TEST_F(BitTest, FreshlyEncryptedBitIsReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Bit<Lwe, Rlwe> a_ct = boundary.lift(true);

  EXPECT_TRUE(a_ct.is_ready());
}

// hom_and's result is Rlwe-shaped -- exactly what every tfhe/gate/Hom*
// returns -- until something materializes it back down.
TEST_F(BitTest, GateResultIsNotReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Bit<Lwe, Rlwe> a_ct = boundary.lift(true);
  Bit<Lwe, Rlwe> b_ct = boundary.lift(true);

  Bit<Lwe, Rlwe> result_ct = circuit_.And(a_ct, b_ct);

  EXPECT_FALSE(result_ct.is_ready());
  EXPECT_TRUE(boundary.drop(result_ct));
}

// Relay::materialize() is how a caller normalizes a Bit back to
// Lwe-shaped -- Circuit's And/Or/AndNot/Xor never do this on their own.
TEST_F(BitTest, ExplicitMaterializeMakesItReady) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Bit<Lwe, Rlwe> a_ct = boundary.lift(true);
  Bit<Lwe, Rlwe> b_ct = boundary.lift(true);

  Bit<Lwe, Rlwe> result_ct = circuit_.And(a_ct, b_ct);
  relay_.materialize(result_ct);

  EXPECT_TRUE(result_ct.is_ready());
  bool decoded = lwe_runtime_.decrypt(result_ct.ready()).value() != 0;
  (void)decoded;

  // A second call is a harmless no-op.
  relay_.materialize(result_ct);
  EXPECT_TRUE(result_ct.is_ready());
}

TEST_F(BitTest, HomOrHomAndNotHomXorAllWork) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Bit<Lwe, Rlwe> t_ct = boundary.lift(true);
  Bit<Lwe, Rlwe> f_ct = boundary.lift(false);

  Bit<Lwe, Rlwe> or_result_ct = circuit_.Or(t_ct, f_ct);
  Bit<Lwe, Rlwe> and_not_result_ct = circuit_.AndNot(t_ct, f_ct);
  Bit<Lwe, Rlwe> xor_result_ct = circuit_.Xor(t_ct, f_ct);

  EXPECT_TRUE(boundary.drop(or_result_ct));
  EXPECT_TRUE(boundary.drop(and_not_result_ct));
  EXPECT_TRUE(boundary.drop(xor_result_ct));
}

// Circuit's And/Or/AndNot/Xor require both operands already Lwe-shaped --
// chaining a gate's own (Rlwe-shaped) output into another gate call needs
// an explicit Relay::materialize() first.
TEST_F(BitTest, ChainingTwoGatesNeedsExplicitMaterialize) {
  Boundary<4, Lwe, Rlwe, Decomp> boundary(lwe_runtime_, rlwe_runtime_);

  Bit<Lwe, Rlwe> a_ct = boundary.lift(true);
  Bit<Lwe, Rlwe> b_ct = boundary.lift(true);
  Bit<Lwe, Rlwe> c_ct = boundary.lift(false);

  // (a AND b) AND c == false
  Bit<Lwe, Rlwe> ab_ct = circuit_.And(a_ct, b_ct);
  ASSERT_FALSE(ab_ct.is_ready());
  relay_.materialize(ab_ct);
  ASSERT_TRUE(ab_ct.is_ready());

  Bit<Lwe, Rlwe> abc_ct = circuit_.And(ab_ct, c_ct);

  EXPECT_FALSE(boundary.drop(abc_ct));
}
