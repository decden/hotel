#ifndef HOTEL_PERSISTENTOBJECT_H
#define HOTEL_PERSISTENTOBJECT_H

namespace hotel
{

  class PersistentObject
  {
  public:
    int id() const;
    void setId(int id);

  private:
    int _id = 0;
  };

} // namespace hotel

#endif // HOTEL_PERSISTENTOBJECT_H
