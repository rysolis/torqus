#ifndef TFHE_PRIMITIVE_CONCEPT_TORUS_TYPE_HPP
#define TFHE_PRIMITIVE_CONCEPT_TORUS_TYPE_HPP

#include <concepts>

template <typename Torus>
class TorusBase;

template <typename Torus>
concept torus_type = std::derived_from<std::remove_cvref_t<Torus>,
                                       TorusBase<std::remove_cvref_t<Torus>>>;

#endif
