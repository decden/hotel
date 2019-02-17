#include "persistence/op/operations.h"

namespace persistence
{
  namespace op
  {
    template <> StreamableType getStreamableType<hotel::Hotel>() { return StreamableType::Hotel; }
    template <> StreamableType getStreamableType<hotel::Reservation>() { return StreamableType::Reservation; }
    template <> StreamableType getStreamableType<hotel::Person>() { return StreamableType::Person; }

    StreamableType getStreamableType(const StreamableTypePtr& ptr)
    {
      return std::visit([](const auto& ptr) {
        using T = typename std::decay<decltype(*ptr)>::type;
        return getStreamableType<T>();
      }, ptr);
    }

  } // namespace op
} // namespace persistence
