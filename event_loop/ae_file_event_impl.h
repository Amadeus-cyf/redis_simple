#pragma once

#include "event_loop/ae.h"

namespace redis_simple {
namespace ae {
class EventLoop;

template <typename T>
class FileEventImpl : public FileEvent {
 public:
  using FileCallback = EventHandlerStatus (*)(EventLoop* el, int fd,
                                              T* client_data_, int mask);
  static FileEvent* Create(FileCallback read_callback_,
                           FileCallback write_callback_, T* client_data_,
                           int mask) {
    return new FileEventImpl(read_callback_, write_callback_, client_data_,
                             mask);
  }
  static FileEvent* Create(FileCallback read_callback_,
                           FileCallback write_callback_, T* client_data_,
                           EventFlag mask) {
    return Create(read_callback_, write_callback_, client_data_, ToInt(mask));
  }
  void CallReadCallback(EventLoop* el, int fd, int mask) const override {
    read_callback_(el, fd, client_data_, mask);
  }
  void CallWriteCallback(EventLoop* el, int fd, int mask) const override {
    write_callback_(el, fd, client_data_, mask);
  }
  bool HasReadCallback() const override { return read_callback_ != nullptr; }
  bool HasWriteCallback() const override { return write_callback_ != nullptr; }
  bool HasSeparateReadWriteCallbacks() const override {
    return read_callback_ != write_callback_;
  }
  void Merge(const FileEvent* file_event) override;
  FileCallback ReadCallback() const { return read_callback_; }
  void SetReadCallback(FileCallback p) { read_callback_ = p; }
  FileCallback WriteCallback() const { return write_callback_; }
  void SetWriteCallback(FileCallback p) { write_callback_ = p; }
  T* ClientData() const { return client_data_; }
  void ClientData(const T* data) { client_data_ = data; }

 private:
  explicit FileEventImpl(FileCallback read_callback_,
                         FileCallback write_callback_, T* client_data_,
                         int mask)
      : read_callback_(read_callback_),
        write_callback_(write_callback_),
        client_data_(client_data_),
        FileEvent(mask) {}
  FileCallback read_callback_;
  FileCallback write_callback_;
  T* client_data_;
};

template <typename T>
void FileEventImpl<T>::Merge(const FileEvent* file_event) {
  if (!file_event) {
    return;
  }
  const FileEventImpl<T>* file_event_impl =
      static_cast<const FileEventImpl<T>*>(file_event);
  AddMask(file_event->Mask());
  if (!HasReadCallback() && file_event->HasReadCallback()) {
    read_callback_ = file_event_impl->ReadCallback();
  }
  if (!HasWriteCallback() && file_event->HasWriteCallback()) {
    write_callback_ = file_event_impl->WriteCallback();
  }
}
}  // namespace ae
}  // namespace redis_simple
