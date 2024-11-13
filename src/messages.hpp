#pragma once

#include <cstdint>

enum class ControlMessageType : uint8_t {
  Brightness,
  Temperature,
};

#pragma pack(push, 1)

template <typename T> struct ControlMessage {
  ControlMessageType type;
  T args;
};

struct BrightnessMessageArgs {
  uint8_t brightness;
};
using BrightnessMessage = ControlMessage<BrightnessMessageArgs>;

struct TemperatureMessageArgs {
  uint16_t temperature;
};
using TemperatureMessage = ControlMessage<TemperatureMessageArgs>;

#pragma pack(pop)
