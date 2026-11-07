#include "tfhe/transport.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <type_traits>

#include "primitive/torus.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace {

using Lwe = lwe_params<tlwe_core_params<ModTorus<16>, 4>>;
using Torus = Lwe::torus_type;
using LweRuntime = Runtime<Lwe, RandomGenerator<std::mt19937>, Tracking>;

}  // namespace

template <typename Transport>
class TransportTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};
};

using Transports =
    ::testing::Types<DirectTransport, SerializedTransport, EnvelopedTransport>;
TYPED_TEST_SUITE(TransportTest, Transports);

// A value handed across any Transport decrypts to what was encrypted.
TYPED_TEST(TransportTest, CiphertextRoundtrip) {
  LweRuntime runtime(this->eng_);
  auto ct = runtime.encrypt(Torus(1u, 2u));
  using Ct = decltype(ct);

  typename TypeParam::template wire_type<Ct> wire = TypeParam::send(ct);
  Ct back = TypeParam::template receive<Ct>(wire);

  EXPECT_EQ(runtime.decrypt(back), Torus(1u, 2u));
}

TYPED_TEST(TransportTest, PublicKeyRoundtrip) {
  LweRuntime runtime(this->eng_);
  auto pk = runtime.template generate_public_key<Torus, 2 * Lwe::n>();
  using Pk = decltype(pk);

  Pk back = TypeParam::template receive<Pk>(TypeParam::send(pk));
  EXPECT_EQ(serialize::to_bytes(back), serialize::to_bytes(pk));
}

static_assert(std::is_same_v<SerializedTransport::wire_type<int>,
                             std::vector<std::byte>>);
static_assert(
    std::is_same_v<EnvelopedTransport::wire_type<int>, std::vector<std::byte>>);

// EnvelopedTransport's wire bytes are the envelope; SerializedTransport's
// are the raw format.
TEST(EnvelopedTransportTest, WireIsEnvelopeAroundRawFormat) {
  TLWE<Torus, Lwe::n> ct;
  auto raw = SerializedTransport::send(ct);
  auto env = EnvelopedTransport::send(ct);

  EXPECT_EQ(env, serialize::to_bytes_enveloped(ct));
  EXPECT_EQ(env.size(), serialize::enveloped_size<decltype(ct)>());
  ASSERT_GT(env.size(), raw.size());
  EXPECT_TRUE(std::equal(raw.rbegin(), raw.rend(), env.rbegin()));
}

// Same length, different QBit: SerializedTransport can't tell, while
// EnvelopedTransport rejects it.
TEST(EnvelopedTransportTest, RejectsBytesSentAsAnotherType) {
  TLWE<ModTorus<32, uint32_t>, 8> ct;
  using Other = TLWE<ModTorus<16, uint32_t>, 8>;

  EXPECT_NO_THROW(
      SerializedTransport::receive<Other>(SerializedTransport::send(ct)));
  EXPECT_THROW(EnvelopedTransport::receive<Other>(EnvelopedTransport::send(ct)),
               serialize::format_error);
}

TEST(EnvelopedTransportTest, RejectsRawAndTruncatedBytes) {
  TLWE<Torus, Lwe::n> ct;
  using Ct = decltype(ct);

  EXPECT_THROW(EnvelopedTransport::receive<Ct>(SerializedTransport::send(ct)),
               serialize::format_error);

  auto env = EnvelopedTransport::send(ct);
  env.pop_back();
  EXPECT_THROW(EnvelopedTransport::receive<Ct>(env), serialize::format_error);
}
