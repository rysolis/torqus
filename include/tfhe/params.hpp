#ifndef TFHE_PARAMS_HPP
#define TFHE_PARAMS_HPP

#include <cstdint>

template <typename Torus, uint32_t n_>
struct tlwe_core_params {
  using torus_type = Torus;
  static constexpr uint32_t n = n_;
};

template <typename Torus, uint32_t N_>
struct trlwe_core_params {
  using torus_type = Torus;
  static constexpr uint32_t N = N_;
};

template <uint32_t B_, uint32_t l_>
struct gadget_params {
  static constexpr uint32_t B = B_;
  static constexpr uint32_t l = l_;
};

template <typename Core, typename... Features>
struct lwe_params : Core, Features... {};

template <typename Core, typename... Features>
struct glwe_params : Core, Features... {};

template <typename Message, typename Codec>
struct encoding_params {
  using message_type = Message;
  using codec_type = Codec;
};

#endif