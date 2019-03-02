#pragma once

namespace fas
{
  class ThreadedExecutor;
}

namespace fas::detail
{
  extern ThreadedExecutor* g_systemExecutor;
}

namespace fas
{
  /**
   * @brief initializes the fas library.
   *
   * This method has to be called in order to correctly setup the system executor
   */
  extern void initialize();

  /**
   * @brief shuts down the fas library
   *
   * This method has to be called in order to shut down the system executor. When calling this method, all executors
   * need to be idle.
   */
  extern void shutdown();
} // namespace fas
