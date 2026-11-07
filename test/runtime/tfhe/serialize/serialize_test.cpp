#include "tfhe/serialize.hpp"
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

#include "primitive/modint.hpp"
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

  auto bytes = serialize::to_bytes(ct);
  auto back = serialize::from_bytes<decltype(ct)>(bytes);
  EXPECT_EQ(bytes, serialize::to_bytes(back));
}

TYPED_TEST(SerializeRoundtripTest, PublicKey) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  auto pk = runtime.template generate_public_key<Torus, 2 * Lwe::n>();

  auto bytes = serialize::to_bytes(pk);
  auto back = serialize::from_bytes<decltype(pk)>(bytes);
  EXPECT_EQ(bytes, serialize::to_bytes(back));
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
  auto bk_bytes = serialize::to_bytes(bk);
  auto bk_back = serialize::from_bytes<decltype(bk)>(bk_bytes);
  EXPECT_EQ(bk_bytes, serialize::to_bytes(bk_back));

  auto ksk =
      lwe_runtime
          .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
              rlwe_runtime.secret());
  auto ksk_bytes = serialize::to_bytes(ksk);
  auto ksk_back = serialize::from_bytes<decltype(ksk)>(ksk_bytes);
  EXPECT_EQ(ksk_bytes, serialize::to_bytes(ksk_back));
}

// write(ostream&, ...)/read<T>(istream&) (#51) against std::ostringstream/
// std::istringstream -- same bytes as to_bytes/from_bytes, just without
// ever materializing the whole value as a byte buffer first.
TYPED_TEST(SerializeRoundtripTest, StreamRoundtrip) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  TLWE<Torus, Lwe::n> ct = runtime.encrypt(Torus(1u, 2u));

  std::ostringstream oss(std::ios::binary);
  serialize::write(oss, ct);

  std::istringstream iss(std::move(oss).str(), std::ios::binary);
  auto back = serialize::read<decltype(ct)>(iss);

  EXPECT_EQ(serialize::to_bytes(ct), serialize::to_bytes(back));
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

  EXPECT_EQ(serialize::to_bytes(ct), serialize::to_bytes(back));
}

// The one-call buffer copy for Vector/Poly of scalars must produce exactly
// the per-element little-endian encoding Writer::write does.
template <typename Torus, typename Container>
std::vector<std::byte> per_element_bytes(const Container& c) {
  std::ostringstream oss(std::ios::binary);
  serialize::Writer w(oss);
  for (size_t i = 0; i < c.size(); ++i) {
    serialize::Serde<Torus>::serialize(w, c[i]);
  }
  std::string s = std::move(oss).str();
  const auto* begin = reinterpret_cast<const std::byte*>(s.data());
  return std::vector<std::byte>(begin, begin + s.size());
}

TEST(SerializeBulkTest, PolyMatchesPerElementEncoding) {
  using T = ModTorus<32, uint32_t>;
  std::mt19937 rng(0);  // NOLINT(cert-msc51-cpp)
  Poly<T, 64> p([&] { return T(static_cast<uint32_t>(rng())); });

  auto bytes = serialize::to_bytes(p);
  EXPECT_EQ(bytes, per_element_bytes<T>(p));
  EXPECT_EQ(serialize::to_bytes(serialize::from_bytes<decltype(p)>(bytes)),
            bytes);
}

TEST(SerializeBulkTest, VectorMatchesPerElementEncoding) {
  using T = ModTorus<64, uint64_t>;
  std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
  Vector<T, 37> v([&] { return T(static_cast<uint64_t>(rng())); });

  auto bytes = serialize::to_bytes(v);
  EXPECT_EQ(bytes, per_element_bytes<T>(v));
  EXPECT_EQ(serialize::to_bytes(serialize::from_bytes<decltype(v)>(bytes)),
            bytes);
}

// Raw values outside ModTorus<16>'s range are masked on read, the same as
// constructing ModTorus<16>(raw) does.
TEST(SerializeBulkTest, ReadNormalizesOutOfRangeRawValues) {
  using T = ModTorus<16, uint32_t>;
  Vector<ModTorus<32, uint32_t>, 4> wide(
      [](size_t i) { return ModTorus<32, uint32_t>(0xabcd0000u + i); });

  const auto back =
      serialize::from_bytes<Vector<T, 4>>(serialize::to_bytes(wide));
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(back[i].value(), T(0xabcd0000u + i).value());
  }
}

TEST(SerializeBulkTest, TruncatedStreamThrows) {
  using T = ModTorus<32, uint32_t>;
  Poly<T, 8> p;
  std::ostringstream oss(std::ios::binary);
  serialize::write(oss, p);
  std::string s = std::move(oss).str();
  s.pop_back();
  std::istringstream iss(s, std::ios::binary);
  EXPECT_THROW(serialize::read<decltype(p)>(iss), std::out_of_range);
}

// serialized_size<T>() is the exact length of T's raw encoding.
TYPED_TEST(SerializeRoundtripTest, SerializedSizeMatchesEncoding) {
  using Lwe = typename TestFixture::Lwe;
  using Rlwe = typename TestFixture::Rlwe;
  using Decomp = typename TestFixture::Decomp;
  using Kst = typename TestFixture::Kst;
  using Torus = typename Lwe::torus_type;
  using RlweRuntime = Runtime<ParamsPack<Rlwe, Decomp>,
                              RandomGenerator<std::mt19937>, Tracking>;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> lwe_runtime(this->eng_);
  RlweRuntime rlwe_runtime(this->eng_);

  auto ct = lwe_runtime.encrypt(Torus(1u, 2u));
  EXPECT_EQ(serialize::to_bytes(ct).size(),
            serialize::serialized_size<decltype(ct)>());

  auto pk = lwe_runtime.template generate_public_key<Torus, 2 * Lwe::n>();
  EXPECT_EQ(serialize::to_bytes(pk).size(),
            serialize::serialized_size<decltype(pk)>());

  auto bk = rlwe_runtime.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
      lwe_runtime.secret());
  EXPECT_EQ(serialize::to_bytes(bk).size(),
            serialize::serialized_size<decltype(bk)>());

  auto ksk =
      lwe_runtime
          .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
              rlwe_runtime.secret());
  EXPECT_EQ(serialize::to_bytes(ksk).size(),
            serialize::serialized_size<decltype(ksk)>());
}

TEST(SerializeLengthTest, FromBytesRejectsWrongLength) {
  using T = ModTorus<32, uint32_t>;
  TLWE<T, 8> ct;
  auto bytes = serialize::to_bytes(ct);

  EXPECT_THROW((serialize::from_bytes<TLWE<T, 9>>(bytes)),
               serialize::format_error);
  EXPECT_THROW((serialize::from_bytes<TLWE<T, 7>>(bytes)),
               serialize::format_error);

  auto shorter = bytes;
  shorter.pop_back();
  EXPECT_THROW((serialize::from_bytes<TLWE<T, 8>>(shorter)),
               serialize::format_error);

  auto longer = bytes;
  longer.push_back(std::byte{0});
  EXPECT_THROW((serialize::from_bytes<TLWE<T, 8>>(longer)),
               serialize::format_error);
}

// Descriptors are part of the envelope format; pin a few so a renumbered
// tag or reordered dimension fails to compile.
static_assert(serialize::Serde<ModTorus<16, uint32_t>>::descriptor() ==
              std::array<uint32_t, 3>{1, 16, 4});
static_assert(serialize::Serde<dbl::Torus>::descriptor() ==
              std::array<uint32_t, 1>{2});
static_assert(
    serialize::Serde<ModInt<(1ull << 40) + 7, uint64_t>>::descriptor() ==
    std::array<uint32_t, 4>{3, 7, 1u << 8, 8});
static_assert(
    serialize::Serde<TLWE<ModTorus<32, uint32_t>, 630>>::descriptor() ==
    std::array<uint32_t, 5>{32, 630, 1, 32, 4});
static_assert(serialize::Serde<BootstrapKey<ModTorus<32, uint32_t>, 1024, 3,
                                            630>>::descriptor() ==
              std::array<uint32_t, 7>{48, 1024, 3, 630, 1, 32, 4});
static_assert(serialize::Serde<KeySwitchKey<ModTorus<32, uint32_t>, 630, 8,
                                            1024>>::descriptor() ==
              std::array<uint32_t, 7>{49, 630, 8, 1024, 1, 32, 4});
static_assert(serialize::Serde<PublicKey<TLWE<ModTorus<32, uint32_t>, 4>,
                                         8>>::descriptor() ==
              std::array<uint32_t, 7>{50, 8, 32, 4, 1, 32, 4});

namespace {

template <typename T>
std::string enveloped(const T& value) {
  std::ostringstream oss(std::ios::binary);
  serialize::write_enveloped(oss, value);
  return std::move(oss).str();
}

template <typename T>
T read_enveloped_from(const std::string& s) {
  std::istringstream iss(s, std::ios::binary);
  return serialize::read_enveloped<T>(iss);
}

}  // namespace

TYPED_TEST(SerializeRoundtripTest, EnvelopeRoundtrip) {
  using Lwe = typename TestFixture::Lwe;
  using Torus = typename Lwe::torus_type;

  Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking> runtime(this->eng_);
  auto ct = runtime.encrypt(Torus(1u, 2u));

  auto back = read_enveloped_from<decltype(ct)>(enveloped(ct));
  EXPECT_EQ(serialize::to_bytes(back), serialize::to_bytes(ct));
  EXPECT_EQ(runtime.decrypt(back), Torus(1u, 2u));
}

// Stripping the envelope header leaves exactly the raw encoding.
TEST(SerializeEnvelopeTest, BodyIsRawEncoding) {
  using T = ModTorus<32, uint32_t>;
  std::mt19937 rng(0);  // NOLINT(cert-msc51-cpp)
  TLWE<T, 8> ct;
  ct.a() = Vector<T, 8>([&] { return T(static_cast<uint32_t>(rng())); });

  std::string env = enveloped(ct);
  auto raw = serialize::to_bytes(ct);
  constexpr std::size_t header =
      4 + 4 + 4 + 4 * serialize::Serde<TLWE<T, 8>>::descriptor().size();
  ASSERT_EQ(env.size(), header + raw.size());
  EXPECT_EQ(std::memcmp(env.data() + header, raw.data(), raw.size()), 0);
  EXPECT_EQ(env.substr(0, 4), "TQSE");
}

TEST(SerializeEnvelopeTest, RejectsDimensionMismatch) {
  using T = ModTorus<32, uint32_t>;
  TLWE<T, 8> ct;
  std::string env = enveloped(ct);
  EXPECT_THROW((read_enveloped_from<TLWE<T, 9>>(env)), serialize::format_error);
  EXPECT_THROW((read_enveloped_from<TRLWE<T, 8>>(env)),
               serialize::format_error);
}

// Same length, different QBit: invisible to from_bytes, caught here.
TEST(SerializeEnvelopeTest, RejectsSameLengthTorusMismatch) {
  TLWE<ModTorus<32, uint32_t>, 8> ct;
  static_assert(serialize::serialized_size<decltype(ct)>() ==
                serialize::serialized_size<TLWE<ModTorus<16, uint32_t>, 8>>());
  std::string env = enveloped(ct);
  EXPECT_THROW((read_enveloped_from<TLWE<ModTorus<16, uint32_t>, 8>>(env)),
               serialize::format_error);
}

TEST(SerializeEnvelopeTest, RejectsBadMagicAndVersion) {
  using T = ModTorus<32, uint32_t>;
  TLWE<T, 8> ct;

  std::string bad_magic = enveloped(ct);
  bad_magic[0] = 'X';
  EXPECT_THROW((read_enveloped_from<TLWE<T, 8>>(bad_magic)),
               serialize::format_error);

  std::string bad_version = enveloped(ct);
  bad_version[4] = static_cast<char>(2);
  EXPECT_THROW((read_enveloped_from<TLWE<T, 8>>(bad_version)),
               serialize::format_error);
}

// Raw bytes without an envelope are rejected rather than misread.
TEST(SerializeEnvelopeTest, RejectsRawInput) {
  using T = ModTorus<32, uint32_t>;
  TLWE<T, 8> ct;
  std::ostringstream oss(std::ios::binary);
  serialize::write(oss, ct);
  EXPECT_THROW((read_enveloped_from<TLWE<T, 8>>(std::move(oss).str())),
               serialize::format_error);
}
