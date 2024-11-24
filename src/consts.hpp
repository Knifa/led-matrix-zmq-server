#pragma once

#include <cstdint>

namespace consts {
constexpr auto pixel_size = sizeof(std::uint32_t);
constexpr auto bpp = 8 * pixel_size;

const auto default_control_endpoint = "ipc:///tmp/matrix-control.sock";
const auto default_frame_endpoint = "ipc:///tmp/matrix-frames.sock";
} // namespace consts
