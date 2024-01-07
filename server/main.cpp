#include "server/server.h"

int main() { redis_simple::Server::Get()->Run("localhost", 8080); }
