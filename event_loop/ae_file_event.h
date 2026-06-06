#pragma once

namespace redis_simple {
namespace ae {
class EventLoop;

class FileEvent {
 public:
  FileEvent(int mask) : mask_(mask) {}
  int Mask() const { return mask_; }
  void SetMask(int mask) { mask_ = mask; }
  void AddMask(int mask) { mask_ |= mask; }
  virtual void CallReadCallback(EventLoop* el, int fd, int mask) const = 0;
  virtual void CallWriteCallback(EventLoop* el, int fd, int mask) const = 0;
  virtual bool HasReadCallback() const = 0;
  virtual bool HasWriteCallback() const = 0;
  virtual bool HasSeparateReadWriteCallbacks() const = 0;
  virtual void Merge(const FileEvent* file_event) = 0;
  virtual ~FileEvent() = default;

 private:
  int mask_;
};
}  // namespace ae
}  // namespace redis_simple
