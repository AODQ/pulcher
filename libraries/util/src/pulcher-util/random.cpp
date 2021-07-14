#include <pulcher-util/random.hpp>

#include <random>

namespace {
  std::mt19937_64 generator;
}

void pul::util::InitializeRandom(uint64_t const seed) {
  generator.seed(seed);
}

bool pul::util::RandomBool() {
  static std::uniform_int_distribution<int32_t> distribution { 0, 1 };
  return distribution(generator);
}

bool pul::util::RandomBoolBiased(int32_t biasZeroToOneHundred) {
  return pul::util::RandomInt32(0, 99) <= (biasZeroToOneHundred-1);
}

float pul::util::RandomFloat() {
  static std::uniform_real_distribution<float> distribution { 0.0f, 1.0f };
  return distribution(generator);
}

int32_t pul::util::RandomInt32(int32_t min, int32_t max) {
  std::uniform_int_distribution<int32_t> distribution { min, max };
  return distribution(generator);
}

