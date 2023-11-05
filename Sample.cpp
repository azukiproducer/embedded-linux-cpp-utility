#include <cstdio>
#include <iostream>
#include <chrono>
#include "EventLoop.h"

#define LOG(_logbuf) std::cout << __func__ << " "<< (_logbuf) << std::endl
#define LOG_HEADER LOGHeaderImpl(__func__)

std::string LOGHeaderImpl(const char* func_name) {
  // Header
  auto tp = std::chrono::steady_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp);

  char header[128];
  std::snprintf(header, sizeof(header), "[%lld.%03lld] %s:", (ms.count() / 1000), (ms.count() % 1000), func_name);
  return std::string(header);
}

void LOGimpl(const char* func_name) {
  ;
}

using namespace threadutil;
using namespace std::chrono;

uint8_t TaskA(int32_t arg1, int32_t arg2) {
  std::printf("%s arg1=%d arg2=%d\n", LOG_HEADER.c_str(), arg1, arg2);
  return EVENT_ONESHOT;
}

uint8_t TaskB(int32_t arg1, int32_t arg2) {
  std::printf("%s arg1=%d arg2=%d\n", LOG_HEADER.c_str(), arg1, arg2);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return EVENT_ONESHOT;
}


int main(void) {
  std::printf("%s enter\n", LOG_HEADER.c_str());
  EventLoop ev;

  // event is fired after 1 second.
  steady_clock::time_point tp = steady_clock::now();
  ev.TimedAdd( std::bind(&TaskA, 1, 10), tp + seconds(1), "tag1");
  std::this_thread::sleep_for(std::chrono::seconds(2));
  
  // multiple events are sorted by time. second queued event is fired first.
  tp = steady_clock::now();
  ev.TimedAdd( std::bind(&TaskA, 2, 20), tp + seconds(2), "tag2");
  ev.TimedAdd( std::bind(&TaskA, 3, 30), tp + seconds(1), "tag3");
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // event is removed.
  tp = steady_clock::now();
  ev.TimedAdd( std::bind(&TaskA, 4, 40), tp + seconds(1), "tag4");
  ev.RemoveByTag("tag4");
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // already running event is not removed.
  ev.IdleAdd( std::bind(&TaskB, 5, 50), "tag5");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  ev.IdleAdd( std::bind(&TaskB, 6, 60), "tag6");
  ev.RemoveByTag("tag5");
  ev.RemoveByTag("tag6");
  std::this_thread::sleep_for(std::chrono::seconds(3));

  return 0;
}