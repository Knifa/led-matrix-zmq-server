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

int main(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser program("led-matrix-zmq-control");
  program.add_description("Send control messages to led-matrix-zmq-server");
  program.add_argument("--control-endpoint").default_value(consts::DEFAULT_CONTROL_ENDPOINT);

  argparse::ArgumentParser brightness_command("brightness");
  brightness_command.add_description("Set the brightness");
  brightness_command.add_argument("brightness").help("Brightness level (0%-100%)").scan<'i', int>();

  argparse::ArgumentParser temperature_command("temperature");
  temperature_command.add_description("Set the color temperature");
  temperature_command.add_argument("temperature")
      .help("Temperature level (2000K-6500K)")
      .scan<'i', int>();

  program.add_subparser(brightness_command);
  program.add_subparser(temperature_command);

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


  if (program.is_subcommand_used("brightness")) {
    const auto brightness = brightness_command.get<int>("brightness");
    PLOG_INFO << "Setting brightness to " << brightness << "%";

    BrightnessMessage msg;
    msg.type = ControlMessageType::Brightness;
    msg.args.brightness = brightness;

    zmq::message_t req(sizeof(msg));
    std::memcpy(req.data(), &msg, sizeof(msg));

    sock.send(req, zmq::send_flags::none);
  } else if (program.is_subcommand_used("temperature")) {
    const auto temperature = temperature_command.get<int>("temperature");
    PLOG_INFO << "Setting temperature to " << temperature << "K";

    TemperatureMessage msg;
    msg.type = ControlMessageType::Temperature;
    msg.args.temperature = temperature;

    zmq::message_t req(sizeof(msg));
    std::memcpy(req.data(), &msg, sizeof(msg));

    sock.send(req, zmq::send_flags::none);
  } else {
    std::cerr << program;
    return 1;
  }

  zmq::message_t rep;
  static_cast<void>(sock.recv(rep, zmq::recv_flags::none));

  return 0;
}
