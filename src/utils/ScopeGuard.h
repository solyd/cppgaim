#pragma once

#include <algorithm>

namespace utils {

template<typename FuncT>
class ScopeGuard {
public:
  ScopeGuard(ScopeGuard&& other) noexcept(std::is_nothrow_move_constructible<FuncT>::value)
    : cleanup_(std::move(other.cleanup_))
    , should_cleanup_(other.should_cleanup_)
  {
    other.should_cleanup_ = false;
  }

  explicit ScopeGuard(const FuncT& cleanup) : should_cleanup_(true), cleanup_(cleanup) {}

  ~ScopeGuard() {
    if (should_cleanup_) {
      cleanup_();
    }
  }

  void disable() {
    should_cleanup_ = false;
  }

private:
  bool should_cleanup_;
  FuncT cleanup_;
};

template<typename FuncT>
ScopeGuard<typename std::decay<FuncT>::type> make_guard(FuncT&& cleanup) {
  return ScopeGuard<typename std::decay<FuncT>::type>(std::forward<FuncT>(cleanup));
}

}
