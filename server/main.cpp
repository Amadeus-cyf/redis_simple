#include "server/server.h"

int main() { redis_simple::Server::get()->run("localhost", 8081); }
