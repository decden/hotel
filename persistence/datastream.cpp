#include "persistence/datastream.h"
#include "persistence/backend.h"

#include <algorithm>
#include <iostream>

namespace persistence
{
  template <> StreamableType DataStream::GetStreamTypeFor<hotel::Hotel>() { return StreamableType::Hotel; }
  template <> StreamableType DataStream::GetStreamTypeFor<hotel::Reservation>() { return StreamableType::Reservation; }

  UniqueDataStreamHandle::UniqueDataStreamHandle(UniqueDataStreamHandle&& that)
      : _backend(that._backend), _dataStream(std::move(that._dataStream))
  {
    that._backend = nullptr;
  }

  UniqueDataStreamHandle& UniqueDataStreamHandle::operator=(UniqueDataStreamHandle&& that)
  {
    reset();
    _backend = that._backend;
    that._backend = nullptr;
    _dataStream = std::move(that._dataStream);
  }

  UniqueDataStreamHandle::~UniqueDataStreamHandle()
  {
    reset();
  }

  void UniqueDataStreamHandle::reset()
  {
    if (_dataStream)
    {
      _dataStream->disconnect();
      _backend->removeStream(_dataStream);
    }
    _dataStream = nullptr;
    _backend = nullptr;
  }

} // namespace persistence
