//
// Created by Petr Flajsingr on 2019-02-27.
//

#ifndef PROJECT_LAZY_H
#define PROJECT_LAZY_H

#include <functional>

template<typename T>
class Lazy {
  using InitFnc = std::function<T()>;
 public:
  explicit Lazy(const InitFnc &init) : init(init) {}
  virtual operator const T &() {
    if (!initialised) {
      value = init();
      initialised = true;
    }
    return value;
  }
  void invalidate() {
    initialised = false;
  }
 private:
  T value;
  InitFnc init;
  bool initialised = false;
};

#endif //PROJECT_LAZY_H
