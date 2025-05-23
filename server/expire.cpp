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
      printf("activeExpireCycle: expired key \"%s\" deleted\n", key.c_str());
      if (std::shared_ptr<const db::RedisDb> db = Server::Get()->DB().lock()) {
        assert(db->DeleteKey(key) == db::DBStatus::dbOK);
      } else {
        printf("db pointer expired\n");
      }
    }
  };
  if (std::shared_ptr<const db::RedisDb> db = Server::Get()->DB().lock()) {
    int64_t start = utils::GetNowInMilliseconds(), timelimit = 1000;
    int iteration = 0;
    bool timeout = false;
    while (!timeout && db->ExpiredPercentage() > 0.5) {
      if (!db->ScanExpires(expire_callback)) {
        printf("expire cycle break\n");
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
    printf("db pointer expired\n");
  }
}
}  // namespace redis_simple
