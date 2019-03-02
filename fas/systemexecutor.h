#pragma once

#include "fas/fas.h"
#include "fas/threadedexecutor.h"

#include <cassert>

namespace fas
{
  class SystemExecutor
  {
  public:
    SystemExecutor() = default;
    SystemExecutor(const SystemExecutor&) = default;
    SystemExecutor& operator=(const SystemExecutor&) = default;

    template <class Fn>
    void spawn(Fn&& fn)
    {
      assert(detail::g_systemExecutor != nullptr);
      detail::g_systemExecutor->spawn(std::forward<Fn>(fn));
    }
  };
} // namespace fass
