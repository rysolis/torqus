#include "tfhe/serialize/secret.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "algebra/utility/randomize.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/serialize.hpp"

namespace {

using Lwe = lwe_params<tlwe_core_params<ModTorus<32>, 20>>;
using Rlwe = rlwe_params<trlwe_core_params<ModTorus<32>, 128>>;
using Decomp = dcp_params<256, 2>;
using Torus = Lwe::torus_type;
using Engine = std::mt19937;
using LweRuntime = Runtime<Lwe, Engine, Tracking>;
using RlweRuntime = Runtime<ParamsPack<Rlwe, Decomp>, Engine, Tracking>;

template <uint32_t N>
bool same_secret(const SecretHolder<N>& a, const SecretHolder<N>& b) {
  return std::equal(a.get(), a.get() + N, b.get());
}

template <uint32_t N>
std::string saved(const SecretHolder<N>& holder) {
  std::ostringstream oss(std::ios::binary);
  serialize::write_secret(oss, holder);
  return std::move(oss).str();
}

template <uint32_t N>
SecretHolder<N> loaded(const std::string& s) {
  std::istringstream iss(s, std::ios::binary);
  return serialize::read_secret<N>(iss);
}

}  // namespace

TEST(SerializeSecretTest, StreamRoundtrip) {
  Engine eng(0);  // NOLINT(cert-msc51-cpp)
  SecretHolder<Lwe::n> holder(eng);

  std::string s = saved(holder);
  EXPECT_EQ(s.size(), serialize::secret_size<Lwe::n>());
  EXPECT_TRUE(same_secret(holder, loaded<Lwe::n>(s)));
}

// Each element is a little-endian u32 0 or 1, and the span overloads
// produce and accept the same bytes as the stream ones.
TEST(SerializeSecretTest, SpanMatchesStreamEncoding) {
  Engine eng(0);  // NOLINT(cert-msc51-cpp)
  SecretHolder<Lwe::n> holder(eng);

  std::vector<std::byte> buf(serialize::secret_size<Lwe::n>());
  serialize::write_secret(std::span<std::byte>(buf), holder);

  std::string s = saved(holder);
  ASSERT_EQ(buf.size(), s.size());
  EXPECT_EQ(std::memcmp(buf.data(), s.data(), s.size()), 0);
  for (uint32_t i = 0; i < Lwe::n; ++i) {
    EXPECT_EQ(buf[4 * i], static_cast<std::byte>(holder.get()[i]));
    EXPECT_EQ(buf[4 * i + 1], std::byte{0});
    EXPECT_EQ(buf[4 * i + 2], std::byte{0});
    EXPECT_EQ(buf[4 * i + 3], std::byte{0});
  }

  auto back = serialize::read_secret<Lwe::n>(std::span<const std::byte>(buf));
  EXPECT_TRUE(same_secret(holder, back));
}

TEST(SerializeSecretTest, RejectsElementOtherThanZeroOrOne) {
  Engine eng(0);  // NOLINT(cert-msc51-cpp)
  SecretHolder<Lwe::n> holder(eng);

  std::string s = saved(holder);
  s[4 * 3] = static_cast<char>(2);
  EXPECT_THROW(loaded<Lwe::n>(s), serialize::format_error);

  std::vector<std::byte> buf(s.size());
  std::memcpy(buf.data(), s.data(), s.size());
  EXPECT_THROW(
      (serialize::read_secret<Lwe::n>(std::span<const std::byte>(buf))),
      serialize::format_error);
}

TEST(SerializeSecretTest, RejectsWrongLength) {
  Engine eng(0);  // NOLINT(cert-msc51-cpp)
  SecretHolder<Lwe::n> holder(eng);

  std::string s = saved(holder);
  s.pop_back();
  EXPECT_THROW(loaded<Lwe::n>(s), std::out_of_range);

  std::vector<std::byte> buf(serialize::secret_size<Lwe::n>() - 1);
  EXPECT_THROW(
      (serialize::read_secret<Lwe::n>(std::span<const std::byte>(buf))),
      serialize::format_error);
  EXPECT_THROW(serialize::write_secret(std::span<std::byte>(buf), holder),
               std::invalid_argument);
}

// A Runtime made from a loaded secret decrypts what the original
// encrypted, for both an Lwe- and an Rlwe-side secret.
TEST(SerializeSecretTest, RuntimeFromLoadedSecretDecrypts) {
  Engine eng(0);  // NOLINT(cert-msc51-cpp)
  LweRuntime lwe(eng);
  RlweRuntime rlwe(eng);

  auto tlwe_ct = lwe.encrypt(Torus(1u, 4u));
  auto pt = randomize<Poly<Torus, Rlwe::N>>(eng);
  auto trlwe_ct = rlwe.encrypt(pt);

  LweRuntime lwe2(loaded<Lwe::n>(saved(lwe.holder())), eng);
  RlweRuntime rlwe2(loaded<Rlwe::N>(saved(rlwe.holder())), eng);

  EXPECT_EQ(lwe2.decrypt(tlwe_ct), lwe.decrypt(tlwe_ct));
  EXPECT_EQ(serialize::to_bytes(rlwe2.decrypt(trlwe_ct)),
            serialize::to_bytes(rlwe.decrypt(trlwe_ct)));

  // ...and what it encrypts, the original decrypts.
  auto ct2 = lwe2.encrypt(Torus(3u, 4u));
  EXPECT_EQ(lwe.decrypt(ct2), lwe2.decrypt(ct2));
}

// The secret stays out of the Serde machinery, so to_bytes and every
// Transport reject it at compile time.
template <typename T>
concept has_serde = requires { serialize::Serde<T>::byte_size(); };
static_assert(!has_serde<SecretHolder<4>>);
static_assert(has_serde<TLWE<Torus, 4>>);
