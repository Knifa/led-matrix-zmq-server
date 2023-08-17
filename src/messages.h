#pragma once

#include <cstdint>

enum class ControlMessageType
{
    Brightness,
};

struct BrightnessMessageArgs
{
    uint8_t brightness;
};

template <typename T>
struct ControlMessage
{
    ControlMessageType type;
    T args;
};

using BrightnessMessage = ControlMessage<BrightnessMessageArgs>;
