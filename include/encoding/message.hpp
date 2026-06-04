#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstdint>

template <uint32_t MessageBit>
class MessageWord {
 public:
  using raw_value_type = uint32_t;
  static constexpr raw_value_type message_bit = MessageBit;

  explicit MessageWord(raw_value_type v) : value_(v) {
    assert(v <= max_message());
  }

  static constexpr raw_value_type mask() noexcept {
    return std::numeric_limits<raw_value_type>::max() >>
           (std::numeric_limits<raw_value_type>::digits - message_bit);
  }

  static constexpr raw_value_type max_message() noexcept {
    return static_cast<raw_value_type>(-1) & mask();
  }

  raw_value_type value() const { return value_; }

  constexpr bool operator==(const MessageWord& other) const noexcept {
    return value_ == other.value_;
  }

 private:
  raw_value_type value_;
};

#endif