#include "fas/future.h"

namespace fas::detail
{
  void executeFuture(std::pair<std::unique_ptr<FutureContinuation>, std::shared_ptr<FutureStateBase>> next)
  {
    while (next.first)
      next = next.first->continueWith(*next.second);
  }
} // namespace fas::detail
