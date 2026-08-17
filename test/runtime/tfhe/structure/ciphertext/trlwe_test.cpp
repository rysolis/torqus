#include "tfhe/structure/ciphertext/trlwe.hpp"
#include <gtest/gtest.h>

#include "primitive/torus.hpp"

#include "algebra/poly.hpp"

#include "tfhe/params.hpp"

namespace trlwe_basic_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using Params = T;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<8>, 32>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace trlwe_basic_test

template <typename Ctx>
class TrlweBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(TrlweBasicTest, trlwe_basic_test::TestContexts);

TYPED_TEST(TrlweBasicTest, SizeConstructor_initializesBuffer) {
  using Params = typename TypeParam::context::Params;

  using Torus = Params::torus_type;
  constexpr uint32_t N = Params::N;

  TRLWE<Torus, N> trlwe;
  EXPECT_EQ(trlwe.a().size(), N);
  EXPECT_EQ(trlwe.a(), (Poly<Torus, N>()));
  EXPECT_EQ(trlwe.b().size(), N);
  EXPECT_EQ(trlwe.b(), (Poly<Torus, N>()));
}

TYPED_TEST(TrlweBasicTest, Generator_InitializedBuffer) {
  using Params = typename TypeParam::context::Params;

  using Torus = Params::torus_type;
  constexpr uint32_t N = Params::N;

  TRLWE<Torus, N> trlwe([]() { return Torus(10u); });
  EXPECT_EQ(trlwe.a().size(), N);
  EXPECT_EQ(trlwe.a(), (Poly<Torus, N>([]() { return Torus(10u); })));
  EXPECT_EQ(trlwe.b().size(), N);
  EXPECT_EQ(trlwe.b(), (Poly<Torus, N>()));
}
