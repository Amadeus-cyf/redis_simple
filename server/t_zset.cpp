#include "t_zset.h"

#include <vector>

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/t_cmd.h"
#include "server/zset/z_set.h"

namespace redis_simple {
namespace t_cmd {
void zaddCommand(Client* const client) {
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no command\n");
    return;
  }
  const std::vector<std::string>& args = cmd->getArgs();
  if (args.size() < 3) {
    printf("invalid number args\n");
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  const std::string& key = args[0];
  const std::string& ele = args[1];
  double score = 0.0;
  try {
    score = stod(args[2]);
  } catch (const std::exception&) {
    printf("invalid args format\n");
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  const db::RedisObj* obj = db->lookupKey(key);
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  if (!obj) {
    obj = db::RedisObj::createRedisZSetObj(z_set::ZSet::init());
    if (db->setKey(key, obj, 0) == db::DBStatus::dbErr) {
      addReplyToClient(client, reply::fromInt64(-1));
    }
  }
  const z_set::ZSet* const zset = obj->getZSet();
  zset->addOrUpdate(ele, score);
  addReplyToClient(client, reply::fromInt64(1));
}
}  // namespace t_cmd
}  // namespace redis_simple
