#include "math/ng_math.h"

#include <math.h>

double ng_math_min(double left, double right) {
  return left < right ? left : right;
}

double ng_math_max(double left, double right) {
  return left > right ? left : right;
}

double ng_math_sin(double radians) {
  return sin(radians);
}

double ng_math_cos(double radians) {
  return cos(radians);
}

double ng_math_deg_to_rad(double degrees) {
  return degrees * NG_MATH_PI / 180.0;
}
