#include <cstdlib>

#include "server/server.h"

int main() {
  try {
    return redis_simple::Server::Get()->Run("localhost", 8080) ? EXIT_SUCCESS
                                                               : EXIT_FAILURE;
  } catch (...) {
    return EXIT_FAILURE;
  }
}
