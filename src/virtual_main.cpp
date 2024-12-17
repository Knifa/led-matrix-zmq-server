#include <SDL_pixels.h>
#include <string>

#include <SDL.h>
#include <argparse/argparse.hpp>
#include <zmq.hpp>

#include "consts.hpp"

class Options {
public:
  std::string frame_endpoint;
  int width;
  int height;
  int scale;

  static Options from_args(int argc, char *argv[]) {
    argparse::ArgumentParser parser("led-matrix-zmq-virtual");

    parser.add_argument("--width").default_value(32).scan<'i', int>();
    parser.add_argument("--height").default_value(32).scan<'i', int>();
    parser.add_argument("--scale").default_value(-1).scan<'i', int>();
    parser.add_argument("--frame-endpoint").default_value(consts::default_frame_endpoint);

    parser.parse_args(argc, argv);

    return Options{
        .frame_endpoint = parser.get<std::string>("--frame-endpoint"),
        .width = parser.get<int>("--width"),
        .height = parser.get<int>("--height"),
        .scale = parser.get<int>("--scale"),
    };
    ;
  }
};
;

int main(int argc, char *argv[]) {
  const auto options = Options::from_args(argc, argv);

  SDL_Init(SDL_INIT_VIDEO);

  auto scale = options.scale;
  if (options.scale < 0) {
    SDL_Rect bounds;
    SDL_GetDisplayBounds(0, &bounds);

    bounds.w *= 0.8;
    bounds.h *= 0.8;

    scale = std::min(bounds.w / options.width, bounds.h / options.height);
  }

  const auto win_width = options.width * scale;
  const auto win_height = options.height * scale;

  auto *window = SDL_CreateWindow("led-matrix-zmq-virtual", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, win_width, win_height, 0);
  auto *renderer = SDL_CreateRenderer(window, -1, 0);
  auto *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                    options.width, options.height);

  zmq::context_t zmq_ctx;
  zmq::socket_t zmq_sock(zmq_ctx, zmq::socket_type::rep);
  zmq_sock.bind(options.frame_endpoint);

  auto running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    void *tex_pixels;
    int tex_pitch;
    SDL_LockTexture(texture, nullptr, &tex_pixels, &tex_pitch);

    zmq::message_t req;
    static_cast<void>(zmq_sock.recv(req, zmq::recv_flags::none));
    zmq_sock.send(zmq::message_t(), zmq::send_flags::none);

    if (req.size() != options.width * options.height * consts::pixel_size) {
      throw std::runtime_error("Invalid frame size");
    }

    std::memcpy(tex_pixels, req.data(), req.size());

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  zmq_sock.close();
  zmq_ctx.close();

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
