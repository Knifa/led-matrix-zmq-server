#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <led-matrix.h>
#include <string>
#include <unistd.h>
#include <zmq.hpp>

class ServerOptions {
public:
  enum class Args { endpoint = 1, bytes_per_pixel };

  std::string endpoint = "tcp://*:42024";
  int bytes_per_pixel = 3;

  static ServerOptions from_args(int argc, char *argv[]) {
    ServerOptions server_options;

    static struct option long_opts[]{
        {"zmq-endpoint", required_argument, nullptr,
         static_cast<int>(ServerOptions::Args::endpoint)},
        {"bytes-per-pixel", required_argument, nullptr,
         static_cast<int>(ServerOptions::Args::bytes_per_pixel)}};

    int opt_code;
    int opt_index;
    while ((opt_code = getopt_long(argc, argv, "", long_opts, &opt_index)) !=
           -1) {
      switch (opt_code) {
      case static_cast<int>(ServerOptions::Args::endpoint):
        server_options.endpoint = std::string(optarg);
        break;

      case static_cast<int>(ServerOptions::Args::bytes_per_pixel):
        server_options.bytes_per_pixel = std::stoi(optarg);
        break;
      }
    }

    return server_options;
  }
};

volatile bool running = true;

void handle_interrupt(const int signo) { running = false; }

int main(int argc, char *argv[]) {
  std::signal(SIGINT, handle_interrupt);
  std::signal(SIGTERM, handle_interrupt);

  rgb_matrix::RGBMatrix::Options matrix_opts;
  rgb_matrix::RuntimeOptions runtime_opts;
  ParseOptionsFromFlags(&argc, &argv, &matrix_opts, &runtime_opts);

  auto matrix = CreateMatrixFromOptions(matrix_opts, runtime_opts);
  if (matrix == nullptr) {
    return 1;
  }

  matrix->set_luminance_correct(true);
  auto canvas = matrix->CreateFrameCanvas();

  ServerOptions server_options = ServerOptions::from_args(argc, argv);

  zmq::context_t zmq_ctx;
  zmq::socket_t zmq_sock(zmq_ctx, zmq::socket_type::rep);
  zmq_sock.bind(server_options.endpoint.c_str());
  std::cout << "led-matrix-zmq-server Listening on " << server_options.endpoint
            << " @ " << (server_options.bytes_per_pixel) * 8 << "BPP" << std::endl;

  size_t expected_frame_size =
      canvas->width() * canvas->height() * server_options.bytes_per_pixel;
  uint8_t frame[expected_frame_size];

  while (running) {
    size_t frame_size = zmq_sock.recv(&frame, expected_frame_size);
    zmq_sock.send(nullptr, 0);

    if (frame_size != expected_frame_size) {
      std::cout << "Frame size mismatch! Expected " << expected_frame_size
                << " but got " << frame_size << std::endl;
    } else {
      for (uint8_t y = 0; y < canvas->height(); y++) {
        for (uint8_t x = 0; x < canvas->width(); x++) {
          uint8_t r =
              frame[y * canvas->width() * server_options.bytes_per_pixel +
                    x * server_options.bytes_per_pixel + 0];
          uint8_t g =
              frame[y * canvas->width() * server_options.bytes_per_pixel +
                    x * server_options.bytes_per_pixel + 1];
          uint8_t b =
              frame[y * canvas->width() * server_options.bytes_per_pixel +
                    x * server_options.bytes_per_pixel + 2];

          canvas->SetPixel(x, y, r, g, b);
        }
      }

      canvas = matrix->SwapOnVSync(canvas);
    }
  }

  delete matrix;

  return 0;
}
