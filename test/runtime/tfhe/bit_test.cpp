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

  Bit<Lwe, Rlwe> result_ct = tfhe::bit::And<Kst, Decomp>(a_ct, b_ct, BK_, KSK_);

  EXPECT_FALSE(result_ct.is_ready());
  rTorus decrypted = rlwe_runtime_.decrypt(result_ct.pending_ciphertext());
  EXPECT_TRUE(RPlain(decrypted).index());
}

// Calling materialize() explicitly is a valid way to normalize a Bit --
// the caller's own way to control when that step happens rather than
// leaving it to the next gate call.
TEST_F(BitTest, ExplicitMaterializeMakesItReady) {
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);
  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);

  Bit<Lwe, Rlwe> result_ct = tfhe::bit::And<Kst, Decomp>(a_ct, b_ct, BK_, KSK_);
  result_ct.materialize<Kst>(KSK_);

  EXPECT_TRUE(result_ct.is_ready());
  Torus decrypted = lwe_runtime_.decrypt(result_ct.ready_ciphertext());
  EXPECT_TRUE(Plain(decrypted).index());

  // A second call is a harmless no-op.
  result_ct.materialize<Kst>(KSK_);
  EXPECT_TRUE(result_ct.is_ready());
}

TEST_F(BitTest, HomOrHomAndNotHomXorAllWork) {
  Torus t = Plain(true).value();
  Bit<Lwe, Rlwe> t_ct = lwe_runtime_.encrypt(t);
  Torus f = Plain(false).value();
  Bit<Lwe, Rlwe> f_ct = lwe_runtime_.encrypt(f);

  Bit<Lwe, Rlwe> or_result_ct =
      tfhe::bit::Or<Kst, Decomp>(t_ct, f_ct, BK_, KSK_);
  Bit<Lwe, Rlwe> and_not_result_ct =
      tfhe::bit::AndNot<Kst, Decomp>(t_ct, f_ct, BK_, KSK_);
  Bit<Lwe, Rlwe> xor_result_ct =
      tfhe::bit::Xor<Kst, Decomp>(t_ct, f_ct, BK_, KSK_);

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

// The whole point of Bit: chaining gates needs no explicit KeySwitch call
// from the caller -- hom_and here materializes its not-yet-ready operand
// (the output of the first hom_and) automatically.
TEST_F(BitTest, ChainingTwoGatesRequiresNoExplicitMaterialize) {
  Torus a = Plain(true).value();
  Bit<Lwe, Rlwe> a_ct = lwe_runtime_.encrypt(a);
  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);
  Torus c = Plain(false).value();
  Bit<Lwe, Rlwe> c_ct = lwe_runtime_.encrypt(c);

  // (a AND b) AND c == false
  Bit<Lwe, Rlwe> ab_ct = tfhe::bit::And<Kst, Decomp>(a_ct, b_ct, BK_, KSK_);
  ASSERT_FALSE(ab_ct.is_ready());

  Bit<Lwe, Rlwe> abc_ct = tfhe::bit::And<Kst, Decomp>(ab_ct, c_ct, BK_, KSK_);

  rTorus decrypted = rlwe_runtime_.decrypt(abc_ct.pending_ciphertext());
  EXPECT_FALSE(RPlain(decrypted).index());
}

// hom_and takes both operands by const& and never mutates them --
// materialize()'s effect inside detail::combine() stays local to that
// call.
TEST_F(BitTest, GateCallsDoNotMutateTheirOperands) {
  Torus a0 = Plain(true).value();
  Bit<Lwe, Rlwe> a0_ct = lwe_runtime_.encrypt(a0);
  Torus b0 = Plain(true).value();
  Bit<Lwe, Rlwe> b0_ct = lwe_runtime_.encrypt(b0);
  Bit<Lwe, Rlwe> a_ct = tfhe::bit::And<Kst, Decomp>(a0_ct, b0_ct, BK_, KSK_);
  ASSERT_FALSE(a_ct.is_ready());

  Torus b = Plain(true).value();
  Bit<Lwe, Rlwe> b_ct = lwe_runtime_.encrypt(b);
  Bit<Lwe, Rlwe> unused_ct = tfhe::bit::And<Kst, Decomp>(a_ct, b_ct, BK_, KSK_);
  (void)unused_ct;

  EXPECT_FALSE(a_ct.is_ready());
}
