#include "expire.h"

#include "server.h"
#include "src/memory/dict.h"

namespace redis_simple {
void activeExpireCycle() {
  auto expire_callback =
      [](const in_memory::Dict<std::string, uint64_t>::DictEntry* de) {
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
        if (de->val < now) {
          printf("activeExpireCycle: key deleted %s\n", de->key.c_str());
          assert(Server::get()->getDb()->delKey(de->key) == db::DBStatus::dbOK);
        }
      };
  const db::RedisDb* db = Server::get()->getDb();
  while (db->getExpiredPercentage() > 0.5) {
    db->scanExpires(expire_callback);
  }
}
}  // namespace redis_simple
