#pragma once

namespace redis_simple {
namespace ae {
class AeTimeEvent {
 public:
  static void FreeAeTimeEvent(const AeTimeEvent* te) { delete te; }
  AeTimeEvent(long long id)
      : id_(id), when_(0), next_(nullptr), prev_(nullptr) {}
  long long Id() { return id_; }
  long long Id() const { return id_; }
  void SetId(long long id) { id_ = id; }
  void SetNext(AeTimeEvent* next) { next_ = next; }
  void DeleteNext() { next_ = nullptr; }
  AeTimeEvent* Next() { return next_; }
  AeTimeEvent* Next() const { return next_; }
  void SetPrev(AeTimeEvent* prev) { prev_ = prev; }
  void DeletePrev() { prev_ = nullptr; }
  AeTimeEvent* Prev() { return prev_; }
  AeTimeEvent* Prev() const { return prev_; }
  void SetWhen(time_t when) { when_ = when; }
  time_t When() { return when_; }
  time_t When() const { return when_; }
  virtual bool HasTimeFinalizeProc() = 0;
  virtual bool HasTimeFinalizeProc() const = 0;
  virtual int CallTimeProc() = 0;
  virtual int CallTimeProc() const = 0;
  virtual int CallTimeFinalizeProc() = 0;
  virtual int CallTimeFinalizeProc() const = 0;
  virtual ~AeTimeEvent() = default;

 private:
  long long id_;
  time_t when_;
  AeTimeEvent *next_, *prev_;
};
}  // namespace ae
}  // namespace redis_simple
