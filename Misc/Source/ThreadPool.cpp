//
// Created by Petr Flajsingr on 2019-02-24.
//

#include <Exceptions.h>
#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t numThreads) {
  start(numThreads);
}

ThreadPool::~ThreadPool() {
  stop();
}
void ThreadPool::start(std::size_t numThreads) {
  for (auto i = 0u; i < numThreads; ++i) {
    threads.emplace_back([=] {
      while (true) {
        Task task;

        {
          std::unique_lock<std::mutex> lock{mutex};
          event.wait(lock, [=] { return stopping || !tasks.empty(); });

          if (stopping && tasks.empty())
            break;

          task = std::move(tasks.front());
          tasks.pop();
        }

        task();
      }
    });
  }
}
void ThreadPool::stop() noexcept {
  {
    std::unique_lock<std::mutex> lock{mutex};
    stopping = true;
  }

  event.notify_all();

  for (auto &thread : threads)
    thread.join();
}
void ThreadPool::receive(std::shared_ptr<Message> message) {
  if (auto msg = std::dynamic_pointer_cast<ExecAsync>(message)) {
    enqueue(msg->getFnc());
  } else if (auto msg = std::dynamic_pointer_cast<ExecAsyncNotify>(message)) {
    auto execAndNotify = [msg] {
      msg->getFnc()();
      msg->getOnDone()();
    };
    enqueue(execAndNotify);
  }
}