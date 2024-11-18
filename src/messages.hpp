#pragma once

#include <cstdint>

namespace messages {

enum class ControlRequestType : uint8_t {
  SetBrightness,
  SetTemperature,
  GetBrightness,
  GetTemperature,
};

enum class ControlResponseType : uint8_t {
  Brightness,
  Temperature,
};

#pragma pack(push, 1)

template <typename T> struct ControlRequest {
  ControlRequestType type;
  T args;
};

template <typename T> struct ControlResponse {
  ControlResponseType type;
  T args;
};

struct NullArgs {};
struct BrightnessArgs {
  uint8_t brightness;
};
struct TemperatureArgs {
  uint16_t temperature;
};

using SetBrightnessRequest = ControlRequest<BrightnessArgs>;
using SetTemperatureRequest = ControlRequest<TemperatureArgs>;
using GetBrightnessRequest = ControlRequest<NullArgs>;
using GetTemperatureRequest = ControlRequest<NullArgs>;

using BrightnessResponse = ControlResponse<BrightnessArgs>;
using TemperatureResponse = ControlResponse<TemperatureArgs>;

#pragma pack(pop)

} // namespace messages
