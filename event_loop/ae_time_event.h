#pragma once

namespace redis_simple {
namespace ae {
class TimeEvent {
 public:
  static void FreeTimeEvent(const TimeEvent* time_event) { delete time_event; }
  TimeEvent(long long id) : id_(id), when_(0), next_(nullptr), prev_(nullptr) {}
  long long Id() const { return id_; }
  void SetId(long long id) { id_ = id; }
  void SetNext(TimeEvent* next) { next_ = next; }
  void DeleteNext() { next_ = nullptr; }
  TimeEvent* Next() const { return next_; }
  void SetPrev(TimeEvent* prev) { prev_ = prev; }
  void DeletePrev() { prev_ = nullptr; }
  TimeEvent* Prev() const { return prev_; }
  void SetWhen(time_t when) { when_ = when; }
  time_t When() const { return when_; }
  virtual bool HasFinalizeCallback() const = 0;
  virtual int CallTimeCallback() const = 0;
  virtual int CallFinalizeCallback() const = 0;
  virtual ~TimeEvent() = default;

 private:
  long long id_;
  time_t when_;
  TimeEvent *next_, *prev_;
};
}  // namespace ae
}  // namespace redis_simple
