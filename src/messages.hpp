#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace lmz {

enum class MessageId : std::uint8_t {
  NullResponse,
  SetBrightnessRequest,
  SetTemperatureRequest,
  GetBrightnessRequest,
  GetBrightnessResponse,
  GetTemperatureRequest,
  GetTemperatureResponse,
};

#pragma pack(push, 1)

struct NullArgs {};

struct BrightnessArgs {
  uint8_t brightness;
};

struct TemperatureArgs {
  uint16_t temperature;
};

namespace {
  constexpr MessageId first_message_id = MessageId::NullResponse;
  constexpr MessageId last_message_id = MessageId::GetTemperatureResponse;

  template <MessageId Id, typename ArgsT> struct Message {
    const MessageId id = Id;
    ArgsT args;

    static constexpr auto id_value = Id;
    static constexpr auto size_value = sizeof(id) + sizeof(args);
  };
} // namespace

template <typename T>
concept IsMessage = requires {
  { T::id_value } -> std::convertible_to<MessageId>;
  { T::size_value } -> std::convertible_to<std::size_t>;
};

using NullResponseMessage = Message<MessageId::NullResponse, NullArgs>;
using SetBrightnessRequestMessage = Message<MessageId::SetBrightnessRequest, BrightnessArgs>;
using SetTemperatureRequestMessage = Message<MessageId::SetTemperatureRequest, TemperatureArgs>;
using GetBrightnessRequestMessage = Message<MessageId::GetBrightnessRequest, NullArgs>;
using GetBrightnessResponseMessage = Message<MessageId::GetBrightnessResponse, BrightnessArgs>;
using GetTemperatureRequestMessage = Message<MessageId::GetTemperatureRequest, NullArgs>;
using GetTemperatureResponseMessage = Message<MessageId::GetTemperatureResponse, TemperatureArgs>;

#pragma pack(pop)

inline MessageId get_id_from_data(const std::span<const std::byte> &data) {
  using UnderlyingType = std::underlying_type_t<MessageId>;

  if (data.size() < sizeof(UnderlyingType)) {
    throw std::runtime_error("Received empty control message");
  }

  UnderlyingType id_underlying = static_cast<UnderlyingType>(data[0]);
  if (id_underlying < static_cast<UnderlyingType>(first_message_id) ||
      id_underlying > static_cast<UnderlyingType>(last_message_id)) {
    throw std::runtime_error("Received control message with invalid type");
  }

  return static_cast<MessageId>(id_underlying);
}

template <IsMessage MessageT>
MessageT get_message_from_data(const std::span<const std::byte> &data) {
  if (data.size() != MessageT::size_value) {
    throw std::runtime_error("Received control message of unexpected size");
  }

  if (get_id_from_data(data) != MessageT::id_value) {
    throw std::runtime_error("Received control message with unexpected type");
  }

  MessageT msg;
  std::copy(data.begin(), data.end(), reinterpret_cast<std::byte *>(&msg));
  return msg;
}

} // namespace lmz
