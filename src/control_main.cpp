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

template <lmz::IsMessage SendType>
const static lmz::MessageReplyType<SendType> send_and_recv(zmq::socket_t &sock,
                                                           const SendType &send_msg) {
  zmq::message_t zmq_req(&send_msg, sizeof(send_msg));
  sock.send(zmq_req, zmq::send_flags::none);

  zmq::message_t zmq_rep;
  static_cast<void>(sock.recv(zmq_rep, zmq::recv_flags::none));

  const auto data = std::span<const std::byte>(zmq_rep.data<const std::byte>(), zmq_rep.size());
  return lmz::get_message_from_data<lmz::MessageReplyType<SendType>>(data);
}

int main(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser program("led-matrix-zmq-control");
  program.add_description("Send control messages to led-matrix-zmq-server");
  program.add_argument("--control-endpoint").default_value(consts::default_control_endpoint);

  argparse::ArgumentParser set_brightness_command("set-brightness");
  set_brightness_command.add_description("Set the brightness");
  set_brightness_command.add_argument("brightness")
      .help("Brightness level (0-255)")
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

  argparse::ArgumentParser get_configuration_command("get-configuration");
  get_configuration_command.add_description("Get the configuration");

  program.add_subparser(get_brightness_command);
  program.add_subparser(set_brightness_command);
  program.add_subparser(get_temperature_command);
  program.add_subparser(set_temperature_command);
  program.add_subparser(get_configuration_command);

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

  zmq::message_t zmq_req, zmq_rep;

  if (program.is_subcommand_used(set_brightness_command)) {
    const auto brightness = set_brightness_command.get<int>("brightness");
    PLOG_INFO << "Setting brightness to " << brightness << "";

    const lmz::SetBrightnessRequest control_req = {
        .args = {.brightness = static_cast<uint8_t>(brightness)},
    };

    send_and_recv(sock, control_req);
  } else if (program.is_subcommand_used(set_temperature_command)) {
    const auto temperature = set_temperature_command.get<int>("temperature");
    PLOG_INFO << "Setting temperature to " << temperature << "K";

    const lmz::SetTemperatureRequest control_req = {
        .args = {.temperature = static_cast<uint16_t>(temperature)},
    };

    send_and_recv(sock, control_req);
  } else if (program.is_subcommand_used(get_brightness_command)) {
    const auto resp_msg = send_and_recv(sock, lmz::GetBrightnessRequest{});

    std::cout << std::to_string(resp_msg.args.brightness) << std::endl;
  } else if (program.is_subcommand_used(get_temperature_command)) {
    const auto res_msg = send_and_recv(sock, lmz::GetTemperatureRequest{});

    std::cout << std::to_string(res_msg.args.temperature) << std::endl;
  } else if (program.is_subcommand_used(get_configuration_command)) {
    const auto res_msg = send_and_recv(sock, lmz::GetConfigurationRequest{});

    std::cout << std::to_string(res_msg.args.width) << " " << std::to_string(res_msg.args.height)
              << std::endl;
  } else {
    std::cerr << program;
    return 1;
  }

  return 0;
}
