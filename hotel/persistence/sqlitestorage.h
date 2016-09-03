#ifndef HOTEL_PERSISTENCE_SQLITESTORAGE_H
#define HOTEL_PERSISTENCE_SQLITESTORAGE_H

#include "hotel/hotel.h"
#include "hotel/reservation.h"
#include "hotel/persistence/sqlitestatement.h"

#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <map>
#include <memory>

namespace hotel {
namespace persistence {



class SqliteStorage
{
public:
  SqliteStorage(const std::string& file);

  bool storeNewHotel(hotel::Hotel& hotel);
  bool storeNewReservationAndAtoms(hotel::Reservation& reservation);

  void getReservation();

  void beginTransaction();
  void commitTransaction();

private:
  SqliteStatement &query(const std::string &key);
  int64_t lastInsertId();

  void prepareQueries();
  void recreateSchema();

  sqlite3 *_db;
  std::map<std::string, SqliteStatement> _statements;
};

} // namespace persistence
} // namespace hotel


#endif // HOTEL_PERSISTENCE_SQLITESTORAGE_H
