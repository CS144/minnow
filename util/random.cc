#include "random.hh"

#include <algorithm>
#include <array>

using namespace std;

default_random_engine get_random_engine()
{
  auto rd = random_device();
  array<uint32_t, 1024> seed_data {};
  generate( seed_data.begin(), seed_data.end(), [&] { return rd(); } );
  seed_seq seed( seed_data.begin(), seed_data.end() );
  return default_random_engine( seed );
}
