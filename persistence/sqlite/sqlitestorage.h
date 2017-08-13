#ifndef PERSISTENCE_SQLITE_SQLITESTORAGE_H
#define PERSISTENCE_SQLITE_SQLITESTORAGE_H

#include "persistence/sqlite/sqlitestatement.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"
#include "hotel/reservation.h"

#include <sqlite3.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace persistence
{
  namespace sqlite
  {

    class SqliteStorage
    {
    public:
      SqliteStorage(const std::string& file);
      ~SqliteStorage();

      void deleteAll();
      void deleteReservationById(int id);

      template<typename T>
      std::vector<T> loadAll();

      template<typename T>
      boost::optional<T> loadById(int id);

      void storeNewHotel(hotel::Hotel& hotel);
      void storeNewReservationAndAtoms(hotel::Reservation& reservation);

      template<typename T>
      bool update(T& value);

      void getReservation();

      void beginTransaction();
      void commitTransaction();

    private:
      SqliteStatement& query(const std::string& key);
      int64_t lastInsertId();

      void prepareQueries();
      void createSchema();

      sqlite3* _db;
      std::map<std::string, SqliteStatement> _statements;
    };

  } // namespace sqlite
} // namespace persistence

#endif // PERSISTENCE_SQLITE_SQLITESTORAGE_H
