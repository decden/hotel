#ifndef __CLI_TESTDATA_H__
#define __CLI_TESTDATA_H__

#include "hotel/hotel.h"
#include "hotel/planning.h"

#include <memory>
#include <vector>
#include <random>

namespace cli {

  std::vector<std::unique_ptr<hotel::Hotel>> createTestHotels(std::mt19937 &rng);
  std::unique_ptr<hotel::PlanningBoard> createTestPlanning(std::mt19937 &rng, std::vector<std::unique_ptr<hotel::Hotel>>& hotels);

} // namespaec cli

#endif // __CLI_TESTDATA_H__
