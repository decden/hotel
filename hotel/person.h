#ifndef HOTEL_PERSON_H
#define HOTEL_PERSON_H

#include <memory>
#include <string>

#include "hotel/persistentobject.h"

namespace hotel
{
  /**
   * @brief The Person class contains information about a physical person.
   */
  class Person : public PersistentObject
  {
  public:
    Person(const std::string& firstName, const std::string& lastName);

    const std::string& firstName() const;
    const std::string& lastName() const;

  private:
    std::string _firstName;
    std::string _lastName;
  };

} // namespace hotel

#endif // HOTEL_PERSON_H
