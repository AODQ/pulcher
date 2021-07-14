#include <cstdint>

namespace pul::util {
  void InitializeRandom(uint64_t const seed = 0);
  bool  RandomBool();
  bool  RandomBoolBiased(int32_t biasZeroToOneHundred);
  float RandomFloat();
  int32_t RandomInt32(int32_t min, int32_t max);
}
