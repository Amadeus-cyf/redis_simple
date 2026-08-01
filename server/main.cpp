#include <cstdlib>
#include <iostream>

#include "server/server.h"
#include "server/server_options.h"

int main(int argc, char* argv[]) {
  try {
    const auto result = redis_simple::ParseServerOptions(argc, argv);
    if (result.status == redis_simple::OptionsStatus::kHelp) {
      std::cout << redis_simple::ServerUsage();
      return EXIT_SUCCESS;
    }
    if (result.status == redis_simple::OptionsStatus::kError) {
      std::cerr << "redis_simple: " << result.error << '\n'
                << redis_simple::ServerUsage();
      return EXIT_FAILURE;
    }
    return redis_simple::Server::Get()->Run(result.options.bind_address,
                                            result.options.port)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
  } catch (...) {
    return EXIT_FAILURE;
  }
}
