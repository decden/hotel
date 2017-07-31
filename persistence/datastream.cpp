#include "persistence/datastream.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  template<> StreamableType DataStream::GetStreamTypeFor<hotel::Hotel>() { return StreamableType::Hotel; }
  template<> StreamableType DataStream::GetStreamTypeFor<hotel::Reservation>() { return StreamableType::Reservation; }
} // namespace persistence
