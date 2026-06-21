#ifndef TFHE_CONCEPTS_HPP
#define TFHE_CONCEPTS_HPP

#include <concepts>
#include <cstdint>

template <typename Frontend, typename Backend>
concept blindrotate_params = requires {
  typename Frontend::Torus;
  typename Backend::Torus;
  { Frontend::n } -> std::convertible_to<uint32_t>;
  { Backend::N } -> std::convertible_to<uint32_t>;
  { Backend::B } -> std::convertible_to<uint32_t>;
  { Backend::l } -> std::convertible_to<uint32_t>;
};

template <typename Backend>
concept decompose_params = requires {
  { Backend::N } -> std::convertible_to<uint32_t>;
  { Backend::B } -> std::convertible_to<uint32_t>;
  { Backend::l } -> std::convertible_to<uint32_t>;
};

template <typename Frontend>
concept tlwe_encrypt_params = requires {
  typename Frontend::Torus;
  { Frontend::n } -> std::convertible_to<uint32_t>;
};

template <typename Backend>
concept trlwe_encrypt_params = requires {
  typename Backend::Torus;
  { Backend::N } -> std::convertible_to<uint32_t>;
};

template <typename Backend>
concept trgsw_encrypt_params = requires {
  typename Backend::Torus;
  { Backend::N } -> std::convertible_to<uint32_t>;
  { Backend::B } -> std::convertible_to<uint32_t>;
  { Backend::l } -> std::convertible_to<uint32_t>;
};

#endif