#include "fas/cancellation.h"

namespace fas
{
  CancellationSource makeCancellationSource()
  {
    return CancellationSource(std::make_shared<detail::CancellationTokenState>());
  }
} // namespace fas
