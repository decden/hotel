#ifndef HOTEL_PERSISTENCE_SQLITESTORAGE_H
#define HOTEL_PERSISTENCE_SQLITESTORAGE_H

#include "hotel/hotel.h"
#include "hotel/persistence/sqlitestatement.h"
#include "hotel/planning.h"
#include "hotel/reservation.h"

#include <sqlite3.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace hotel
{
  namespace persistence
  {

    class SqliteStorage
    {
    public:
      SqliteStorage(const std::string& file);
      ~SqliteStorage();

      void deleteAll();

      std::vector<std::unique_ptr<hotel::Hotel>> loadHotels();
      std::unique_ptr<hotel::PlanningBoard> loadPlanning(const std::vector<int>& roomIds);

      bool storeNewHotel(hotel::Hotel& hotel);
      bool storeNewReservationAndAtoms(hotel::Reservation& reservation);

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

  } // namespace persistence
} // namespace hotel

#endif // HOTEL_PERSISTENCE_SQLITESTORAGE_H
