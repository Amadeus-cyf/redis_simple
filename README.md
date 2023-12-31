# redis_simple
Simple redis in C++.

## Building
```sh
mkdir build && cd build
cmake .. && cmake --build .
```

## Running
Server
```sh
cd build && ./redis_simple
```

### Running Tests
### Unit Tests
```sh
cd build && ./redis_simple_tests
```

### Mock Test
#### Mock Client-Server
```sh
cd build && ./mock_server
```
```sh
cd build && ./mock_client
```

#### Mock Event Loop
```sh
cd build && ./mock_event_loop_server
```
```sh
cd build && ./mock_event_loop_client
```

#### Mock TCP client-server
```sh
cd build && ./mock_tcp_server
```
```sh
cd build && ./mock_tcp_client
```

#### Mock Commands
Server
```sh
cd build && ./mock_command_server
```
Clients
```sh
cd build && ./mock_t_string_client
```
```sh
cd build && ./mock_t_zset_client
```

### Running Benchmark
```sh
cd build && ./memory_benchmark
```
