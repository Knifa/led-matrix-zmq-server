#pragma once

#include <cstdint>

enum class ControlMessageType {
  Brightness,
  Temperature,
};

struct BrightnessMessageArgs {
  uint8_t brightness;
};

struct TemperatureMessageArgs {
  uint16_t temperature;
};

template <typename T> struct ControlMessage {
  ControlMessageType type;
  T args;
};

using BrightnessMessage = ControlMessage<BrightnessMessageArgs>;
using TemperatureMessage = ControlMessage<TemperatureMessageArgs>;
