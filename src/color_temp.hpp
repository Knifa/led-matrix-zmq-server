#pragma once

#include <tuple>

namespace color_temp {
constexpr int min = 2000;
constexpr int max = 6500;

using TemperatureColor = std::tuple<int, int, int>;

TemperatureColor get(int kelvin);
} // namespace color_temp
