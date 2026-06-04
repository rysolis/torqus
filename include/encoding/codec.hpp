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
  static constexpr Message decode(Torus phase) {
    constexpr uint32_t qbit = Torus::qbit;
    constexpr uint32_t MessageBit = Message::message_bit;
    static_assert(MessageBit + PaddingBit <= qbit,
                  "MessageBit + PaddingBit must be less than or equal to QBit");

    constexpr uint32_t noise_bit = qbit - (MessageBit + PaddingBit);

    constexpr uint32_t rounding =
        noise_bit == 0 ? 0 : (uint32_t{1} << (noise_bit - 1));

    return Message((phase.value() + rounding) >> noise_bit);
  }
};

#endif