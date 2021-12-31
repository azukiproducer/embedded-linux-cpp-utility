#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <iostream>
#include <memory>
#include <queue>
#include <functional>
#include <thread>
#include <array>

namespace cpputil
{
constexpr uint8_t NUM_TASKS_DEFAULT = 3;

class Scheduler {
public:
  Scheduler();
  ~Scheduler();
  int8_t Add(const std::function<void()>& cb, uint8_t delay_sec);
  void Cancel(uint8_t index);

private:
  void Init();
  void WorkerFunc(uint8_t index);
  void DummyCallback();

private:
  struct Task {
    bool running;
    enum class Status {Empty, Ready, Working};
    Status status;
    std::chrono::steady_clock::time_point tp;
    std::mutex m;
    std::condition_variable cv;
    std::function<void()> cb;
    // NOTE: The thread will run when a new task is appended.
    // To avoid the behavior that thread() runs by constructor, the type is defined as a pointer.
    std::unique_ptr<std::thread> worker;
  };
  // TODO : Consider to change the type to vector or queue.
  // Cannot assign to vector or queue because copy and move constructor of mutex are deleted. 
  uint8_t numtasks_;
  std::array<Task, NUM_TASKS_DEFAULT> taskList_;
};
} // namespace cpputil
#endif // SCHEDULER_H_
