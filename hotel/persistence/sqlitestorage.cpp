#include "sqlitestorage.h"

#include <iostream>

namespace hotel {
namespace persistence {

namespace {

  void executeSQL(sqlite3 *db, const std::string &sql)
  {
    auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (rc)
      std::cerr << "Cannot execute query: " << sql << std::endl;
  }

}

SqliteStorage::SqliteStorage(const std::string &file)
  : _db(nullptr)
{
  if (sqlite3_open(file.c_str(), &_db))
  {
    std::cerr << "Cannot open sqlite database: " << file << std::endl;
    sqlite3_close(_db);
    _db = nullptr;
  }

  if (_db) prepareQueries();
  if (_db) recreateSchema();
}

bool SqliteStorage::storeNewReservationAndAtoms(Reservation &reservation)
{
  query("reservation.insert").execute(reservation._description);
  reservation.setId(lastInsertId());
  for (auto& atom : reservation.atoms())
  {
    auto &q = query("reservation_atom.insert");
    q.execute(reservation.id(), atom->_room, atom->_dateRange.begin(), atom->_dateRange.end());
    atom->setId(lastInsertId());
  }
}

void SqliteStorage::getReservation()
{
  auto &q = query("reservation.get");
  q.execute();
  while(q.hasResultRow())
  {
    std::string a, b;
    q.readRow(a, b);
    std::cout << a << " - " << b << std::endl;
  }
}

SqliteStatement &SqliteStorage::query(const std::string &key)
{
  auto it = _statements.find(key);
  if (it == _statements.end())
  {
    std::cerr << "Query '" << key << "' does not exist!" << std::endl;
  }

  return it->second;
}

int64_t SqliteStorage::lastInsertId()
{
  return sqlite3_last_insert_rowid(_db);
}

void SqliteStorage::beginTransaction()
{
  sqlite3_exec(_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
}

void SqliteStorage::commitTransaction()
{
  sqlite3_exec(_db, "END TRANSACTION", nullptr, nullptr, nullptr);
}

void SqliteStorage::prepareQueries()
{
  _statements.emplace("reservation.get", SqliteStatement(_db, "SELECT date_from, date_to FROM h_reservation_atom LIMIT 1;"));
  _statements.emplace("reservation.insert", SqliteStatement(_db, "INSERT INTO h_reservation (description) VALUES (?);"));
  _statements.emplace("reservation_atom.insert", SqliteStatement(_db, "INSERT INTO h_reservation_atom (reservation_id, room_id, date_from, date_to) VALUES (?, ?, ?, ?);"));
}

void SqliteStorage::recreateSchema()
{
  executeSQL(_db, "DROP TABLE IF EXISTS h_reservation;");
  executeSQL(_db, "DROP TABLE IF EXISTS h_reservation_atom;");
  executeSQL(_db, "DROP TABLE IF EXISTS h_hotel;");
  executeSQL(_db, "DROP TABLE IF EXISTS h_room;");
  executeSQL(_db, "DROP TABLE IF EXISTS h_room_category;");

  executeSQL(_db, "CREATE TABLE h_reservation ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "description TEXT NOT NULL);");
  executeSQL(_db, "CREATE TABLE h_reservation_atom ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                      "reservation_id INTEGER NOT NULL," // Foreign key
                      "room_id TEXT NOT NULL,"
                      "date_from TEXT NOT NULL,"
                      "date_to TEXT NOT NULL);");
}


} // namespace persistence
} // namespace hotel
