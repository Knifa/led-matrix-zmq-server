#include <iostream>
#include <string>

#include <argparse/argparse.hpp>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <zmq.hpp>

#include "consts.hpp"
#include "messages.hpp"

template <lmz::IsMessage RecvType, lmz::IsMessage SendType>
const RecvType send_and_recv(zmq::socket_t &sock, const SendType &send_msg) {
  zmq::message_t zmq_req(&send_msg, sizeof(send_msg));
  sock.send(zmq_req, zmq::send_flags::none);

  zmq::message_t zmq_res;
  static_cast<void>(sock.recv(zmq_res, zmq::recv_flags::none));

  const auto data = std::span<const std::byte>(zmq_res.data<const std::byte>(), zmq_res.size());
  return lmz::get_message_from_data<RecvType>(data);
}

int main(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser program("led-matrix-zmq-control");
  program.add_description("Send control messages to led-matrix-zmq-server");
  program.add_argument("--control-endpoint").default_value(consts::DEFAULT_CONTROL_ENDPOINT);

  argparse::ArgumentParser set_brightness_command("set-brightness");
  set_brightness_command.add_description("Set the brightness");
  set_brightness_command.add_argument("brightness")
      .help("Brightness level (0%-100%)")
      .scan<'i', int>();
  argparse::ArgumentParser set_temperature_command("set-temperature");
  set_temperature_command.add_description("Set the color temperature");
  set_temperature_command.add_argument("temperature")
      .help("Temperature level (2000K-6500K)")
      .scan<'i', int>();

  argparse::ArgumentParser get_brightness_command("get-brightness");
  get_brightness_command.add_description("Get the brightness");
  argparse::ArgumentParser get_temperature_command("get-temperature");
  get_temperature_command.add_description("Get the color temperature");

  program.add_subparser(get_brightness_command);
  program.add_subparser(set_brightness_command);
  program.add_subparser(get_temperature_command);
  program.add_subparser(set_temperature_command);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  zmq::context_t ctx;
  zmq::socket_t sock(ctx, ZMQ_REQ);
  sock.set(zmq::sockopt::linger, 0);
  sock.set(zmq::sockopt::rcvtimeo, 1000);
  sock.set(zmq::sockopt::sndtimeo, 1000);
  sock.connect(program.get("--control-endpoint"));

  zmq::message_t zmq_req, zmq_res;

  if (program.is_subcommand_used(set_brightness_command)) {
    const auto brightness = set_brightness_command.get<int>("brightness");
    PLOG_INFO << "Setting brightness to " << brightness << "%";

    const lmz::SetBrightnessRequestMessage control_req = {
        .args = {.brightness = static_cast<uint8_t>(brightness)},
    };

    send_and_recv<lmz::NullResponseMessage>(sock, control_req);
  } else if (program.is_subcommand_used(set_temperature_command)) {
    const auto temperature = set_temperature_command.get<int>("temperature");
    PLOG_INFO << "Setting temperature to " << temperature << "K";

    const lmz::SetTemperatureRequestMessage control_req = {
        .args = {.temperature = static_cast<uint16_t>(temperature)},
    };

    send_and_recv<lmz::NullResponseMessage>(sock, control_req);
  } else if (program.is_subcommand_used(get_brightness_command)) {
    const auto resp_msg =
        send_and_recv<lmz::GetBrightnessResponseMessage>(sock, lmz::GetBrightnessRequestMessage{});

    PLOG_INFO << "Brightness: " << std::to_string(resp_msg.args.brightness) << "%";
  } else if (program.is_subcommand_used(get_temperature_command)) {
    const auto res_msg = send_and_recv<lmz::GetTemperatureResponseMessage>(
        sock, lmz::GetTemperatureRequestMessage{});

    PLOG_INFO << "Temperature: " << res_msg.args.temperature;
  } else {
    std::cerr << program;
    return 1;
  }

  return 0;
}
