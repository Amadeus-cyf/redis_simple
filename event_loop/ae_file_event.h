#pragma once

namespace redis_simple {
namespace ae {
class AeEventLoop;

class AeFileEvent {
 public:
  AeFileEvent(int mask) : mask(mask) {}
  int getMask() { return mask; }
  int getMask() const { return mask; }
  void setMask(int _mask) { mask |= _mask; }
  virtual void callReadProc(AeEventLoop* el, int fd, int mask) = 0;
  virtual void callReadProc(AeEventLoop* el, int fd, int mask) const = 0;
  virtual void callWriteProc(AeEventLoop* el, int fd, int mask) = 0;
  virtual void callWriteProc(AeEventLoop* el, int fd, int mask) const = 0;
  virtual bool hasRFileProc() = 0;
  virtual bool hasRFileProc() const = 0;
  virtual bool hasWFileProc() = 0;
  virtual bool hasWFileProc() const = 0;
  virtual bool isRWProcDiff() = 0;
  virtual bool isRWProcDiff() const = 0;
  virtual void merge(const AeFileEvent* fe) = 0;
  virtual ~AeFileEvent() = default;

 private:
  int mask;
};
}  // namespace ae
}  // namespace redis_simple
