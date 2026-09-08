#include <gtest/gtest.h>

#include <array>
#include <random>
#include <vector>

#include "tfhe/cipher.hpp"
#include "tfhe/circuit/binary_expansion.hpp"
#include "tfhe/circuit/relay.hpp"
#include "tfhe/circuit/reslot.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/params.hpp"

namespace party_test {
// Same params as cipher_test.cpp's own Context.
using Lwe = lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>;
using Rlwe =
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>;
using Decomp = dcp_params<16, 7>;
using Kst = kst_params<2, 11>;
}  // namespace party_test

class PartyTest : public ::testing::Test {
 protected:
  using Lwe = party_test::Lwe;
  using Rlwe = party_test::Rlwe;
  using Decomp = party_test::Decomp;
  using Kst = party_test::Kst;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  Party<Lwe, Rlwe, Decomp, Kst> party_{eng_};

  void SetUp() override {
    party_.generate_bootstrap_key();
    party_.generate_key_switch_key();
  }
};

// lift<Resolution>() always produces an Lwe-shaped Cipher, same as
// Boundary::lift().
TEST_F(PartyTest, LiftProducesAReadyCipher) {
  Cipher<Lwe, Rlwe> a_ct = party_.lift<4>(true);

  EXPECT_TRUE(a_ct.is_ready());
}

// drop<Resolution>() handles a gate's Rlwe-shaped output the same way the
// free drop(boundary, bit) does -- Party just builds a Boundary and
// delegates to it.
TEST_F(PartyTest, DropDecodesAGateResult) {
  Cipher<Lwe, Rlwe> t_ct = party_.lift<4>(true);
  Cipher<Lwe, Rlwe> f_ct = party_.lift<4>(false);

  Cipher<Lwe, Rlwe> tt_ct = tfhe::gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(
      t_ct.ready(), t_ct.ready(), party_.bootstrap_key());
  Cipher<Lwe, Rlwe> tf_ct = tfhe::gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(
      t_ct.ready(), f_ct.ready(), party_.bootstrap_key());

  EXPECT_TRUE(party_.drop<4>(tt_ct));
  EXPECT_FALSE(party_.drop<4>(tf_ct));
}

// The same Party, with the same underlying secrets, lift<Resolution>()s
// and drop<Resolution>()s under whichever Resolution a given value needs
// -- Resolution names one value's encoding, not a property of the party
// itself. Mirrors cipher_test.cpp building several differently-resolved
// Boundarys from one pair of Runtimes.
TEST_F(PartyTest, LiftAndDropWorkAtSeveralResolutions) {
  Cipher<Lwe, Rlwe> t_ct = party_.lift<2>(true);
  Cipher<Lwe, Rlwe> f_ct = party_.lift<2>(false);

  EXPECT_EQ(party_.drop<2>(t_ct), 1u);
  EXPECT_EQ(party_.drop<100>(party_.lift<100>(1u)), 1u);
  EXPECT_EQ(party_.drop<2>(f_ct), 0u);
}

// The BootstrapKey/KeySwitchKey Party generates internally from its own
// two Runtimes are usable exactly like any other -- bootstrap_key() above
// already exercises HomAnd; key_switch_key() feeds a Relay the same way a
// caller-held KeySwitchKey would.
TEST_F(PartyTest, KeySwitchKeyMaterializesAGateResult) {
  Relay<Lwe, Rlwe, Kst> relay(party_.key_switch_key());

  Cipher<Lwe, Rlwe> t_ct = party_.lift<4>(true);
  Cipher<Lwe, Rlwe> result_ct =
      tfhe::gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(
          t_ct.ready(), t_ct.ready(), party_.bootstrap_key());
  ASSERT_FALSE(result_ct.is_ready());

  relay.materialize(result_ct);

  EXPECT_TRUE(result_ct.is_ready());
  EXPECT_TRUE(party_.drop<4>(result_ct));
}

// tfhe::circuit::Reslot takes the same BootstrapKey/KeySwitchKey Party
// owns -- a smoke check that they plug straight in, not a re-verification
// of Reslot's own math (see CircuitReslotCorrectnessTest for that).
TEST_F(PartyTest, BootstrapKeyAndKeySwitchKeyFeedReslot) {
  tfhe::circuit::Reslot<2, 4, Lwe, Rlwe, Decomp, Kst> reslot(
      party_.bootstrap_key(), party_.key_switch_key());

  Cipher<Lwe, Rlwe> t_ct = party_.lift<2>(true);
  Cipher<Lwe, Rlwe> t_resloted = reslot.exec(t_ct);

  EXPECT_TRUE(party_.drop<4>(t_resloted));
}

// Same idea for tfhe::circuit::BinaryExpansion -- see
// BinaryExpansionCorrectnessTest for the full one-hot correctness matrix.
TEST_F(PartyTest, BootstrapKeyAndKeySwitchKeyFeedBinaryExpansion) {
  tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst> expansion(
      party_.bootstrap_key(), party_.key_switch_key());

  std::vector<TLWE<typename Lwe::torus_type, Lwe::n>> operand_ct;
  operand_ct.push_back(party_.lift<4>(true).ready());
  operand_ct.push_back(party_.lift<4>(false).ready());

  std::array<Cipher<Lwe, Rlwe>, 4> res_ct = expansion.exec(operand_ct);

  for (uint32_t i = 0; i < 4; ++i) {
    EXPECT_EQ(party_.drop<4>(res_ct[i]), i == 1);
  }
}
