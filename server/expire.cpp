#include "expire.h"

#include <cassert>

#include "memory/dict.h"
#include "server.h"
#include "utils/time_utils.h"

namespace redis_simple {
void ActiveExpireCycle() {
  auto expire_callback = [](const std::string& key,
                            const int64_t& expire_time) {
    int64_t now = utils::GetNowInMilliseconds();
    if (expire_time < now) {
      RS_LOG_DEBUG("activeExpireCycle: expired key \"%s\" deleted\n",
                   key.c_str());
      if (auto db = Server::Get()->DB().lock()) {
        assert(db->DeleteKey(key) == db::DbStatus::kOk);
      } else {
        RS_LOG_DEBUG("db pointer expired\n");
      }
    }
  };
  if (auto db = Server::Get()->DB().lock()) {
    int64_t start = utils::GetNowInMilliseconds();
    int64_t timelimit = 1000;
    int iteration = 0;
    bool timeout = false;
    while (!timeout && db->ExpiredPercentage() > 0.5) {
      if (!db->ScanExpires(expire_callback)) {
        RS_LOG_DEBUG("expire cycle break\n");
        break;
      }
      ++iteration;
      if ((iteration & 0xf) == 0) {
        int64_t now = utils::GetNowInMilliseconds();
        if (now - start >= timelimit) {
          timeout = true;
        }
      }
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
  }
}
}  // namespace redis_simple
