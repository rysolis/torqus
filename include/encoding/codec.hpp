#ifndef TFHE_CODEC_HPP
#define TFHE_CODEC_HPP

#include <cassert>
#include <cstdint>
#include <limits>

template <uint32_t PaddingBit = 1>
struct MessageCodec {
  using raw_value_type = uint32_t;
  static constexpr raw_value_type padding_bit = PaddingBit;

  template <typename Torus, typename Message>
  static constexpr Torus encode(Message m) {
    constexpr uint32_t qbit = Torus::qbit;
    constexpr uint32_t MessageBit = Message::message_bit;
    static_assert(MessageBit + PaddingBit <= qbit,
                  "MessageBit + PaddingBit must be less than or equal to QBit");

    constexpr uint32_t noise_bit = qbit - (MessageBit + PaddingBit);

    return Torus(static_cast<typename Torus::raw_value_type>(m.value())
                 << noise_bit);
  }

  template <typename Torus, typename Message>
  static constexpr Message decode(Torus t) {
    constexpr uint32_t qbit = Torus::qbit;
    constexpr uint32_t MessageBit = Message::message_bit;
    static_assert(MessageBit + PaddingBit <= qbit,
                  "MessageBit + PaddingBit must be less than or equal to QBit");

    constexpr uint32_t noise_bit = qbit - (MessageBit + PaddingBit);

    return Message(t.value() >> noise_bit);
  }

  template <typename Torus, typename Message>
  static constexpr Torus::raw_value_type radius_raw() {
    constexpr uint32_t qbit = Torus::qbit;
    constexpr uint32_t MessageBit = Message::message_bit;

    static_assert(MessageBit + PaddingBit <= qbit,
                  "MessageBit + PaddingBit must be less than or equal to QBit");

    // -1 makes diameter to radius
    return (typename Torus::raw_value_type(1)
            << (qbit - (MessageBit + PaddingBit) - 1));
  }

  template <typename Torus, typename Message>
  static constexpr double radius() {
    constexpr uint32_t qbit = Torus::qbit;
    constexpr uint32_t MessageBit = Message::message_bit;

    static_assert(MessageBit + PaddingBit <= qbit,
                  "MessageBit + PaddingBit must be less than or equal to QBit");

    // +1 makes diameter to radius
    return double(1) / (1 << (MessageBit + PaddingBit + 1));
  }
};

#endif