#include "color_temp.hpp"
#include <array>
#include <tuple>

using color_temp::TemperatureColor;

constexpr int color_temp_min = 2000;
constexpr int color_temp_max = 6500;
constexpr int color_temp_size = 48;

std::array<TemperatureColor, color_temp_size> color_temps = {{
    {255, 138, 18},
    {255, 142, 33},
    {255, 147, 44},
    {255, 152, 54},
    {255, 157, 63},
    {255, 161, 72},
    {255, 165, 79},
    {255, 169, 87},
    {255, 173, 94},
    {255, 177, 101},
    {255, 180, 107},
    {255, 184, 114},
    {255, 187, 120},
    {255, 190, 126},
    {255, 193, 132},
    {255, 196, 137},
    {255, 199, 143},
    {255, 201, 148},
    {255, 204, 153},
    {255, 206, 159},
    {255, 209, 163},
    {255, 211, 168},
    {255, 213, 173},
    {255, 215, 177},
    {255, 217, 182},
    {255, 219, 186},
    {255, 221, 190},
    {255, 223, 194},
    {255, 225, 198},
    {255, 227, 202},
    {255, 228, 206},
    {255, 230, 210},
    {255, 232, 213},
    {255, 233, 217},
    {255, 235, 220},
    {255, 236, 224},
    {255, 238, 227},
    {255, 239, 230},
    {255, 240, 233},
    {255, 242, 236},
    {255, 243, 239},
    {255, 244, 242},
    {255, 245, 245},
    {255, 246, 247},
    {255, 248, 251},
    {255, 249, 253},
    {254, 252, 255},
    {255, 255, 255},
}};

TemperatureColor color_temp::get(int kelvin)
{
    const auto index_fract = float(kelvin - color_temp_min) / float(color_temp_max - color_temp_min);
    const auto index_a = static_cast<int>(index_fract * (color_temps.size() - 1));
    const auto index_b = index_a + 1;

    const auto fract = index_fract * float(color_temp_size - 1) - index_a;

    const auto ca = color_temps[index_a];
    const auto cb = color_temps[index_b];

    const auto r = std::get<0>(ca) * (1.0 - fract) + std::get<0>(cb) * fract;
    const auto g = std::get<1>(ca) * (1.0 - fract) + std::get<1>(cb) * fract;
    const auto b = std::get<2>(ca) * (1.0 - fract) + std::get<2>(cb) * fract;

    return std::make_tuple(r, g, b);
}
