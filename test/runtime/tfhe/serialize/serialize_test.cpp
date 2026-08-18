#include "tfhe/serialize.hpp"
#include <gtest/gtest.h>

#include <random>

#include "primitive/torus.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace serialize_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe_, typename Rlwe_, typename Decomp_, typename Kst_>
struct ParameterSet {
  using Lwe = Lwe_;
  using Rlwe = Rlwe_;
  using Decomp = Decomp_;
  using Kst = Kst_;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 4>>,
                          rlwe_params<trlwe_core_params<ModTorus<16>, 16>>,
                          dcp_params<4, 6>, kst_params<4, 6>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 20>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 128>>,
                          dcp_params<256, 2>, kst_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace serialize_test

template <typename Ctx>
class SerializeRoundtripTest : public ::testing::Test {
 protected:
  using Lwe = typename Ctx::context::Lwe;
  using Rlwe = typename Ctx::context::Rlwe;
  using Decomp = typename Ctx::context::Decomp;
  using Kst = typename Ctx::context::Kst;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};
};

TYPED_TEST_SUITE(SerializeRoundtripTest, serialize_test::TestContexts);

// Every type below round-trips as raw structural data (see serde.hpp), so
// a deserialized value is expected to be byte-for-byte identical to the
// original -- checked by re-serializing it and comparing bytes, since most
// of these ciphertext/key types don't define operator==.

TYPED_TEST(SerializeRoundtripTest, Tlwe) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, Tracking> runtime(this->eng_);
  TLWE<Torus, Lwe::n> ct = runtime.encrypt(Torus(1u, 2u));

  auto bytes = serialize::to_bytes(ct);
  auto back = serialize::from_bytes<decltype(ct)>(bytes);
  EXPECT_EQ(bytes, serialize::to_bytes(back));
}

TYPED_TEST(SerializeRoundtripTest, PublicKey) {
  using Lwe = typename TestFixture::Lwe;

  Runtime<Lwe, Tracking> runtime(this->eng_);
  auto pk = runtime.template generate_public_key<2 * Lwe::n>();

  auto bytes = serialize::to_bytes(pk);
  auto back = serialize::from_bytes<decltype(pk)>(bytes);
  EXPECT_EQ(bytes, serialize::to_bytes(back));
}

TYPED_TEST(SerializeRoundtripTest, BootstrapKeyAndKeySwitchKey) {
  using Lwe = typename TestFixture::Lwe;
  using Rlwe = typename TestFixture::Rlwe;
  using Decomp = typename TestFixture::Decomp;
  using Kst = typename TestFixture::Kst;
  using RlweRuntime = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>;

  Runtime<Lwe, Tracking> lwe_runtime(this->eng_);
  RlweRuntime rlwe_runtime(this->eng_);

  auto bk = rlwe_runtime.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
      lwe_runtime.holder().get());
  auto bk_bytes = serialize::to_bytes(bk);
  auto bk_back = serialize::from_bytes<decltype(bk)>(bk_bytes);
  EXPECT_EQ(bk_bytes, serialize::to_bytes(bk_back));

  auto ksk = lwe_runtime.template generate_key_switch_key<ExtractedLwe<Rlwe>,
                                                          Lwe, Kst>(
      rlwe_runtime.holder().get());
  auto ksk_bytes = serialize::to_bytes(ksk);
  auto ksk_back = serialize::from_bytes<decltype(ksk)>(ksk_bytes);
  EXPECT_EQ(ksk_bytes, serialize::to_bytes(ksk_back));
}
