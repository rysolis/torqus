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
  using Torus = ModTorus<32>;
  using Codec = MessageCodec<1>;
  using Message = MessageWord<4>;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2>;

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
  for (uint32_t m = 0; m <= Message::max_message(); ++m) {
    Message message(m);
    Torus encoded = Codec::template encode<Torus>(message);
    std::cout << "message " << message.value() << ": " << encoded.value()
              << " (binary: " << std::bitset<Torus::qbit>(encoded.value())
              << ")\n";
    Message decoded = Codec::template decode<Torus, Message>(encoded);

    EXPECT_EQ(message, decoded);
  }
}