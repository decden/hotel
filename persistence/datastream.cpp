#include "persistence/backend.h"
#include "persistence/datastream.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  template <> StreamableType DataStream::GetStreamTypeFor<hotel::Hotel>() { return StreamableType::Hotel; }
  template <> StreamableType DataStream::GetStreamTypeFor<hotel::Reservation>() { return StreamableType::Reservation; }

  UniqueDataStreamHandle::~UniqueDataStreamHandle()
  {
    if (_dataStream)
    {
      _backend->removeStream(_dataStream);
      _dataStream->disconnect();
    }
  }

} // namespace persistence
