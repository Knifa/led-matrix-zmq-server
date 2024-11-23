#include <mutex>
#include <thread>
#include <unistd.h>

#include <argparse/argparse.hpp>
#include <led-matrix.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <zmq.hpp>

#include "color_temp.hpp"
#include "consts.hpp"
#include "messages.hpp"

int max_brightness = 100;
std::string frame_endpoint;
std::string control_endpoint;

rgb_matrix::RGBMatrix::Options matrix_opts;
rgb_matrix::RuntimeOptions matrix_runtime_opts;
rgb_matrix::RGBMatrix *matrix;
std::string matrix_opts_pixel_mapper;

std::mutex matrix_mutex;
int color_temp_current_k = 6500;
std::tuple<int, int, int> color_temp_current = {255, 255, 255};

void setup(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser parser("led-matrix-zmq-server");

  parser.add_argument("--rows").default_value(32).scan<'i', int>();
  parser.add_argument("--cols").default_value(32).scan<'i', int>();
  parser.add_argument("--chain-length").default_value(1).scan<'i', int>();
  parser.add_argument("--parallel").default_value(1).scan<'i', int>();
  parser.add_argument("--pixel-mapper");

  parser.add_argument("--pwm-lsb-ns").default_value(100).scan<'i', int>();
  parser.add_argument("--pwm-bits").default_value(11).scan<'i', int>();
  parser.add_argument("--pwm-dither-bits").default_value(2).scan<'i', int>();
  parser.add_argument("--gpio-slowdown").default_value(4).scan<'i', int>();

  parser.add_argument("--limit-hz").default_value(180).scan<'i', int>();
  parser.add_argument("--show-hz").default_value(false).implicit_value(true);

  parser.add_argument("--max-brightness")
      .help("Maximum brightness level (0-100)")
      .default_value(100)
      .scan<'i', int>();

  parser.add_argument("--frame-endpoint").default_value(consts::DEFAULT_FRAME_ENDPOINT);
  parser.add_argument("--control-endpoint").default_value(consts::DEFAULT_CONTROL_ENDPOINT);

  parser.parse_args(argc, argv);

  if (getuid() != 0) {
    PLOG_WARNING << "You probably want to run this as root.";
  }

  matrix_opts.rows = parser.get<int>("--rows");
  matrix_opts.cols = parser.get<int>("--cols");
  matrix_opts.chain_length = parser.get<int>("--chain-length");
  matrix_opts.parallel = parser.get<int>("--parallel");

  if (parser.present("--pixel-mapper")) {
    matrix_opts_pixel_mapper = parser.get<std::string>("--pixel-mapper");
    matrix_opts.pixel_mapper_config = matrix_opts_pixel_mapper.c_str();
  }

  matrix_opts.pwm_lsb_nanoseconds = parser.get<int>("--pwm-lsb-ns");
  matrix_opts.pwm_bits = parser.get<int>("--pwm-bits");
  matrix_opts.pwm_dither_bits = parser.get<int>("--pwm-dither-bits");

  matrix_opts.limit_refresh_rate_hz = parser.get<int>("--limit-hz");
  matrix_opts.show_refresh_rate = parser.get<bool>("--show-hz");

  max_brightness = parser.get<int>("--max-brightness");
  matrix_opts.brightness = max_brightness;

  frame_endpoint = parser.get<std::string>("--frame-endpoint");
  control_endpoint = parser.get<std::string>("--control-endpoint");

  matrix_runtime_opts.daemon = 0;
  matrix_runtime_opts.drop_privileges = 0;
  matrix_runtime_opts.gpio_slowdown = parser.get<int>("--gpio-slowdown");

  const std::lock_guard<std::mutex> guard(matrix_mutex);
  matrix = rgb_matrix::CreateMatrixFromOptions(matrix_opts, matrix_runtime_opts);
  matrix->set_luminance_correct(true);
}

void frame_loop() {
  const auto matrix_width = matrix->width();
  const auto matrix_height = matrix->height();
  const std::size_t expected_frame_size = matrix_width * matrix_height * consts::BPP;

  zmq::context_t ctx;
  zmq::socket_t sock(ctx, zmq::socket_type::rep);
  sock.bind(frame_endpoint);

  PLOG_INFO << "Listening for frames on " << frame_endpoint;
  PLOG_INFO << "Expected frame size: " << expected_frame_size << " bytes" << " (" << matrix_width
            << "x" << matrix_height << "x" << consts::BPP << ")";

  while (true) {
    zmq::message_t req;
    static_cast<void>(sock.recv(req, zmq::recv_flags::none));

    zmq::message_t rep(0);
    sock.send(rep, zmq::send_flags::none);

    if (req.size() != expected_frame_size) {
      PLOG_ERROR << "Received frame of unexpected size: " << req.size();
      continue;
    }

    const std::lock_guard<std::mutex> guard(matrix_mutex);
    const auto data = static_cast<const uint32_t *>(req.data());
    for (auto y = 0; y < matrix_height; ++y) {
      for (auto x = 0; x < matrix_width; ++x) {
        const auto pixel = data[y * matrix_width + x];
        auto r = (pixel >> 0) & 0xFF;
        auto g = (pixel >> 8) & 0xFF;
        auto b = (pixel >> 16) & 0xFF;

        r = (r * std::get<0>(color_temp_current)) / 255;
        g = (g * std::get<1>(color_temp_current)) / 255;
        b = (b * std::get<2>(color_temp_current)) / 255;

        matrix->SetPixel(x, y, r, g, b);
      }
    }
  }
}

namespace control {

template <lmz::IsMessage RequestT>
lmz::MessageReplyType<RequestT> process_request(const RequestT &) {
  static_assert(false, "No process implementation for this message");
}

template <> lmz::GetBrightnessReply process_request(const lmz::GetBrightnessRequest &) {
  return lmz::GetBrightnessReply{
      .args = {.brightness = static_cast<uint8_t>((matrix->brightness() * max_brightness) / 100)},
  };
}

template <> lmz::NullReply process_request(const lmz::SetBrightnessRequest &req_msg) {
  if (req_msg.args.brightness > 100) {
    PLOG_ERROR << "Received invalid brightness: " << std::to_string(req_msg.args.brightness) << "%";
    return lmz::NullReply{};
  }

  int scaled_brightness = (req_msg.args.brightness * max_brightness) / 100;
  PLOG_INFO << "Setting brightness to " << std::to_string(req_msg.args.brightness) << "% ("
            << std::to_string(scaled_brightness) << ")";
  matrix->SetBrightness(scaled_brightness);

  return lmz::NullReply{};
}

template <> lmz::GetTemperatureReply process_request(const lmz::GetTemperatureRequest &) {
  return lmz::GetTemperatureReply{
      .args = {.temperature = static_cast<uint16_t>(color_temp_current_k)},
  };
}

template <> lmz::NullReply process_request(const lmz::SetTemperatureRequest &req_msg) {
  if (req_msg.args.temperature < 2000 || req_msg.args.temperature > 6500) {
    PLOG_ERROR << "Received invalid temperature: " << req_msg.args.temperature << "K";
    return lmz::NullReply{};
  }

  PLOG_INFO << "Setting temperature to " << std::to_string(req_msg.args.temperature) << "K";

  color_temp_current_k = req_msg.args.temperature;
  color_temp_current = color_temp::get(static_cast<int>(color_temp_current_k));

  return lmz::NullReply{};
}

template <lmz::IsMessage RequestT>
void process_message(zmq::socket_t &sock, const std::span<const std::byte> &data) {
  const auto req_msg = lmz::get_message_from_data<RequestT>(data);
  const std::lock_guard<std::mutex> guard(matrix_mutex);
  const auto reply = process_request<RequestT>(req_msg);
  sock.send(zmq::const_buffer(&reply, sizeof(reply)), zmq::send_flags::none);
}

void loop() {
  zmq::context_t ctx;
  zmq::socket_t sock = zmq::socket_t(ctx, zmq::socket_type::rep);
  sock.bind(control_endpoint);

  PLOG_INFO << "Listening for control messages on " << control_endpoint;

  while (true) {
    zmq::message_t zmq_req;
    static_cast<void>(sock.recv(zmq_req, zmq::recv_flags::none));

    const auto data = std::span<const std::byte>(zmq_req.data<const std::byte>(), zmq_req.size());
    const auto id = lmz::get_id_from_data(data);

    switch (id) {
    case lmz::MessageId::GetBrightnessRequest: {
      process_message<lmz::GetBrightnessRequest>(sock, data);
    } break;
    case lmz::MessageId::SetBrightnessRequest: {
      process_message<lmz::SetBrightnessRequest>(sock, data);
    } break;

    case lmz::MessageId::SetTemperatureRequest: {
      process_message<lmz::SetTemperatureRequest>(sock, data);
    } break;
    case lmz::MessageId::GetTemperatureRequest: {
      process_message<lmz::GetTemperatureRequest>(sock, data);
    } break;
    default: {
      PLOG_ERROR << "Received control message with invalid type";
    } break;
    }
  }
}

} // namespace control

int main(int argc, char *argv[]) {
  setup(argc, argv);

  auto frame_thread = std::thread(frame_loop);
  auto control_thread = std::thread(control::loop);

  frame_thread.join();
  control_thread.join();

  return 0;
}
