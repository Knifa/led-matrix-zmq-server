#pragma once

#include <cstdint>

namespace consts {
constexpr auto BPP = sizeof(std::uint32_t);

const auto DEFAULT_CONTROL_ENDPOINT = "ipc:///tmp/matrix-control.sock";
const auto DEFAULT_FRAME_ENDPOINT = "ipc:///tmp/matrix-frames.sock";
} // namespace consts
