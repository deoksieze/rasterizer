#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace raster {

inline int& FailureCount() {
  static int count = 0;
  return count;
}

inline void Check(bool condition, const char* expression, const char* file,
                  int line) {
  if (!condition) {
    std::cerr << file << ':' << line << ": CHECK failed: " << expression
              << '\n';
    ++FailureCount();
  }
}

inline void CheckNear(double actual, double expected, double tolerance,
                      const char* expression, const char* file, int line) {
  if (std::fabs(actual - expected) > tolerance) {
    std::cerr << file << ':' << line << ": CHECK_NEAR failed: " << expression
              << " (actual " << actual << ", expected " << expected << ")\n";
    ++FailureCount();
  }
}

inline int Summary(const char* suite) {
  if (FailureCount() == 0) {
    std::cout << suite << " OK\n";
    return EXIT_SUCCESS;
  }

  std::cerr << suite << ": " << FailureCount() << " failure(s)\n";
  return EXIT_FAILURE;
}

}  // namespace raster

#define CHECK(condition) \
  ::raster::Check((condition), #condition, __FILE__, __LINE__)

#define CHECK_NEAR(actual, expected, tolerance)          \
  ::raster::CheckNear((actual), (expected), (tolerance), \
                      #actual " ~= " #expected, __FILE__, __LINE__)
