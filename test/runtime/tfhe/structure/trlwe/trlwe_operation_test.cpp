#include <gtest/gtest.h>

#include <random>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"
#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace trlwe_operation_test {
struct Ctx1 {
  using torus_type = ModTorus;
  static constexpr uint32_t N = 4;
};

using TestContexts = ::testing::Types<Ctx1>;

}  // namespace trlwe_operation_test

template <typename Ctx>
class TrlweOperationFixture : public ::testing::Test {
 protected:
  static inline std::shared_ptr<const Poly<UInt>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> cryptor_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  void SetUp() override {
    this->eng_.seed(std::random_device{}());
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<ModTorus::raw_value_type> torus_dist(
        0, Torus::Q - 1);

    this->secret_ = std::make_shared<const Poly<UInt>>(
        Ctx::N, [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ = std::make_unique<trlwe::Cryptor<Ctx>>(
        this->secret_, this->eng_, torus_dist);
  }
};

TYPED_TEST_SUITE(TrlweOperationFixture, trlwe_operation_test::TestContexts);

TYPED_TEST(TrlweOperationFixture, AdditionCorrectness) {
  using Ctx = TypeParam;

  Poly<ModTorus> lhs(Ctx::N), rhs(Ctx::N);
  lhs[0] = ModTorus(1);
  rhs[0] = ModTorus(2);

  Poly<ModTorus> expected = lhs + rhs;

  TRLWE<ModTorus> encrypted_lhs =
      this->cryptor_->template encrypt<ModTorus>(lhs);
  TRLWE<ModTorus> encrypted_rhs =
      this->cryptor_->template encrypt<ModTorus>(lhs);

  TRLWE<ModTorus> encrypted_add = encrypted_lhs + encrypted_rhs;

  Poly<ModTorus> decrypted =
      this->cryptor_->template decrypt<ModTorus>(encrypted_add);

  double norm = infinity_norm(decrypted - expected);

  EXPECT_LE(norm, 0.1);
}