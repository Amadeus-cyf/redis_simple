#include "expire.h"

#include "server.h"
#include "src/memory/dict.h"
#include "src/utils/time_utils.h"

namespace redis_simple {
void activeExpireCycle() {
  auto expire_callback =
      [](const in_memory::Dict<std::string, uint64_t>::DictEntry* de) {
        uint64_t now = utils::getNowInMilliseconds();
        if (de->val < now) {
          printf("activeExpireCycle: key deleted %s\n", de->key.c_str());
          assert(Server::get()->getDb()->delKey(de->key) == db::DBStatus::dbOK);
        }
      };
  const db::RedisDb* db = Server::get()->getDb();
  uint64_t start = utils::getNowInMilliseconds();
  int iteration = 0;
  bool timeout = false;
  uint64_t timelimit = 1000;
  while (!timeout && db->getExpiredPercentage() > 0.5) {
    db->scanExpires(expire_callback);
    ++iteration;
    if ((iteration & 0xf) == 0) {
      uint64_t now = utils::getNowInMilliseconds();
      if (now - start >= timelimit) {
        printf("timediff %llu %llu %llu\n", now, start, now - start);
        timeout = true;
      }
    }
  }
}
}  // namespace redis_simple
