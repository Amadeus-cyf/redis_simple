#include "storage/list/list.h"

namespace redis_simple {
namespace list {
List::List() : quicklist_(std::make_unique<in_memory::QuickList>()) {}

bool List::LPush(const std::string& value) { return quicklist_->LPush(value); }

bool List::RPush(const std::string& value) { return quicklist_->RPush(value); }

std::optional<std::string> List::RPop() { return quicklist_->RPop(); }

std::optional<std::string> List::LPop() { return quicklist_->LPop(); }
}  // namespace list
}  // namespace redis_simple
