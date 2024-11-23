#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace lmz {

enum class MessageId : std::uint8_t {
  NullReply,

  GetBrightnessRequest,
  GetBrightnessReply,
  SetBrightnessRequest,

  GetTemperatureRequest,
  GetTemperatureReply,
  SetTemperatureRequest,
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
  constexpr MessageId first_message_id = MessageId::NullReply;
  constexpr MessageId last_message_id = MessageId::GetTemperatureReply;

  template <MessageId Id, typename ArgsT = NullArgs> struct Message {
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

using NullReply = Message<MessageId::NullReply>;
using SetBrightnessRequest = Message<MessageId::SetBrightnessRequest, BrightnessArgs>;
using SetTemperatureRequest = Message<MessageId::SetTemperatureRequest, TemperatureArgs>;
using GetBrightnessRequest = Message<MessageId::GetBrightnessRequest>;
using GetBrightnessReply = Message<MessageId::GetBrightnessReply, BrightnessArgs>;
using GetTemperatureRequest = Message<MessageId::GetTemperatureRequest>;
using GetTemperatureReply = Message<MessageId::GetTemperatureReply, TemperatureArgs>;

namespace {
  template <IsMessage MessageT> struct MessageRequestReply {
    static_assert(false, "No reply type defined for this message");
  };

  template <> struct MessageRequestReply<SetBrightnessRequest> {
    using ReplyType = NullReply;
  };

  template <> struct MessageRequestReply<SetTemperatureRequest> {
    using ReplyType = NullReply;
  };

  template <> struct MessageRequestReply<GetBrightnessRequest> {
    using ReplyType = GetBrightnessReply;
  };

  template <> struct MessageRequestReply<GetTemperatureRequest> {
    using ReplyType = GetTemperatureReply;
  };
} // namespace

template <IsMessage RequestT>
using MessageReplyType = typename MessageRequestReply<RequestT>::ReplyType;

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
