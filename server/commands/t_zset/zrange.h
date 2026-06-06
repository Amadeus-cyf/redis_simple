#pragma once

#include "server/commands/command.h"
#include "server/db/db.h"
#include "server/db/redis_obj.h"
#include "storage/zset/zset.h"

namespace redis_simple {
namespace command {
namespace t_zset {
class ZRangeCommand : public Command {
 public:
  ZRangeCommand() : Command("ZRANGE"){};
  void Exec(Client* const client) const override;

 private:
  int RangeByRank(Client* const client, const std::vector<std::string>& args,
                  std::vector<const zset::ZSetEntry*>* result) const;
  int RangeByScore(Client* const client, const std::vector<std::string>& args,
                   std::vector<const zset::ZSetEntry*>* result) const;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
