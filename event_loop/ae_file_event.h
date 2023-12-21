#pragma once
#include <memory>

namespace redis_simple {
namespace ae {
class AeEventLoop;

class AeFileEvent {
 public:
  AeFileEvent(int mask) : mask_(mask) {}
  int GetMask() { return mask_; }
  int GetMask() const { return mask_; }
  void SetMask(int mask) { mask_ |= mask; }
  virtual void CallReadProc(AeEventLoop* el, int fd, int mask) = 0;
  virtual void CallReadProc(AeEventLoop* el, int fd, int mask) const = 0;
  virtual void CallWriteProc(AeEventLoop* el, int fd, int mask) = 0;
  virtual void CallWriteProc(AeEventLoop* el, int fd, int mask) const = 0;
  virtual bool HasRFileProc() = 0;
  virtual bool HasRFileProc() const = 0;
  virtual bool HasWFileProc() = 0;
  virtual bool HasWFileProc() const = 0;
  virtual bool IsRWProcDiff() = 0;
  virtual bool IsRWProcDiff() const = 0;
  virtual void Merge(const AeFileEvent* fe) = 0;
  virtual ~AeFileEvent() = default;

 private:
  int mask_;
};
}  // namespace ae
}  // namespace redis_simple
