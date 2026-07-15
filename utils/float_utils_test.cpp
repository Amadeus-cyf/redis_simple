#include "utils/float_utils.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace redis_simple::utils {
TEST(FloatUtilsTest, FormatsCommonValuesConcise) {
  EXPECT_EQ(FloatToString(0.0001), "0.0001");
  EXPECT_EQ(FloatToString(1.5), "1.5");
  EXPECT_EQ(FloatToString(99999.25), "99999.25");
  EXPECT_EQ(FloatToString(100000.0), "100000");
}

TEST(FloatUtilsTest, PreservesDoubleValuesExactly) {
  const std::array values = {
      0.00001,
      1.0000234,
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::max(),
      std::nextafter(1.0, 2.0),
  };
  for (const double value : values) {
    double parsed = 0;
    const std::string text = FloatToString(value);
    ASSERT_TRUE(ToDouble(text, &parsed)) << text;
    EXPECT_EQ(parsed, value) << text;
  }
}

TEST(FloatUtilsTest, ParsesOnlyCompleteFiniteOrInfiniteNumbers) {
  double value = 0;
  EXPECT_TRUE(ToDouble("-1.25e+3", &value));
  EXPECT_EQ(value, -1250);
  EXPECT_TRUE(ToDouble("+1.5", &value));
  EXPECT_EQ(value, 1.5);
  EXPECT_TRUE(ToDouble("+inf", &value));
  EXPECT_TRUE(std::isinf(value));
  EXPECT_FALSE(ToDouble("nan", &value));
  EXPECT_FALSE(ToDouble("1.5junk", &value));
  EXPECT_FALSE(ToDouble(" 1.5", &value));
  EXPECT_FALSE(ToDouble("1.5 ", &value));
  EXPECT_FALSE(ToDouble("0x1p2", &value));
  EXPECT_FALSE(ToDouble("", &value));
}
}  // namespace redis_simple::utils
