#include <gtest/gtest.h>

#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace trlwe_operation_test {
struct Ctx1 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
};

using TestContexts = ::testing::Types<Ctx1>;

}  // namespace trlwe_operation_test

template <typename Ctx>
class TrlweOperationFixture : public ::testing::Test {
 protected:
  static inline std::shared_ptr<const Poly<UInt, Ctx::N>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> cryptor_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Torus = typename Ctx::Torus;

  void SetUp() override {
    this->eng_.seed(std::random_device{}());
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->secret_ = std::make_shared<const Poly<UInt, Ctx::N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ =
        std::make_unique<trlwe::Cryptor<Ctx>>(this->secret_, this->eng_);
  }
};

TYPED_TEST_SUITE(TrlweOperationFixture, trlwe_operation_test::TestContexts);

TYPED_TEST(TrlweOperationFixture, AdditionCorrectness) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  Poly<ModTorus<16>, Ctx::N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(2);

  Poly<Torus, Ctx::N> expected = lhs + rhs;

  TRLWE<Torus, Ctx::N> encrypted_lhs =
      this->cryptor_->template encrypt<Torus>(lhs);
  TRLWE<Torus, Ctx::N> encrypted_rhs =
      this->cryptor_->template encrypt<Torus>(rhs);

  TRLWE<Torus, Ctx::N> encrypted_add = encrypted_lhs + encrypted_rhs;

  Poly<Torus, Ctx::N> decrypted =
      this->cryptor_->template decrypt<Torus>(encrypted_add);

  double norm = infinity_norm(decrypted - expected);

  EXPECT_LE(norm, 0.1);
}