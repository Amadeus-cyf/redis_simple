#pragma once

namespace redis_simple {
namespace ae {
class AeTimeEvent {
 public:
  static void freeAeTimeEvent(const AeTimeEvent* te) { delete te; }
  AeTimeEvent(long long id) : id(id), when(0), next(nullptr), prev(nullptr) {}
  long long getId() { return id; }
  long long getId() const { return id; }
  void setId(long long tid) { id = tid; }
  void setNext(AeTimeEvent* _next) { next = _next; }
  void delNext() { next = nullptr; }
  AeTimeEvent* getNext() { return next; }
  AeTimeEvent* getNext() const { return next; }
  void setPrev(AeTimeEvent* _prev) { prev = _prev; }
  void delPrev() { prev = nullptr; }
  AeTimeEvent* getPrev() { return prev; }
  AeTimeEvent* getPrev() const { return prev; }
  void setWhen(time_t _when) { when = _when; }
  time_t getWhen() { return when; }
  time_t getWhen() const { return when; }
  virtual bool hasTimeFinalizeProc() = 0;
  virtual bool hasTimeFinalizeProc() const = 0;
  virtual int callTimeProc() = 0;
  virtual int callTimeProc() const = 0;
  virtual int callTimeFinalizeProc() = 0;
  virtual int callTimeFinalizeProc() const = 0;
  virtual ~AeTimeEvent() = default;

 private:
  long long id;
  time_t when;
  AeTimeEvent *next, *prev;
};
}  // namespace ae
}  // namespace redis_simple
