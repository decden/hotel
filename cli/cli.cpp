#include "cli/testdata.h"

#include "hotel/persistence/sqlitestorage.h"

#include <iostream>
#include <fstream>


int main(int argc, char **argv)
{
  // Get us some random test data
  std::mt19937 rng;
  auto hotels = cli::createTestHotels(rng);
  auto planning = cli::createTestPlanning(rng, hotels);

  hotel::persistence::SqliteStorage storage("test.db");
  storage.beginTransaction();
  for (auto& reservation : planning->reservations())
    storage.storeNewReservationAndAtoms(*reservation);
  storage.commitTransaction();

  storage.getReservation();

  std::ofstream output("out.html");

  output << "<!DOCTYPE html>" << std::endl;
  output << "<html><head><style>"
         << "div.reservation {box-sizing:border-box;position:absolute;padding:2px 4px;height:23px;"
         << "border:1px solid #aaa;background:#fafafa;border-radius:7px;"
         << "overflow:hidden;text-overflow:ellipsis;cursor:pointer;"
         << "font-family:sans-serif;color:#444;box-sizing:border-box;font-size:12px;line-height:17px;}"
         << "div.reservation:hover {background-color:dodgerblue;color:white;}"
         << "div.cont-left {border-radius: 0 7px 7px 0;}"
         << "div.cont-right {border-radius: 7px 0 0 7px;}"
         << "div.cont-left.cont-right {border-radius:0;}"
         << "</style></head><body>" << std::endl;

  std::map<std::string, int> roomY;
  int y = 0;
  for (auto& hotel : hotels)
  {
    for (auto& room : hotel->rooms())
    {
      roomY[room.name()] = y;
      y += 24;
    }
    y += 5;
  }

  auto baseDate = boost::gregorian::date(2016, boost::gregorian::Jan, 1);
  for (auto& reservation : planning->reservations())
  {
    for (auto& atom : reservation->atoms())
    {
      auto y = roomY[atom->_room];
      auto x = (atom->_dateRange.begin() - baseDate).days() * 20;
      auto width = atom->_dateRange.length().days() * 20 + 1 - 2;
      output << "<div class=\"reservation"
             << (atom->isFirst() ? "" : " cont-left")
             << (atom->isLast() ? "" : " cont-right")
             << "\" style=\"width:" << width << "px;"
             << "top:" << y << "px;"
             << "left:" << x << "px;\">"
             << (atom->isFirst() ? "" : "&gt; ")
             << atom->_reservation->_description << " ("
             << atom->_reservation->length() << " days)</div>";
    }
  }

  output << "</html>" << std::endl;


  return 0;
}
