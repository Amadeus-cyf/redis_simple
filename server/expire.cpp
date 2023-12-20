#include "expire.h"

#include <cassert>

#include "memory/dict.h"
#include "server.h"
#include "utils/time_utils.h"

namespace redis_simple {
void activeExpireCycle() {
  auto expire_callback =
      [](const in_memory::Dict<std::string, int64_t>::DictEntry* de) {
        int64_t now = utils::getNowInMilliseconds();
        if (de->val < now) {
          printf("activeExpireCycle: key deleted %s\n", de->key.c_str());
          if (std::shared_ptr<const db::RedisDb> db =
                  Server::get()->getDb().lock()) {
            assert(db->delKey(de->key) == db::DBStatus::dbOK);
          } else {
            printf("db pointer expired\n");
          }
        }
      };
  if (std::shared_ptr<const db::RedisDb> db = Server::get()->getDb().lock()) {
    int64_t start = utils::getNowInMilliseconds();
    int iteration = 0;
    bool timeout = false;
    int64_t timelimit = 1000;
    while (!timeout && db->getExpiredPercentage() > 0.5) {
      db->scanExpires(expire_callback);
      ++iteration;
      if ((iteration & 0xf) == 0) {
        int64_t now = utils::getNowInMilliseconds();
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
