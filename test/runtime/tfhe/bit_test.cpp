#include <gtest/gtest.h>

#include "tfhe/bit.hpp"
#include "tfhe/dial.hpp"
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

  using Torus = typename Lwe::torus_type;
  using rTorus = typename Rlwe::torus_type;

  // Bit's ciphertexts must be encrypted from -- and decoded through -- this
  // plaintext convention: 4 slots, not 2, because tfhe::gate::HomAnd/Or/
  // AndNot/Xor sum two encoded messages and compare against one fixed
  // boundary, which only works if each message stays under half the circle
  // (see dial.hpp's own doc comment). true/false live at indices 1/0;
  // indices 2/3 are headroom those gates need and are never used here. Bit
  // itself has no opinion on this -- it only tracks ciphertext shape -- so
  // this convention lives at the call site, not inside bit.hpp.
  using Plain = Dial<4, Torus>;
  using RPlain = Dial<4, rTorus>;

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
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);

  EXPECT_TRUE(a_ct.is_ready());
}

// hom_and's result is Rlwe-shaped -- exactly what every tfhe/gate/Hom*
// returns -- until something materializes it back down.
TEST_F(BitTest, GateResultIsNotReady) {
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);
  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);

  Bit<Lwe, Rlwe> result_ct = circuit_.And(a_ct, b_ct);

  EXPECT_FALSE(result_ct.is_ready());
  rTorus decrypted = rlwe_runtime_.decrypt(result_ct.pending_ciphertext());
  EXPECT_TRUE(RPlain(decrypted).index());
}

// Relay::materialize() is how a caller normalizes a Bit back to
// Lwe-shaped -- Circuit's And/Or/AndNot/Xor never do this on their own.
TEST_F(BitTest, ExplicitMaterializeMakesItReady) {
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);
  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);

  Bit<Lwe, Rlwe> result_ct = circuit_.And(a_ct, b_ct);
  relay_.materialize(result_ct);

  EXPECT_TRUE(result_ct.is_ready());
  Torus decrypted = lwe_runtime_.decrypt(result_ct.ready_ciphertext());
  EXPECT_TRUE(Plain(decrypted).index());

  // A second call is a harmless no-op.
  relay_.materialize(result_ct);
  EXPECT_TRUE(result_ct.is_ready());
}

TEST_F(BitTest, HomOrHomAndNotHomXorAllWork) {
  Torus t = Plain(true).value();
  Bit<Lwe, Rlwe> t_ct = lwe_runtime_.encrypt(t);
  Torus f = Plain(false).value();
  Bit<Lwe, Rlwe> f_ct = lwe_runtime_.encrypt(f);

  Bit<Lwe, Rlwe> or_result_ct = circuit_.Or(t_ct, f_ct);
  Bit<Lwe, Rlwe> and_not_result_ct = circuit_.AndNot(t_ct, f_ct);
  Bit<Lwe, Rlwe> xor_result_ct = circuit_.Xor(t_ct, f_ct);

  rTorus or_decrypted =
      rlwe_runtime_.decrypt(or_result_ct.pending_ciphertext());
  EXPECT_TRUE(RPlain(or_decrypted).index());
  rTorus and_not_decrypted =
      rlwe_runtime_.decrypt(and_not_result_ct.pending_ciphertext());
  EXPECT_TRUE(RPlain(and_not_decrypted).index());
  rTorus xor_decrypted =
      rlwe_runtime_.decrypt(xor_result_ct.pending_ciphertext());
  EXPECT_TRUE(RPlain(xor_decrypted).index());
}

// Circuit's And/Or/AndNot/Xor require both operands already Lwe-shaped --
// chaining a gate's own (Rlwe-shaped) output into another gate call needs
// an explicit Relay::materialize() first.
TEST_F(BitTest, ChainingTwoGatesNeedsExplicitMaterialize) {
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);
  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);
  Torus c = Plain(false).value();
  Bit<Lwe, Rlwe> c_ct = lwe_runtime_.encrypt(c);

  // (a AND b) AND c == false
  Bit<Lwe, Rlwe> ab_ct = circuit_.And(a_ct, b_ct);
  ASSERT_FALSE(ab_ct.is_ready());
  relay_.materialize(ab_ct);
  ASSERT_TRUE(ab_ct.is_ready());

  Bit<Lwe, Rlwe> abc_ct = circuit_.And(ab_ct, c_ct);

  rTorus decrypted = rlwe_runtime_.decrypt(abc_ct.pending_ciphertext());
  EXPECT_FALSE(RPlain(decrypted).index());
}
