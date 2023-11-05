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
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return EVENT_ONESHOT;
}

int main(void) {
  std::printf("%s enter\n", LOG_HEADER.c_str());
  EventLoop ev;

  steady_clock::time_point tp = steady_clock::now() + seconds(3);
  ev.TimedAdd( std::bind(&TaskA, 10, 10), tp + seconds(1), "tag1");
  ev.TimedAdd( std::bind(&TaskA, 20, 20), tp + seconds(4), "tag2");
  ev.TimedAdd( std::bind(&TaskA, 30, 30), tp + seconds(2), "tag3");

  std::this_thread::sleep_for(std::chrono::seconds(10));
  return 0;
}