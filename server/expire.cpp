#include "expire.h"

#include "logging/logger.h"
#include "server.h"
#include "utils/time_utils.h"

namespace redis_simple {
void ActiveExpireCycle() {
  auto* db = Server::Get()->Db();
  if (db == nullptr) {
    RS_LOG_DEBUG("db unavailable\n");
    return;
  }

  constexpr size_t kSampleCount = 20;
  constexpr int64_t kTimeBudgetMilliseconds = 25;
  constexpr size_t kContinueExpirePercent = 25;
  const int64_t start = utils::NowInMilliseconds();
  while (utils::NowInMilliseconds() - start < kTimeBudgetMilliseconds) {
    const auto sample =
        db->ExpireSome(kSampleCount, utils::NowInMilliseconds());
    if (sample.sampled == 0 ||
        sample.expired * 100 <= sample.sampled * kContinueExpirePercent) {
      break;
    }
  }
}
}  // namespace redis_simple
