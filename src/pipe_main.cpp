#include <argparse/argparse.hpp>
#include <iostream>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <vector>
#include <zmq.hpp>

#include "consts.hpp"

int main(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser program("led-matrix-zmq-pipe");
  program.add_description("Reads raw frames from stdin and sends them to led-matrix-zmq-server");

  program.add_argument("-w", "--width").default_value(32).scan<'i', int>();
  program.add_argument("-h", "--height").default_value(32).scan<'i', int>();
  program.add_argument("-f", "--frame-endpoint").default_value(consts::DEFAULT_FRAME_ENDPOINT);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  int width = program.get<int>("--width");
  int height = program.get<int>("--height");
  std::string frame_endpoint = program.get<std::string>("--frame-endpoint");

  zmq::context_t context;
  zmq::socket_t socket(context, zmq::socket_type::req);
  socket.connect(frame_endpoint);

  size_t frame_size = width * height * consts::BPP;
  std::vector<char> frame(frame_size);

  PLOG_INFO << "Sending frames to " << frame_endpoint;
  PLOG_INFO << "Expected frame size: " << frame_size << " bytes" << " (" << width << "x" << height
            << "x" << consts::BPP << ")";

  while (std::cin.read(frame.data(), frame_size)) {
    if (std::cin.eof()) {
      break;
    }

    zmq::const_buffer buffer(frame.data(), frame_size);
    socket.send(buffer, zmq::send_flags::none);

    zmq::message_t reply;
    static_cast<void>(socket.recv(reply));

    frame.clear();
  }

  return 0;
}
