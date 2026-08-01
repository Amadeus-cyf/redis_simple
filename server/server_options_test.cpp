#include "server/server_options.h"

#include <gtest/gtest.h>

#include <array>

namespace redis_simple {
TEST(ServerOptionsTest, UsesDefaults) {
  constexpr std::array kArgv = {"redis_simple"};

  const auto result = ParseServerOptions(kArgv.size(), kArgv.data());

  EXPECT_EQ(result.status, OptionsStatus::kOk);
  EXPECT_EQ(result.options.bind_address, "127.0.0.1");
  EXPECT_EQ(result.options.port, 8080);
}

TEST(ServerOptionsTest, ParsesBindAddressAndPort) {
  constexpr std::array kArgv = {"redis_simple", "--bind", "0.0.0.0", "--port",
                                "6380"};

  const auto result = ParseServerOptions(kArgv.size(), kArgv.data());

  EXPECT_EQ(result.status, OptionsStatus::kOk);
  EXPECT_EQ(result.options.bind_address, "0.0.0.0");
  EXPECT_EQ(result.options.port, 6380);
}

TEST(ServerOptionsTest, HandlesHelpAndInvalidArguments) {
  constexpr std::array kHelp = {"redis_simple", "--help"};
  EXPECT_EQ(ParseServerOptions(kHelp.size(), kHelp.data()).status,
            OptionsStatus::kHelp);

  constexpr std::array kInvalidPort = {"redis_simple", "--port", "65536"};
  EXPECT_EQ(
      ParseServerOptions(kInvalidPort.size(), kInvalidPort.data()).status,
            OptionsStatus::kError);

  constexpr std::array kUnknown = {"redis_simple", "--unknown"};
  EXPECT_EQ(ParseServerOptions(kUnknown.size(), kUnknown.data()).status,
            OptionsStatus::kError);

  constexpr std::array kMissing = {"redis_simple", "--port"};
  EXPECT_EQ(ParseServerOptions(kMissing.size(), kMissing.data()).status,
            OptionsStatus::kError);
}
}  // namespace redis_simple
