#ifndef GUIAPP_TESTDATA_H
#define GUIAPP_TESTDATA_H

#include "persistence/datasource.h"

#include "hotel/hotel.h"
#include "hotel/hotelcollection.h"
#include "hotel/planning.h"

#include <memory>
#include <random>
#include <vector>

namespace guiapp
{
  /**
   * @brief createTestData overwrites the contents of the database with randomly generated test data
   * @note This function does destroy anything that was previously contained in the database. This function is meant
   *       to be used only during development for testing.
   */
  void createTestData(persistence::DataSource& dataSource);

  std::vector<std::unique_ptr<hotel::Hotel>> createTestHotels(std::mt19937& rng);
  std::unique_ptr<hotel::PlanningBoard> createTestPlanning(std::mt19937& rng, const std::vector<hotel::Hotel> &hotels);
} // namespaec guiapp

#endif // GUIAPP_TESTDATA_H
