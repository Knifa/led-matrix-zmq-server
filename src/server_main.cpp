#include <cstdint>
#include <mutex>
#include <thread>

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

namespace frame_task {

static std::string frame_endpoint;

static std::recursive_mutex matrix_mutex;

static rgb_matrix::RGBMatrix *matrix;
static int matrix_width;
static int matrix_height;

static std::vector<std::byte> frame_buffer(0);

static int brightness_current;

static int color_temp_current_k;
static std::tuple<int, int, int> color_temp_current;

static void render_test_pattern() {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);

  std::vector<std::uint32_t> pixels(matrix_width * matrix_height);
  for (auto y = 0; y < matrix_height; ++y) {
    for (auto x = 0; x < matrix_width; ++x) {
      auto b = (y * 255) / matrix_height;
      auto g = (x * 255) / matrix_width;
      auto r = 255 - b;

      pixels[y * matrix_width + x] = (r << 0) | (g << 8) | (b << 16);
    }
  }

  std::span<const std::byte> data(reinterpret_cast<const std::byte *>(pixels.data()),
                                  pixels.size() * sizeof(std::uint32_t));
  frame_buffer.assign(data.begin(), data.end());
}

static void update_matrix() {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);

  std::span<const std::uint32_t> data(reinterpret_cast<const std::uint32_t *>(frame_buffer.data()),
                                      frame_buffer.size() / sizeof(std::uint32_t));

  for (auto y = 0; y < matrix_height; ++y) {
    for (auto x = 0; x < matrix_width; ++x) {
      const auto pixel = data[y * matrix_width + x];
      auto r = (pixel >> 0) & 0xFF;
      auto g = (pixel >> 8) & 0xFF;
      auto b = (pixel >> 16) & 0xFF;

      r = (r * brightness_current) / 255;
      g = (g * brightness_current) / 255;
      b = (b * brightness_current) / 255;

      r = (r * std::get<0>(color_temp_current)) / 255;
      g = (g * std::get<1>(color_temp_current)) / 255;
      b = (b * std::get<2>(color_temp_current)) / 255;

      matrix->SetPixel(x, y, r, g, b);
    }
  }
}

static void set_brightness(int brightness) {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);
  brightness_current = brightness;
  update_matrix();
}

static int get_brightness() {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);
  return brightness_current;
}

static void set_temperature(int temperature) {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);
  color_temp_current_k = temperature;
  color_temp_current = color_temp::get(temperature);
  update_matrix();
}

static int get_temperature() {
  const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);
  return color_temp_current_k;
}

static void loop() {
  zmq::context_t ctx;
  zmq::socket_t sock(ctx, zmq::socket_type::rep);
  sock.bind(frame_endpoint);

  PLOG_INFO << "Listening for frames on " << frame_endpoint;
  PLOG_INFO << "Expected frame size: " << frame_buffer.size() << " bytes" << " (" << matrix_width
            << "x" << matrix_height << "x" << consts::bpp << "bpp)";

  while (true) {
    zmq::message_t req;
    static_cast<void>(sock.recv(req, zmq::recv_flags::none));
    sock.send(zmq::message_t(), zmq::send_flags::none);

    if (req.size() != frame_buffer.size()) {
      PLOG_ERROR << "Received frame of unexpected size: " << req.size() << ", expected "
                 << frame_buffer.size();
      continue;
    }

    const std::lock_guard<std::recursive_mutex> guard(matrix_mutex);
    auto data = req.data<const std::byte>();
    frame_buffer.assign(data, data + req.size());
    update_matrix();
  }
}

} // namespace frame_task

namespace control_task {

static std::string control_endpoint;

zmq::context_t ctx;
zmq::socket_t sock(ctx, zmq::socket_type::rep);

template <lmz::IsMessage RequestT>
static lmz::MessageReplyType<RequestT> process_request(const RequestT &) {
  static_assert(false, "No process implementation for this message");
}

template <> lmz::GetBrightnessReply process_request(const lmz::GetBrightnessRequest &) {
  return lmz::GetBrightnessReply{
      .args = {.brightness = static_cast<uint8_t>(frame_task::get_brightness())},
  };
}

template <> lmz::NullReply process_request(const lmz::SetBrightnessRequest &req_msg) {
  PLOG_INFO << "Setting brightness to " << std::to_string(req_msg.args.brightness) << "";

  frame_task::set_brightness(req_msg.args.brightness);

  return lmz::NullReply{};
}

template <> lmz::GetTemperatureReply process_request(const lmz::GetTemperatureRequest &) {
  return lmz::GetTemperatureReply{
      .args = {.temperature = static_cast<uint16_t>(frame_task::get_temperature())},
  };
}

template <> lmz::NullReply process_request(const lmz::SetTemperatureRequest &req_msg) {
  if (req_msg.args.temperature < color_temp::min || req_msg.args.temperature > color_temp::max) {
    PLOG_ERROR << "Received invalid temperature: " << req_msg.args.temperature << "K";
    return lmz::NullReply{};
  }
  PLOG_INFO << "Setting temperature to " << std::to_string(req_msg.args.temperature) << "K";

  frame_task::set_temperature(req_msg.args.temperature);

  return lmz::NullReply{};
}

template <lmz::IsMessage RequestT> void process_message(const std::span<const std::byte> &data) {
  const auto req_msg = lmz::get_message_from_data<RequestT>(data);
  const auto reply = process_request<RequestT>(req_msg);
  sock.send(zmq::const_buffer(&reply, sizeof(reply)), zmq::send_flags::none);
}

static void loop() {
  sock.bind(control_endpoint);

  PLOG_INFO << "Listening for control messages on " << control_endpoint;

  while (true) {
    zmq::message_t req;
    static_cast<void>(sock.recv(req, zmq::recv_flags::none));

    const auto data = std::span<const std::byte>(req.data<const std::byte>(), req.size());
    const auto id = lmz::get_id_from_data(data);

    switch (id) {
    case lmz::MessageId::GetBrightnessRequest: {
      process_message<lmz::GetBrightnessRequest>(data);
    } break;
    case lmz::MessageId::SetBrightnessRequest: {
      process_message<lmz::SetBrightnessRequest>(data);
    } break;
    case lmz::MessageId::GetTemperatureRequest: {
      process_message<lmz::GetTemperatureRequest>(data);
    } break;
    case lmz::MessageId::SetTemperatureRequest: {
      process_message<lmz::SetTemperatureRequest>(data);
    } break;
    default: {
      PLOG_ERROR << "Received control message with invalid type";
    } break;
    }
  }
}

} // namespace control_task

static void setup(int argc, char *argv[]) {
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);

  argparse::ArgumentParser parser("led-matrix-zmq-server");

  parser.add_argument("--rows").default_value(32).scan<'i', int>();
  parser.add_argument("--cols").default_value(32).scan<'i', int>();
  parser.add_argument("--chain-length").default_value(1).scan<'i', int>();
  parser.add_argument("--parallel").default_value(1).scan<'i', int>();

  parser.add_argument("--pixel-mapper");
  parser.add_argument("--hardware-mapping");

  parser.add_argument("--pwm-lsb-ns").default_value(100).scan<'i', int>();
  parser.add_argument("--pwm-bits").default_value(11).scan<'i', int>();
  parser.add_argument("--pwm-dither-bits").default_value(2).scan<'i', int>();
  parser.add_argument("--gpio-slowdown").default_value(4).scan<'i', int>();

  parser.add_argument("--limit-hz").default_value(180).scan<'i', int>();
  parser.add_argument("--show-hz").default_value(false).implicit_value(true);

  parser.add_argument("--frame-endpoint").default_value(consts::default_frame_endpoint);
  parser.add_argument("--control-endpoint").default_value(consts::default_control_endpoint);

  parser.add_argument("--no-test-pattern").default_value(false).implicit_value(true);

  parser.parse_args(argc, argv);

  if (getuid() != 0) {
    PLOG_WARNING << "You probably want to run this as root.";
  }

  {
    using namespace frame_task;

    frame_endpoint = parser.get<std::string>("--frame-endpoint");

    static rgb_matrix::RGBMatrix::Options matrix_opts;
    static rgb_matrix::RuntimeOptions matrix_runtime_opts;

    matrix_opts.rows = parser.get<int>("--rows");
    matrix_opts.cols = parser.get<int>("--cols");
    matrix_opts.chain_length = parser.get<int>("--chain-length");
    matrix_opts.parallel = parser.get<int>("--parallel");

    if (parser.present("--pixel-mapper")) {
      static std::string matrix_opts_pixel_mapper =
          std::string(parser.get<std::string>("--pixel-mapper"));
      matrix_opts.pixel_mapper_config = matrix_opts_pixel_mapper.c_str();
    }

    if (parser.present("--hardware-mapping")) {
      static std::string matrix_opts_led_rgb_sequence =
          std::string(parser.get<std::string>("--hardware-mapping"));
      matrix_opts.hardware_mapping = matrix_opts_led_rgb_sequence.c_str();
    }

    matrix_opts.pwm_lsb_nanoseconds = parser.get<int>("--pwm-lsb-ns");
    matrix_opts.pwm_bits = parser.get<int>("--pwm-bits");
    matrix_opts.pwm_dither_bits = parser.get<int>("--pwm-dither-bits");

    matrix_opts.limit_refresh_rate_hz = parser.get<int>("--limit-hz");
    matrix_opts.show_refresh_rate = parser.get<bool>("--show-hz");

    matrix_runtime_opts.daemon = 0;
    matrix_runtime_opts.drop_privileges = 0;
    matrix_runtime_opts.gpio_slowdown = parser.get<int>("--gpio-slowdown");

    matrix = rgb_matrix::RGBMatrix::CreateFromOptions(matrix_opts, matrix_runtime_opts);
    matrix->set_luminance_correct(true);

    matrix_width = matrix->width();
    matrix_height = matrix->height();
    frame_buffer.resize(matrix_width * matrix_height * consts::bpp);

    brightness_current = 255;

    color_temp_current_k = color_temp::max;
    color_temp_current = color_temp::get(color_temp_current_k);

    if (!parser.get<bool>("--no-test-pattern")) {
      render_test_pattern();
      update_matrix();
    }
  }

  {
    using namespace control_task;
    control_endpoint = parser.get<std::string>("--control-endpoint");
  }
}

int main(int argc, char *argv[]) {
  setup(argc, argv);

  auto frame_thread = std::thread(frame_task::loop);
  auto control_thread = std::thread(control_task::loop);

  frame_thread.join();
  control_thread.join();

  return 0;
}
