#pragma once

namespace redis_simple {
namespace ae {
class AeTimeEvent {
 public:
  static void FreeAeTimeEvent(const AeTimeEvent* te) { delete te; }
  AeTimeEvent(long long id) : id(id), when(0), next(nullptr), prev(nullptr) {}
  long long Id() { return id; }
  long long Id() const { return id; }
  void SetId(long long tid) { id = tid; }
  void SetNext(AeTimeEvent* _next) { next = _next; }
  void DeleteNext() { next = nullptr; }
  AeTimeEvent* Next() { return next; }
  AeTimeEvent* Next() const { return next; }
  void SetPrev(AeTimeEvent* _prev) { prev = _prev; }
  void DeletePrev() { prev = nullptr; }
  AeTimeEvent* Prev() { return prev; }
  AeTimeEvent* Prev() const { return prev; }
  void SetWhen(time_t _when) { when = _when; }
  time_t When() { return when; }
  time_t When() const { return when; }
  virtual bool HasTimeFinalizeProc() = 0;
  virtual bool HasTimeFinalizeProc() const = 0;
  virtual int CallTimeProc() = 0;
  virtual int CallTimeProc() const = 0;
  virtual int CallTimeFinalizeProc() = 0;
  virtual int CallTimeFinalizeProc() const = 0;
  virtual ~AeTimeEvent() = default;

 private:
  long long id;
  time_t when;
  AeTimeEvent *next, *prev;
};
}  // namespace ae
}  // namespace redis_simple
