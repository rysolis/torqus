#ifndef PARAMS_HPP
#define PARAMS_HPP

#include <concepts>
#include <cstdint>

template <typename Params>
concept decompose_params = requires {
  { Params::N } -> std::convertible_to<uint32_t>;
  { Params::B } -> std::convertible_to<uint32_t>;
  { Params::Bbit } -> std::convertible_to<uint32_t>;
  { Params::l } -> std::convertible_to<uint32_t>;
};

template <typename Params>
concept trlwe_encrypt_params = requires {
  { Params::N } -> std::convertible_to<uint32_t>;
};

template <typename Params>
concept trgsw_encrypt_params = requires {
  { Params::N } -> std::convertible_to<uint32_t>;
  { Params::B } -> std::convertible_to<uint32_t>;
  { Params::l } -> std::convertible_to<uint32_t>;
};

#endif