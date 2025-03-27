#pragma once
#include "defs.h"

#include <numeric>
#include <string>
#include <sstream>
#include <vector>

std::vector<std::string> stringSplit(const std::string &str, const char delim);
Vec3f calculateNormal(const Point3f &a, const Point3f &b, const Point3f &c);
Color clampColor(const Color &color);
Vec3f vec3SelectMax(const Vec3f &v1, const Vec3f &v2);
Vec3f vec3SelectMin(const Vec3f &v1, const Vec3f &v2);

inline bool equalf(FLOAT a, FLOAT b, FLOAT epsion = 1e-6f) { return std::fabs(a - b) <= epsion; };
inline bool isLowest(FLOAT f) { return equalf(f, std::numeric_limits<FLOAT>::lowest()); }

INT saveImage(const char *filename, const Color *data, INT width, INT height);
std::string stringToLower(const std::string& s);