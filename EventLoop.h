/**
 * @brief スレッドによるイベントループを実現する。
 *        実行したい関数をイベントと定義し、サブスレッド上で実行する。複数のイベントをユニークなサブスレッドで実行する。
 *        イベントはFIFOキューで管理される。イベントはキューの最後尾に積まれる。キューに積まれた順に実行される。
 *        イベントの実行タイミングを指定することもできる。指定した時間に、キューの最後尾に積む。
 *        このクラスでサブスレッドは2つまで起動する。1つはイベントループ実行用。もう１つは実行タイミングの監視用。
 *        まだ実行されていないイベントは、キューから削除することができる。
 * @todo  1. スレッドプールによるスレッドの再利用を検討する。スレッドの起動終了にコストがかかる環境のため。
 *        2. スレッドの遅延起動について検討する。コンストラクタ実行時は、パフォーマンスに限りのあるシステム起動時であることが多い。
 *           システム起動時に使わないスレッドを起動するのは避けたい。イベントをキューに積むときに起動するのが良さそう。
 *        3. イベントの削除を実装する。
 */

#ifndef THREADUTIL_EVENTLOOP_H
#define THREADUTIL_EVENTLOOP_H

#include <list>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>

namespace threadutil {

// イベント関数の型
constexpr uint8_t EVENT_ONESHOT  = 0;
constexpr uint8_t EVENT_CONTINUE = 1;
using EventFunc = std::function<uint8_t(void)>;

class EventLoop {
private:
  struct EventSource {
    uint32_t id;
    std::string tag;
    EventFunc func;
  };
  struct EventQueue {
    std::list<EventSource> q;
    std::mutex m;
    std::condition_variable cv;
    uint32_t offset = 0x00000000;
  };
  EventQueue event_queue_;

  struct TimedEventSource {
    std::chrono::steady_clock::time_point schedlued_point;
    uint32_t id;
    std::string tag;
    EventFunc func;
  };
  struct TimedEventQueue {
    std::list<TimedEventSource> q;
    std::mutex m;
    std::condition_variable cv;
    uint32_t offset = 0xFFFF0000;
  };
  TimedEventQueue timed_queue_;
  
  std::thread event_queue_thread_;
  std::thread timed_queue_thread_;
  bool runnable_;

public:
  EventLoop();
  ~EventLoop();

  /**
   * @brief イベントをキューの最後尾に積む。
   * @param func イベントとして実行したい関数。
   * @param tag  イベントをキューから削除するための検索キーに使用する。
   * @return イベントIDを返す。イベントをキューから削除するための検索キーに使用する。
  */
  uint32_t IdleAdd(const EventFunc& func, const std::string& tag = std::string());

  /**
   * @brief イベントを時刻指定でキューの最後尾に積む。その時刻時点での最後尾に積むため、厳密な時刻に実行できないことに注意。
   * @param func イベントとして実行したい関数。
   * @param period steady_clock形式の時刻。
   * @param tag  イベントをキューから削除するための検索キーに使用する。
   * @return イベントIDを返す。イベントをキューから削除するための検索キーに使用する。
  */
  uint32_t TimedAdd(const EventFunc& func, const std::chrono::steady_clock::time_point period, const std::string& tag = std::string());

private:
  void EventDispatcher();
  void TimedEventDispatcher();
};

} // namespace threadutil

#endif //THREADUTIL_EVENTLOOP_H
