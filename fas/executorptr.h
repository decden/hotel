#pragma once

#include <memory>

namespace fas
{
  /**
   * @brief Holds a shared inner executor, which can be copied
   */
  template <class Executor> class ExecutorPtr
  {
  public:
    ExecutorPtr(std::shared_ptr<Executor> innerExecutor) : _innerExecutor(std::move(innerExecutor)) {}

    template <class Fn> void spawn(Fn&& fn) { _innerExecutor->spawn(std::forward<Fn>(fn)); }

  private:
    std::shared_ptr<Executor> _innerExecutor;
  };
} // namespace fass
