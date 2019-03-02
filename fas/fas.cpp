#include "fas/fas.h"
#include "fas/threadedexecutor.h"

#include <cassert>

namespace fas::detail
{
  ThreadedExecutor* g_systemExecutor = nullptr;
}

namespace fas
{
  void initialize()
  {
    assert(fas::detail::g_systemExecutor == nullptr);
    fas::detail::g_systemExecutor = new fas::ThreadedExecutor;
    fas::detail::g_systemExecutor->start();
  }

  /**
   * @brief shuts down the fas library
   *
   * This method has to be called in order to shut down the system executor. When calling this method, all executors
   * need to be idle.
   */
  void shutdown()
  {
    assert(fas::detail::g_systemExecutor != nullptr);
    fas::detail::g_systemExecutor->stop();
    delete fas::detail::g_systemExecutor;
    fas::detail::g_systemExecutor = nullptr;
  }
} // namespace fas
