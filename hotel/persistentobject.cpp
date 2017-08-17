#include "hotel/persistentobject.h"

namespace hotel
{

int PersistentObject::id() const { return _id; }

int PersistentObject::revision() const { return _revision; }

void PersistentObject::setId(int id) { _id = id; }

void PersistentObject::setRevision(int revision) { _revision = revision; }

} // namespace hotel
