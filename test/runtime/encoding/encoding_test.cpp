#include <gtest/gtest.h>

#include <bitset>

#include "encoding/codec.hpp"
#include "encoding/message.hpp"
#include "primitive/torus.hpp"

namespace encode_test {
struct Ctx1 {
  using Torus = ModTorus<16>;
  using Codec = MessageCodec<2>;
  using Message = MessageWord<4>;
};

struct Ctx2 {
  using Torus = ModTorus<16>;
  using Codec = MessageCodec<1>;
  using Message = MessageWord<1>;
};

struct Ctx3 {
  using Torus = ModTorus<32>;
  using Codec = MessageCodec<1>;
  using Message = MessageWord<4>;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3>;

}  // namespace encode_test

template <typename Ctx>
class EncodeTest : public ::testing::Test {};

TYPED_TEST_SUITE(EncodeTest, encode_test::TestContexts);

TYPED_TEST(EncodeTest, EncodeDecode) {
  using Ctx = TypeParam;
  using Codec = typename Ctx::Codec;
  using Torus = typename Ctx::Torus;
  using Message = typename Ctx::Message;

  std::cout << "radius(raw) :" << Codec::template radius_raw<Torus, Message>()
            << "\n";
  std::cout << "radius      :" << Codec::template radius<Torus, Message>()
            << "\n";
  for (uint32_t m = 0; m <= Message::max(); ++m) {
    Message message(m);
    Torus encoded = Codec::template encode<Torus>(message);
    std::cout << "message " << message.value() << ": " << encoded.value()
              << " (binary: " << std::bitset<Torus::qbit>(encoded.value())
              << ")\n";
    Message decoded = Codec::template decode<Torus, Message>(encoded);

    EXPECT_EQ(message, decoded);
  }
}

// Check whether 1/8 in the Torus is representable in the message space.
TYPED_TEST(EncodeTest, Constant_One_Over_Eight_Representation) {
  using Ctx = TypeParam;
  using Codec = typename Ctx::Codec;
  using Torus = typename Ctx::Torus;
  using Message = typename Ctx::Message;

  // k = 1/8 in Torus
  assert(Torus::qbit >= 3);

  Torus k(1 << (Torus::qbit - 3));
  Message m = Codec::template decode<Torus, Message>(k);

  std::cout << "\n=== Constant Representation Test ===\n";
  std::cout << "torus: " << k << "\n";
  std::cout << "message: " << m << "\n";
  std::cout << "======================================\n\n";

  if (Codec::padding_bit + Message::message_bit < 3) {
    // Torus encoding effectively cannot represent messages beyond 3 bits,
    // causing the decoded message to collapse to 0.
    EXPECT_EQ(m.value(), 0);
  } else {
    EXPECT_GT(m.value(), 0);
    EXPECT_LE(m.value(), Message::max());
  }
}