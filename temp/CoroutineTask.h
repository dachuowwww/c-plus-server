#pragma once
#include <coroutine>
#include <exception>

namespace coro {
class DetachedTask {
 public:
  struct promise_type {
    DetachedTask get_return_object() noexcept { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() { std::terminate(); }
  };
};
}  // namespace coro
