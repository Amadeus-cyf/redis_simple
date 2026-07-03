#include "utils/float_utils.h"

#include <gtest/gtest.h>

namespace redis_simple::utils {
TEST(FloatUtilsTest, FormatsFixedRangeValues) {
  EXPECT_EQ(FloatToString(0.0001), "0.000100000000000");
  EXPECT_EQ(FloatToString(1.5), "1.500000000000000");
  EXPECT_EQ(FloatToString(99999.25), "99999.250000000000000");
}

TEST(FloatUtilsTest, FormatsSmallAndLargeValuesScientifically) {
  EXPECT_EQ(FloatToString(0.00001), "1.000000000000000e-05");
  EXPECT_EQ(FloatToString(100000.0), "1.000000000000000e+05");
}
}  // namespace redis_simple::utils
