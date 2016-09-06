#include "hotel/persistentobject.h"

namespace hotel
{

  int PersistentObject::id() const { return _id; }

  void PersistentObject::setId(int id) { _id = id; }

} // namespace hotel
