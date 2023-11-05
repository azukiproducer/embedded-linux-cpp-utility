#include "EventLoop.h"
#include <iostream>

namespace threadutil {

EventLoop::EventLoop() :
event_queue_(),
timed_queue_(),
runnable_(true)
{
  event_queue_thread_ = std::thread(&EventLoop::EventDispatcher, this);
  timed_queue_thread_ = std::thread(&EventLoop::TimedEventDispatcher, this);
}
EventLoop::~EventLoop() {
  runnable_ = false;

  timed_queue_.m.lock();
  timed_queue_.cv.notify_one();
  timed_queue_.m.unlock();
  timed_queue_thread_.join();

  event_queue_.m.lock();
  event_queue_.cv.notify_one();
  event_queue_.m.unlock();
  event_queue_thread_.join();
}


uint32_t EventLoop::IdleAdd(const EventFunc& func, const std::string& tag) {
  std::unique_lock<std::mutex> lk(event_queue_.m);
  const uint32_t id = ++event_queue_.offset;
  const EventSource source {id, tag, func};
  event_queue_.q.push_back(source);
  event_queue_.cv.notify_one();

  return id;
}


uint32_t EventLoop::TimedAdd(const EventFunc& func, const std::chrono::steady_clock::time_point period, const std::string& tag) {
  uint32_t id;
  {
    std::lock_guard<std::mutex> lock(timed_queue_.m);

    // Sort timed events from earliest to earliest
    auto it = timed_queue_.q.begin();
    while (it != timed_queue_.q.end()) {
      if (period < it->schedlued_point) {
        break;
      }
      it++;
    }

    // The event at the head of queue is waited by Dispatcher.
    // So it is necessary to notify if the head is chaned.
    bool is_head_queue = false;
    if (it == timed_queue_.q.begin()) {
      is_head_queue = true;
    }
    id = timed_queue_.offset++;
    TimedEventSource source = {period, id, tag, func};
    timed_queue_.q.insert(it, source);
    if (is_head_queue) {
      timed_queue_.cv.notify_one();
    }

  }
  return 0;
}


void EventLoop::EventDispatcher() {
  while (runnable_) {
    std::unique_lock<std::mutex> lk(event_queue_.m);
    while (runnable_ && event_queue_.q.empty()) {
      event_queue_.cv.wait(lk);
    }
    if (!runnable_) {
      lk.unlock();
      break;
    }
    const EventFunc f = event_queue_.q.begin()->func; // TODO: confirm to make reference
    event_queue_.q.pop_front();
    lk.unlock();

    (void)f(); // TODO: support return code
  }
}

void EventLoop::TimedEventDispatcher() {
  while (runnable_) {
    std::unique_lock<std::mutex> lk(timed_queue_.m);
    while (runnable_ && timed_queue_.q.empty()) {
      timed_queue_.cv.wait(lk);
    }

    while (runnable_) {
      auto scheduled_point = timed_queue_.q.begin()->schedlued_point;
      if (timed_queue_.cv.wait_until(lk, scheduled_point) == std::cv_status::timeout) {
        TimedEventSource source = *(timed_queue_.q.begin());
        timed_queue_.q.pop_front();
        lk.unlock();
        IdleAdd(source.func, source.tag); // MEMO: it is ok to re-get begin
        break; // go to next
      } else {
        // continue with lock
      }
    }
  }
  //wait;
  //==timeout, do. Else, 

}
}