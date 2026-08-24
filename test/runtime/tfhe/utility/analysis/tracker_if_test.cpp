// Regression test for tracker_if_impl.hpp's per-role tracker slots: each of
// get_noise_tracker_if()/get_key_noise_tracker_if()/get_variance_tracker_if()/
// get_key_variance_tracker_if() used to resolve through one single shared
// global `instance`, so whichever of the four ran first "claimed" it for
// every role -- e.g. registering a ciphertext's variance would silently
// overwrite its worst-case bound if they happened to alias. This only
// exercises the tracker plumbing itself (tracker_if.hpp), not any
// NoisePolicy/VarianceNoisePolicy formula, since the bug being guarded
// against is in the tracker storage, not in any formula.
#include "tfhe/utility/analysis/tracker_if.hpp"
#include <gtest/gtest.h>

#include "primitive/torus.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/analysis/tracker/noise_tracker.hpp"

TEST(TrackerIfTest, EachRoleStoresItsOwnValueForTheSameCiphertext) {
  TLWE<ModTorus<16>, 4> ct;

  // Same ciphertext (same identity()), four different roles, four
  // different values -- if any pair of roles aliased the same underlying
  // map, the later update() would clobber the earlier one's value.
  get_noise_tracker_if()->update(ct, 1.0);
  get_key_noise_tracker_if()->update(ct, 2.0);
  get_variance_tracker_if()->update(ct, 3.0);
  get_key_variance_tracker_if()->update(ct, 4.0);

  EXPECT_DOUBLE_EQ(get_noise_tracker_if()->get(ct), 1.0);
  EXPECT_DOUBLE_EQ(get_key_noise_tracker_if()->get(ct), 2.0);
  EXPECT_DOUBLE_EQ(get_variance_tracker_if()->get(ct), 3.0);
  EXPECT_DOUBLE_EQ(get_key_variance_tracker_if()->get(ct), 4.0);
}

TEST(TrackerIfTest, SetTrackerOverridesOnlyItsOwnRole) {
  NoiseTracker custom;
  TLWE<ModTorus<16>, 4> ct;
  custom.update(ct, 42.0);

  NoiseTrackerInterface* saved_noise = get_noise_tracker_if();
  NoiseTrackerInterface* saved_key_noise = get_key_noise_tracker_if();

  set_noise_tracker_if(&custom);

  EXPECT_EQ(get_noise_tracker_if(), &custom);
  EXPECT_DOUBLE_EQ(get_noise_tracker_if()->get(ct), 42.0);
  // key_noise's own singleton is untouched by overriding noise's.
  EXPECT_EQ(get_key_noise_tracker_if(), saved_key_noise);
  EXPECT_NE(get_key_noise_tracker_if(), &custom);

  set_noise_tracker_if(saved_noise);
  EXPECT_EQ(get_noise_tracker_if(), saved_noise);
}
