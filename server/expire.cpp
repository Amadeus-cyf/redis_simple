#include "expire.h"

#include <cassert>

#include "memory/dict.h"
#include "server.h"
#include "utils/time_utils.h"

namespace redis_simple {
void ActiveExpireCycle() {
  auto expire_callback =
      [](const in_memory::Dict<std::string, int64_t>::DictEntry* de) {
        int64_t now = utils::GetNowInMilliseconds();
        if (de->val < now) {
          printf("activeExpireCycle: key deleted %s\n", de->key.c_str());
          if (std::shared_ptr<const db::RedisDb> db =
                  Server::Get()->DB().lock()) {
            assert(db->DeleteKey(de->key) == db::DBStatus::dbOK);
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
