#pragma once

#include <tuple>

namespace color_temp {
using TemperatureColor = std::tuple<int, int, int>;

TemperatureColor get(int kelvin);
} // namespace color_temp
