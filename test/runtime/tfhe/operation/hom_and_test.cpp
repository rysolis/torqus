#include <gtest/gtest.h>

#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/hom_and.hpp"
#include "tfhe/operation/sub.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace hom_and_test {
template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Dcp>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 1>>,
                          rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 20>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;
}  // namespace hom_and_test

template <typename Ctx>
class HomAndFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{10};
  using Lwe = Ctx::lwe_params;
  using Rlwe = Ctx::rlwe_params;
  using Dcp = Ctx::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = Lwe::torus_type;
  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Dcp::l;

  Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> exe_;
  Executor<Cryptor<Lwe>, Tracking> tlwe_exe_;

  BootstrapKey<rTorus, N, l, n> BK_;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    Cryptor<ParamsPack<Rlwe, Dcp>> cryptor(glwe_kr.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(cryptor);

    SecretHolder<Lwe> lwe_kr(eng_);
    Cryptor<Lwe> lwe_cryptor(lwe_kr.secret_ptr(), eng_);
    tlwe_exe_ = Executor<Cryptor<Lwe>, Tracking>(lwe_cryptor);

    // Prepare Bootstrapkey
    BK_ = bootstrap_key::generate<Lwe, Rlwe, Dcp>(exe_, lwe_kr);
  }
};

template <typename Config>
class HomAndCorrectnessTest : public HomAndFixture<typename Config::context> {
 protected:
  using Base = HomAndFixture<typename Config::context>;

  using Torus = Base::Torus;
  using rTorus = Base::rTorus;

  void SetUp() override { Base::SetUp(); }

  struct TestCase {
    Torus lhs;
    Torus rhs;
    rTorus ref;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.lhs = Torus(1u, 4u);   // encode 1/4 in Torus
      tc.rhs = Torus(1u, 4u);   // encode 1/4 in Torus
      tc.ref = rTorus(1u, 4u);  // encode 1/4 in Torus
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = Torus(0u);
      tc.rhs = Torus(1u, 4u);  // encode 1/4 in Torus
      tc.ref = rTorus(0u);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = Torus(0u);
      tc.rhs = Torus(0u);
      tc.ref = rTorus(0u);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(HomAndCorrectnessTest, hom_and_test::TestContexts);

TYPED_TEST(HomAndCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    // Prepare TLWE
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;
    TLWE<Torus, n> lhs_ct = this->tlwe_exe_.encrypt(lhs);
    TLWE<Torus, n> rhs_ct = this->tlwe_exe_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct =
        HomAnd<Lwe, Rlwe, Dcp>::exec_impl(lhs_ct, rhs_ct, this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    rTorus ref = tc.ref;

    // comput actual result
    rTorus res = this->exe_.decrypt(res_ct);

    rTorus err = res - ref;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "           HomAnd Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
    std::cout << std::setw(14) << "rhs" << ": " << lhs << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << " ("
              << double(ref) << ")\n";
    std::cout << std::setw(14) << "actual" << ": " << res << " (" << double(res)
              << ")\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';

    EXPECT_LE(norm, 0.1);
  }
}

template <typename Config>
class HomAndNotCorrectnessTest
    : public HomAndFixture<typename Config::context> {
 protected:
  using Base = HomAndFixture<typename Config::context>;

  using Torus = Base::Torus;
  using rTorus = Base::rTorus;

  void SetUp() override { Base::SetUp(); }

  struct TestCase {
    Torus lhs;
    Torus rhs;
    rTorus ref;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.lhs = Torus(1u, 4u);  // encode 1/4 in Torus
      tc.rhs = Torus(1u, 4u);  // encode 1/4 in Torus
      tc.ref = rTorus(0u);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = Torus(0u);
      tc.rhs = Torus(1u, 4u);  // encode 1/4 in Torus
      tc.ref = rTorus(0u);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = Torus(1u, 4u);  // encode 1/4 in Torus
      tc.rhs = Torus(0u);
      tc.ref = rTorus(1u, 4u);  // encode 1/4 in Torus
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = Torus(0u);
      tc.rhs = Torus(0u);
      tc.ref = rTorus(0);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(HomAndNotCorrectnessTest, hom_and_test::TestContexts);

TYPED_TEST(HomAndNotCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    // Prepare TLWE
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;
    TLWE<Torus, n> lhs_ct = this->tlwe_exe_.encrypt(lhs);
    TLWE<Torus, n> rhs_ct = this->tlwe_exe_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct =
        HomAndNot<Lwe, Rlwe, Dcp>::exec_impl(lhs_ct, rhs_ct, this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    rTorus ref = tc.ref;

    // comput actual result
    rTorus res = this->exe_.decrypt(res_ct);

    rTorus err = res - ref;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "           HomAndNot Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
    std::cout << std::setw(14) << "rhs" << ": " << lhs << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << " ("
              << double(ref) << ")\n";
    std::cout << std::setw(14) << "actual" << ": " << res << " (" << double(res)
              << ")\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';

    EXPECT_LE(norm, 0.1);
  }
}