#include "math/number_utils.h"

#include <math.h>

double ng_clamp_double(double value, double min_value, double max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

int ng_round_to_int(double value) {
  return (int)lround(value);
}
