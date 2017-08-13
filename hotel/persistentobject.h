#ifndef HOTEL_PERSISTENTOBJECT_H
#define HOTEL_PERSISTENTOBJECT_H

namespace hotel
{
  class PersistentObject
  {
  public:
    int id() const;
    int revision() const;
    void setId(int id);
    void setRevision(int revision);

  private:
    int _id = 0;
    int _revision = 0;
  };

} // namespace hotel

#endif // HOTEL_PERSISTENTOBJECT_H
