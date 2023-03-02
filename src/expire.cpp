#include "expire.h"

#include "server.h"
#include "src/memory/dict.h"
#include "src/utils/time_utils.h"

namespace redis_simple {
void activeExpireCycle() {
  auto expire_callback =
      [](const in_memory::Dict<std::string, int64_t>::DictEntry* de) {
        int64_t now = utils::getNowInMilliseconds();
        if (de->val < now) {
          printf("activeExpireCycle: key deleted %s\n", de->key.c_str());
          assert(Server::get()->getDb()->delKey(de->key) == db::DBStatus::dbOK);
        }
      };
  const db::RedisDb* db = Server::get()->getDb();
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
}
}  // namespace redis_simple
