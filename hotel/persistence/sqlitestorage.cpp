#include "sqlitestorage.h"

#include <iostream>

namespace hotel
{
  namespace persistence
  {

    namespace
    {

      void executeSQL(sqlite3* db, const std::string& sql)
      {
        if (!SqliteStatement(db, sql).execute())
          std::cerr << "Cannot execute query: " << sql;
      }
    }

    SqliteStorage::SqliteStorage(const std::string& file) : _db(nullptr)
    {
      if (sqlite3_open(file.c_str(), &_db))
      {
        std::cerr << "Cannot open sqlite database: " << file << std::endl;
        sqlite3_close(_db);
        _db = nullptr;
      }

      if (_db != nullptr)
      {
        createSchema();
        prepareQueries();
      }
    }

    SqliteStorage::~SqliteStorage() { sqlite3_close(_db); }

    void SqliteStorage::deleteAll()
    {
      if (_db == nullptr)
        return;

      _statements.clear();
      executeSQL(_db, "DROP TABLE IF EXISTS h_reservation_atom;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_reservation;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_room;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_room_category;");
      executeSQL(_db, "DROP TABLE IF EXISTS h_hotel;");

      createSchema();
      prepareQueries();
    }

    std::vector<std::unique_ptr<Hotel>> SqliteStorage::loadHotels()
    {
      std::vector<std::unique_ptr<Hotel>> results;

      // Read hotels
      auto& hotelsQuery = query("hotel.all");
      hotelsQuery.execute();
      while (hotelsQuery.hasResultRow())
      {
        int id;
        std::string name;
        hotelsQuery.readRow(id, name);
        auto hotel = std::make_unique<hotel::Hotel>(name);
        hotel->setId(id);
        results.push_back(std::move(hotel));
      }

      for (auto& hotel : results)
      {
        // Read categories
        auto& categoriesQuery = query("room_category.by_hotel_id");
        categoriesQuery.execute(hotel->id());
        while (categoriesQuery.hasResultRow())
        {
          int id;
          std::string short_code;
          std::string name;
          categoriesQuery.readRow(id, short_code, name);
          auto category = std::make_unique<hotel::RoomCategory>(short_code, name);
          category->setId(id);
          hotel->addRoomCategory(std::move(category));
        }

        // Read rooms
        auto& roomsQuery = query("room.by_hotel_id");
        roomsQuery.execute(hotel->id());
        while (roomsQuery.hasResultRow())
        {
          int id;
          int category_id;
          std::string name;
          roomsQuery.readRow(id, category_id, name);
          auto room = std::make_unique<hotel::HotelRoom>(name);
          room->setId(id);
          auto category = hotel->getCategoryById(category_id);
          if (category)
            hotel->addRoom(std::move(room), category->shortCode());
          else
            std::cerr << "Did not find category with id " << category_id << " in hotel " << hotel->name()
                      << " for room " << name << std::endl;
        }
      }

      return std::move(results);
    }

    std::unique_ptr<PlanningBoard> SqliteStorage::loadPlanning(const std::vector<int>& roomIds)
    {
      auto result = std::make_unique<hotel::PlanningBoard>();

      for (auto id : roomIds)
        result->addRoomId(id);

      auto& reservationsQuery = query("reservation_and_atoms.all");
      reservationsQuery.execute();
      std::unique_ptr<hotel::Reservation> current = nullptr;
      while (reservationsQuery.hasResultRow())
      {
        int reservationId;
        std::string description;
        int atomId;
        int roomId;
        boost::gregorian::date dateFrom;
        boost::gregorian::date dateTo;
        reservationsQuery.readRow(reservationId, description, atomId, roomId, dateFrom, dateTo);

        if (current == nullptr || current->id() != reservationId)
        {
          if (current)
            result->addReservation(std::move(current));
          current = std::make_unique<hotel::Reservation>(description, roomId,
                                                         boost::gregorian::date_period(dateFrom, dateTo));
          current->setId(reservationId);
        }
        else
        {
          current->addContinuation(roomId, dateTo);
        }
        (*current->atoms().rbegin())->setId(atomId);
      }
      if (current)
        result->addReservation(std::move(current));

      return result;
    }

    bool SqliteStorage::storeNewHotel(Hotel& hotel)
    {
      // First, store the hotel
      query("hotel.insert").execute(hotel.name());
      hotel.setId(lastInsertId());

      // Store all of the categories
      for (auto& category : hotel.categories())
      {
        query("room_category.insert").execute(hotel.id(), category->shortCode(), category->name());
        category->setId(lastInsertId());
      }

      // Store all of the rooms
      for (auto& room : hotel.rooms())
      {
        query("room.insert").execute(hotel.id(), room->category()->id(), room->name());
        room->setId(lastInsertId());
      }
    }

    bool SqliteStorage::storeNewReservationAndAtoms(Reservation& reservation)
    {
      query("reservation.insert").execute(reservation.description());
      reservation.setId(lastInsertId());
      for (auto& atom : reservation.atoms())
      {
        auto& q = query("reservation_atom.insert");
        q.execute(reservation.id(), atom->_roomId, atom->_dateRange.begin(), atom->_dateRange.end());
        atom->setId(lastInsertId());
      }
    }

    void SqliteStorage::getReservation()
    {
      auto& q = query("reservation.get");
      q.execute();
      while (q.hasResultRow())
      {
        std::string a, b;
        q.readRow(a, b);
        std::cout << a << " - " << b << std::endl;
      }
    }

    SqliteStatement& SqliteStorage::query(const std::string& key)
    {
      auto it = _statements.find(key);
      if (it == _statements.end())
      {
        std::cerr << "Query '" << key << "' does not exist!" << std::endl;
      }

      return it->second;
    }

    int64_t SqliteStorage::lastInsertId() { return sqlite3_last_insert_rowid(_db); }

    void SqliteStorage::beginTransaction() { sqlite3_exec(_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr); }

    void SqliteStorage::commitTransaction() { sqlite3_exec(_db, "END TRANSACTION", nullptr, nullptr, nullptr); }

    void SqliteStorage::prepareQueries()
    {
      _statements.emplace("hotel.insert", SqliteStatement(_db, "INSERT INTO h_hotel (name) VALUES (?);"));
      _statements.emplace("hotel.all", SqliteStatement(_db, "SELECT id, name FROM h_hotel;"));
      _statements.emplace("room_category.insert",
                          SqliteStatement(_db, "INSERT INTO h_room_category (hotel_id, short_code, name) VALUES (?, ?, ?);"));
      _statements.emplace("room_category.by_hotel_id",
                          SqliteStatement(_db, "SELECT id, short_code, name FROM h_room_category WHERE hotel_id = ?;"));
      _statements.emplace("room.insert",
                          SqliteStatement(_db, "INSERT INTO h_room (hotel_id, category_id, name) VALUES (?, ?, ?);"));
      _statements.emplace("room.by_hotel_id",
                          SqliteStatement(_db, "SELECT id, category_id, name FROM h_room WHERE hotel_id = ?;"));

      _statements.emplace("reservation_and_atoms.all",
                          SqliteStatement(_db, "SELECT r.id, r.description, a.id, a.room_id, a.date_from, a.date_to "
                                               "FROM h_reservation as r, h_reservation_atom as a WHERE "
                                               "a.reservation_id = r.id ORDER BY r.id, a.date_from;"));
      _statements.emplace("reservation.get",
                          SqliteStatement(_db, "SELECT date_from, date_to FROM h_reservation_atom LIMIT 1;"));
      _statements.emplace("reservation.insert",
                          SqliteStatement(_db, "INSERT INTO h_reservation (description) VALUES (?);"));
      _statements.emplace(
          "reservation_atom.insert",
          SqliteStatement(
              _db,
              "INSERT INTO h_reservation_atom (reservation_id, room_id, date_from, date_to) VALUES (?, ?, ?, ?);"));
    }

    void SqliteStorage::createSchema()
    {
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_hotel ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "name TEXT NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_room_category ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "hotel_id INTEGER NOT NULL," // Foreign key
                      "short_code TEXT NOT NULL,"
                      "name TEXT NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_room ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "hotel_id INTEGER NOT NULL,"    // Foreign key
                      "category_id INTEGER NOT NULL," // Foreign key
                      "name TEXT NOT NULL);");

      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_reservation ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "description TEXT NOT NULL);");
      executeSQL(_db, "CREATE TABLE IF NOT EXISTS h_reservation_atom ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "reservation_id INTEGER NOT NULL," // Foreign key
                      "room_id INTEGER NOT NULL,"        // Foreign key
                      "date_from TEXT NOT NULL,"
                      "date_to TEXT NOT NULL);");
    }

  } // namespace persistence
} // namespace hotel
