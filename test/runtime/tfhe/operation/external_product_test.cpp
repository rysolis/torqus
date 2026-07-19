#include "tfhe/operation/external_product.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace external_product_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe, typename Dcp>
struct ParameterSet {
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 8>>,
                          dcp_params<8, 3>>;
using Ctx3 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Rlwe = typename Ctx::context::rlwe_params;
  using Dcp = typename Ctx::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Dcp::l;

  TrackedCryptor<Cryptor<Rlwe>> cryptor_;

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    cryptor_ = TrackedCryptor<Cryptor<Rlwe>>(kr.secret_ptr(), eng_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {
 protected:
  using Base = ExternalProductFixture<Ctx>;

  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  Poly<Torus, N> pt_;
  TRLWE<Torus, N> pt_ct_;

  void SetUp() override {
    Base::SetUp();

    // Prepare plaintext and its ciphertext
    randomize(pt_, this->eng_);
    pt_ct_ = this->cryptor_.encrypt(pt_);
  }
};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Dcp::l;

  // mp means multiplier
  Poly<UInt, N> mp_pt_;
  mp_pt_[0] = UInt(1);

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = negacyclic_convolution(mp_pt_, this->pt_);

  // ==================================
  // TEST LOGIC
  // ==================================
  TRGSW<Torus, N, l> mp_ct =
      trgsw::encrypt<Rlwe, Dcp, Torus>(this->cryptor_, mp_pt_);

  TRLWE<Torus, N> res_ct =
      Evaluator<ExternalProduct<Rlwe, Dcp>, Tracking>::exec(mp_ct,
                                                            this->pt_ct_);
  Poly<Torus, N> res_pt = this->cryptor_.decrypt(res_ct);

  // ----------------------------------
  Poly<Torus, N> err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n=== External Product Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "pt" << ": " << this->pt_ << "\n";
    std::cout << std::setw(14) << "mp" << ": " << mp_pt_ << "\n";

    std::cout << std::setw(14) << "expected" << ": " << ref_pt << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res_pt << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "error_bound" << ": "
            << get_noise_tracker_if()->get(res_ct) << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}
