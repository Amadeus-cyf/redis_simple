#include "server/client.h"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <memory>
#include <string>
#include <utility>

#include "tcp/tcp.h"

namespace redis_simple {
namespace {
class ScopedFd {
 public:
  explicit ScopedFd(int fd = -1) : fd_(fd) {}
  ScopedFd(const ScopedFd&) = delete;
  ScopedFd& operator=(const ScopedFd&) = delete;
  ScopedFd(ScopedFd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
  ScopedFd& operator=(ScopedFd&&) = delete;
  ~ScopedFd() {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  [[nodiscard]] int Get() const { return fd_; }
  int Release() { return std::exchange(fd_, -1); }

 private:
  int fd_;
};

std::array<ScopedFd, 2> CreateSocketPair() {
  std::array<int, 2> fds{-1, -1};
  EXPECT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds.data()), 0);
  return {ScopedFd(fds[0]), ScopedFd(fds[1])};
}
}  // namespace

TEST(ClientTest, ReadQueryDrainsReadySocket) {
  auto sockets = CreateSocketPair();
  ASSERT_EQ(tcp::NonBlock(sockets[0].Get()), tcp::TcpStatusCode::kOk);
  auto connection = std::make_unique<connection::Connection>(
      connection::Context{nullptr, sockets[0].Release()});
  connection->SetState(connection::ConnectionState::kConnected);
  auto client = Client::Create(std::move(connection));

  const std::string query(8192, 'x');
  ASSERT_EQ(write(sockets[1].Get(), query.data(), query.size()),
            static_cast<ssize_t>(query.size()));

  EXPECT_EQ(client->ReadQuery(), static_cast<ssize_t>(query.size()));
  EXPECT_EQ(client->ReadQuery(), 0);
  EXPECT_EQ(client->Connection()->State(),
            connection::ConnectionState::kConnected);
}
}  // namespace redis_simple
