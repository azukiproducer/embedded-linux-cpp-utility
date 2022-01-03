#include <functional>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include "DelayWorker.h"

namespace cpputil
{
#define LOG(_logbuf) std::cout << __func__ << " "<< (_logbuf) << std::endl

class CommandWorker {
public:
  CommandWorker();
  ~CommandWorker();
  uint32_t Add(const std::function<void()>& commandfunc);
  uint32_t Add(const std::function<void()>& commandfunc, uint32_t id);
  uint32_t DelayAdd(const std::function<void()>& commandfunc, uint8_t delay_sec);
  void Remove(uint32_t id);

private:
  void Run();
  void DelayCallback(const std::function<void()> commandfunc, uint32_t id);

private:
  DelayWorker dw_;
  struct Task {
    uint32_t id;
    std::function<void()> command;
  };
  std::deque<Task> tasks_;
  std::deque<Task> delaytasks_;
  std::mutex m;
  std::condition_variable cv;
  uint32_t offset_;
  std::thread worker;
  bool runnable;
};

CommandWorker::CommandWorker() {
  worker = std::thread(&CommandWorker::Run, this);
}

CommandWorker::~CommandWorker() {
  worker.join();
}


uint32_t CommandWorker::Add(const std::function<void()>& commandfunc) {
  std::unique_lock<std::mutex> lk(m);
  offset_++;
  Task t {offset_, commandfunc};
  tasks_.emplace_back(t);
  return offset_;
}

uint32_t CommandWorker::Add(const std::function<void()>& commandfunc, uint32_t id) {
  std::unique_lock<std::mutex> lk(m);
  Task t {id, commandfunc};
  tasks_.emplace_back(t);
  return id;
}

uint32_t CommandWorker::DelayAdd(const std::function<void()>& commandfunc, uint8_t delay_sec) {
  std::unique_lock<std::mutex> lk(m);
  offset_++;
  Task t {offset_, commandfunc};
  delaytasks_.emplace_back(t);
  
  std::function<void()> cb = std::bind(&CommandWorker::DelayCallback, this, commandfunc, offset_);
  dw_.Add(cb, delay_sec);
  return offset_;
}

// NOTE : 計算量が大きいので
void CommandWorker::Remove(uint32_t id) {
  std::unique_lock<std::mutex> lk(m);
  decltype(tasks_)::iterator i = tasks_.begin();
  for (; i != tasks_.end(); i++ ) {
    if (i->id == id) {
      tasks_.erase(i);
      return;
    }
  }
  decltype(delaytasks_)::iterator dti = delaytasks_.begin();
  for (; dti != delaytasks_.end(); dti++ ) {
    if (dti->id == id) {
      delaytasks_.erase(dti);
      return;
    }
  }
}

void CommandWorker::Run() {
  LOG("");
  while (runnable) {
    std::unique_lock<std::mutex> lk(m);
    while (tasks_.empty()) {
      cv.wait(lk);
    }
    decltype(tasks_)::iterator taskit = tasks_.begin();
    std::function<void()> command = taskit->command;
    tasks_.pop_front();
    lk.unlock();

    LOG("start task");
    command();
  }
}

void CommandWorker::DelayCallback(const std::function<void()> commandfunc, uint32_t id) {
  Add(commandfunc, id);
}

} // namespace cpputil

cpputil::CommandWorker cw;
void task01(){
  std::cout << __func__ << std::endl;
};
void task02(){
  std::cout << __func__ << std::endl;
};
int main() {
  std::cout << "hello world" << std::endl;

  cw.DelayAdd(task01, 1);
  cw.Add(task02);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  return 0;
}