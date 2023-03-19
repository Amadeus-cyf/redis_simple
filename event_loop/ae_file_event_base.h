#pragma once

namespace redis_simple {
namespace ae {
class AeEventLoop;

class BaseAeFileEvent {
 public:
  BaseAeFileEvent(int mask) : mask(mask) {}
  int getMask() { return mask; }
  int getMask() const { return mask; }
  void setMask(int m) { mask |= m; }
  virtual void callReadProc(AeEventLoop* el, int fd) = 0;
  virtual void callReadProc(AeEventLoop* el, int fd) const = 0;
  virtual void callWriteProc(AeEventLoop* el, int fd) = 0;
  virtual void callWriteProc(AeEventLoop* el, int fd) const = 0;
  virtual bool hasRFileProc() = 0;
  virtual bool hasRFileProc() const = 0;
  virtual bool hasWFileProc() = 0;
  virtual bool hasWFileProc() const = 0;
  virtual bool isRWProcDiff() = 0;
  virtual bool isRWProcDiff() const = 0;
  virtual ~BaseAeFileEvent() = default;

 private:
  int mask;
};
}  // namespace ae
}  // namespace redis_simple
