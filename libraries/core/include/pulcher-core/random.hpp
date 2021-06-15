#include <cstdint>

namespace pul::core {
  void InitializeRandom(uint64_t const seed = 0);
  bool  RandomBool();
  float RandomFloat();
  int32_t RandomInt32(int32_t min, int32_t max);
}
