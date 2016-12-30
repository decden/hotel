#include "hotel/person.h"

#include <algorithm>

namespace hotel
{

  Person::Person(const std::string &firstName, const std::string &lastName)
      : _firstName(firstName), _lastName(lastName)
  {
  }

  const std::string &Person::firstName() const { return _firstName; }
  const std::string &Person::lastName() const { return _lastName; }

} // namespace hotel
