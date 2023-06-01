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
Client
```sh
cd build && ./redis_simple_cli
```

## Running Unit Tests
```sh
cd build && ./memory_test
```

### Running Benchmark
```sh
cd build && ./memory_benchmark
```
