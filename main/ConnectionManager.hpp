#pragma once
#include "VslinkParser.hpp"
#include <array>
#include <cstring>
namespace vslink {
// Externally synchronize all calls. Never allocates. Reset bypasses a full queue.
template<size_t Capacity> class ConnectionManager {
 public:
  static_assert(Capacity > 0, "Queue must have capacity");
  struct Event { std::array<char,513> data{}; size_t length=0; };
  bool enqueue(const char *data, size_t length) {
    if (!connected_ || !length || length > VslinkParser::MaxCommand) return false;
    if (count_ == Capacity) { recover(false); ++overflows_; return false; }
    auto &e=queue_[(head_+count_) % Capacity];
    std::memcpy(e.data.data(),data,length); e.length=length; ++count_; return true;
  }
  bool pop(Event &e) {
    if (!count_ || reset_pending_) return false;
    e=queue_[head_]; head_=(head_+1)%Capacity; --count_; return true;
  }
  void connect() { connected_=true; recover(true); }
  void disconnect() { connected_=false; recover(true); }
  bool take_reset(bool &new_connection) {
    if (!reset_pending_) return false;
    new_connection=new_connection_; reset_pending_=false; new_connection_=false; return true;
  }
  unsigned generation() const { return generation_; }
  unsigned overflows() const { return overflows_; }
 private:
  void recover(bool new_connection) {
    head_=count_=0; reset_pending_=true; new_connection_ |= new_connection; ++generation_;
  }
  std::array<Event,Capacity> queue_{};
  size_t head_=0,count_=0;
  unsigned generation_=0,overflows_=0;
  bool connected_=false,reset_pending_=true,new_connection_=true;
};
}
