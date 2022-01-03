#include "DelayWorker.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace cpputil
{
#define LOG(_logbuf) std::cout << __func__ << " "<< (_logbuf) << std::endl

DelayWorker::DelayWorker() {
  numtasks_ = NUM_TASKS_DEFAULT;
  Init();
}

DelayWorker::~DelayWorker() {
  for (auto index = 0; index < numtasks_; index++) {
    if (taskList_[index].running) {
      taskList_[index].running = false;
      taskList_[index].cv.notify_one();
      taskList_[index].worker->join();
    }
  }
}

int8_t DelayWorker::Add(const std::function<void()>& cb, uint8_t delay_sec) {
  uint8_t index;
  for (index = 0; index < numtasks_; index++) {
    std::unique_lock<std::mutex> ulk(taskList_[index].m);
    if (taskList_[index].status == Task::Status::Empty) {
      taskList_[index].tp = std::chrono::steady_clock::now() + std::chrono::seconds(delay_sec);
      taskList_[index].cb = cb;
      taskList_[index].status = Task::Status::Ready;
      if (taskList_[index].worker == nullptr) {
        taskList_[index].running = true;
        taskList_[index].worker.reset(new std::thread(&DelayWorker::WorkerFunc, this, index));
      }
      break;
    }
  }
  return index;
}

void DelayWorker::Cancel(uint8_t index) {  
  std::unique_lock<std::mutex> ulk(taskList_[index].m);
  if (taskList_[index].status == Task::Status::Ready) {
    taskList_[index].status = Task::Status::Empty;
  } else if (taskList_[index].status == Task::Status::Working) {
    taskList_[index].cv.notify_one();
  }
}

void DelayWorker::Init() {
  for (uint8_t index = 0; index < numtasks_; index++) {
    taskList_[index].running = false;
    taskList_[index].status = Task::Status::Empty;
    taskList_[index].cb = std::bind(&DelayWorker::DummyCallback, this);
  }
}

void DelayWorker::WorkerFunc(uint8_t index) {
  while(taskList_[index].running) {
    LOG("waiting");
    std::unique_lock<std::mutex> ulk(taskList_[index].m);
    while (taskList_[index].status != Task::Status::Ready) {
      if (!taskList_[index].running) {
        LOG("shutdown");
        return;
      }
      taskList_[index].cv.wait(ulk);
    }
    // work
    taskList_[index].status = Task::Status::Working;
    if (taskList_[index].cv.wait_until(ulk, taskList_[index].tp) == std::cv_status::timeout) {
      taskList_[index].m.unlock();
      taskList_[index].cb();
      taskList_[index].m.lock();
    }
    taskList_[index].status = Task::Status::Empty;
    taskList_[index].m.unlock();
  }
  LOG("shutdown");
  return;
}

void DelayWorker::DummyCallback() {;}

} // namespace cpputil

cpputil::DelayWorker sc;
void dummy(){;}
void callback0() {
  sc.Cancel(0);
  sc.Add(callback0, 1);
}

/*
int main() {
  std::cout << "hello" << std::endl;

  sc.Add(callback0, 1);
  sc.Cancel(0);
  sleep(5);
  //sc.~DelayWorker();
  return 0;
}
*/