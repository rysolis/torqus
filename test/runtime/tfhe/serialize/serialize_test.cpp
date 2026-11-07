#include "tfhe/serialize.hpp"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

#include "primitive/torus.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

// encode<N>/decode<N> (buffer.hpp) are plain byte-array<->integer
// conversion, no stream involved -- checked directly here rather than
// only indirectly through a Writer/Reader round-trip.
TEST(SerializeBufferDetailTest, EncodeDecodeU32RoundTrips) {
  uint32_t v = 0x12345678u;
  EXPECT_EQ(serialize::detail::decode<4>(serialize::detail::encode<4>(v)), v);
}

TEST(SerializeBufferDetailTest, EncodeDecodeU64RoundTrips) {
  uint64_t v = 0x0123456789abcdefull;
  EXPECT_EQ(serialize::detail::decode<8>(serialize::detail::encode<8>(v)), v);
}

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

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  TLWE<Torus, Lwe::n> ct = runtime.encrypt(Torus(1u, 2u));

  auto bytes = serialize::write(ct);
  auto back = serialize::read<decltype(ct)>(bytes);
  EXPECT_EQ(bytes, serialize::write(back));
}

TYPED_TEST(SerializeRoundtripTest, PublicKey) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  auto pk = runtime.template generate_public_key<Torus, 2 * Lwe::n>();

  auto bytes = serialize::write(pk);
  auto back = serialize::read<decltype(pk)>(bytes);
  EXPECT_EQ(bytes, serialize::write(back));
}

TYPED_TEST(SerializeRoundtripTest, BootstrapKeyAndKeySwitchKey) {
  using Lwe = typename TestFixture::Lwe;
  using Rlwe = typename TestFixture::Rlwe;
  using Decomp = typename TestFixture::Decomp;
  using Kst = typename TestFixture::Kst;
  using RlweRuntime = Runtime<ParamsPack<Rlwe, Decomp>,
                              RandomGenerator<std::mt19937>, Tracking>;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> lwe_runtime(this->eng_);
  RlweRuntime rlwe_runtime(this->eng_);

  auto bk = rlwe_runtime.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
      lwe_runtime.secret());
  auto bk_bytes = serialize::write(bk);
  auto bk_back = serialize::read<decltype(bk)>(bk_bytes);
  EXPECT_EQ(bk_bytes, serialize::write(bk_back));

  auto ksk =
      lwe_runtime
          .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
              rlwe_runtime.secret());
  auto ksk_bytes = serialize::write(ksk);
  auto ksk_back = serialize::read<decltype(ksk)>(ksk_bytes);
  EXPECT_EQ(ksk_bytes, serialize::write(ksk_back));
}

// write(ostream&, ...)/read<T>(istream&) (#51) against std::ostringstream/
// std::istringstream -- same bytes as write/read on a vector<byte>, just
// without ever materializing the whole value as a byte buffer first.
TYPED_TEST(SerializeRoundtripTest, StreamRoundtrip) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  TLWE<Torus, Lwe::n> ct = runtime.encrypt(Torus(1u, 2u));

  std::ostringstream oss(std::ios::binary);
  serialize::write(oss, ct);

  std::istringstream iss(std::move(oss).str(), std::ios::binary);
  auto back = serialize::read<decltype(ct)>(iss);

  EXPECT_EQ(serialize::write(ct), serialize::write(back));
}

// Same, against a real file -- the actual motivating case: a huge key
// should reach disk without a second full in-memory copy of itself.
TYPED_TEST(SerializeRoundtripTest, StreamRoundtripViaFile) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  TLWE<Torus, Lwe::n> ct = runtime.encrypt(Torus(1u, 2u));

  std::filesystem::path path =
      std::filesystem::temp_directory_path() / "torqus_serialize_test_tlwe.bin";
  {
    std::ofstream ofs(path, std::ios::binary);
    serialize::write(ofs, ct);
  }
  TLWE<Torus, Lwe::n> back;
  {
    std::ifstream ifs(path, std::ios::binary);
    back = serialize::read<decltype(ct)>(ifs);
  }
  std::filesystem::remove(path);

  EXPECT_EQ(serialize::write(ct), serialize::write(back));
}
