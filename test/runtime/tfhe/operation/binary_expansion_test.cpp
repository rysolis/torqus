#include "tfhe/operation/binary_expansion.hpp"
#include <gtest/gtest.h>

#include "primitive/torus.hpp"

#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"

namespace binary_expansion_test {
template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Dcp>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 4>>,
                              rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                              dcp_params<4, 3>>;

using Context2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 32>>,
                              rlwe_params<trlwe_core_params<ModTorus<32>, 32>>,
                              dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;
}  // namespace binary_expansion_test

template <typename Context>
class BinaryExpansionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{10};
  using Lwe = Context::lwe_params;
  using Rlwe = Context::rlwe_params;
  using Dcp = Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = Lwe::torus_type;
  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Dcp::l;

  Runtime<Cryptor<Lwe>, Tracking> lwe_runtime_;
  Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> BK_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(eng_);
    lwe_runtime_ = Runtime<Cryptor<Lwe>, Tracking>(
        std::begin(rlwe_runtime_), std::end(rlwe_runtime_), eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Dcp>(
        lwe_runtime_.secret());
  }
};

template <typename Config>
class BinaryExpansionCorrectnessTest
    : public BinaryExpansionFixture<typename Config::context> {
 protected:
  using Base = BinaryExpansionFixture<typename Config::context>;

  using Torus = Base::Torus;
  using rTorus = Base::rTorus;

  struct TestCase {
    Vector<Torus, 2> operand;
    Vector<rTorus, 4> ref;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.operand = {Torus(0u), Torus(0u)},
             .ref = {rTorus(1u, 4u), rTorus(0u), rTorus(0u), rTorus(0u)}},
            {.operand = {Torus(1u, 4u), Torus(0u)},
             .ref = {rTorus(0u), rTorus(1u, 4u), rTorus(0u), rTorus(0u)}},
            {.operand = {Torus(0u), Torus(1u, 4u)},
             .ref = {rTorus(0u), rTorus(0u), rTorus(1u, 4u), rTorus(0u)}},
            {.operand = {Torus(1u, 4u), Torus(1u, 4u)},
             .ref = {rTorus(0u), rTorus(0u), rTorus(0u), rTorus(1u, 4u)}}};
  }
};

TYPED_TEST_SUITE(BinaryExpansionCorrectnessTest,
                 binary_expansion_test::TestContexts);

TYPED_TEST(BinaryExpansionCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  static_assert(n == N, "n and N must be equal for this test");

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Vector<Torus, 2> operand = tc.operand;

    Vector<TLWE<Torus, n>, 2> operand_ct;
    operand_ct[0] = this->lwe_runtime_.encrypt(static_cast<Torus>(operand[0]));
    operand_ct[1] = this->lwe_runtime_.encrypt(static_cast<Torus>(operand[1]));

    // ==================================
    // Act
    // ==================================
    Vector<TLWE<rTorus, N>, 4> res_ct =
        BinaryExpansion<4, Lwe, Rlwe, Dcp>::exec_impl(operand_ct, this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Vector<rTorus, 4> ref = tc.ref;

    // compute actual result
    Vector<rTorus, 4> res;
    res[0] = this->lwe_runtime_.decrypt(res_ct[0]);
    res[1] = this->lwe_runtime_.decrypt(res_ct[1]);
    res[2] = this->lwe_runtime_.decrypt(res_ct[2]);
    res[3] = this->lwe_runtime_.decrypt(res_ct[3]);

    Vector<rTorus, 4> err = res - ref;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "         BinaryExpansion Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": " << operand << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';

    EXPECT_LE(norm, 0.1);
  }
}