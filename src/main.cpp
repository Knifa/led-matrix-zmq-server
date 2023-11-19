#include <iostream>
#include <mutex>
#include <thread>

#include <led-matrix.h>
#include <zmq.hpp>

#include "color_temp.hpp"
#include "messages.h"

constexpr auto FRAME_ENDPOINT = "ipc:///tmp/matryx.sock";
constexpr auto CONTROL_ENDPOINT = "ipc:///tmp/matryx-control.sock";
constexpr auto BPP = 4;

rgb_matrix::RGBMatrix::Options matrix_opts;
rgb_matrix::RuntimeOptions matrix_runtime_opts;
rgb_matrix::RGBMatrix *matrix;

std::mutex matrix_mutex;

std::tuple<int, int, int> color_temp_current = {255, 255, 255};

void setup()
{
    matrix_opts.rows = 32;
    matrix_opts.cols = 64;
    matrix_opts.chain_length = 10;
    matrix_opts.parallel = 3;
    matrix_opts.pixel_mapper_config = "Z-mapper;Rotate:90";
    matrix_opts.show_refresh_rate = 0;
    matrix_runtime_opts.daemon = 0;
    matrix_runtime_opts.drop_privileges = 0;

    // Stop fucking with these because they're set right, OK?
    matrix_opts.pwm_lsb_nanoseconds = 100;
    matrix_opts.pwm_bits = 11;
    matrix_opts.pwm_dither_bits = 2;
    matrix_opts.limit_refresh_rate_hz = 180;
    matrix_runtime_opts.gpio_slowdown = 4;

    const std::lock_guard<std::mutex> guard(matrix_mutex);
    matrix = rgb_matrix::CreateMatrixFromOptions(matrix_opts, matrix_runtime_opts);
    matrix->set_luminance_correct(true);
}

void frame_loop()
{
    const auto matrix_width = matrix->width();
    const auto matrix_height = matrix->height();
    const size_t expected_frame_size = matrix_width * matrix_height * BPP;

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind(FRAME_ENDPOINT);

    while (true)
    {
        zmq::message_t req;
        static_cast<void>(sock.recv(req, zmq::recv_flags::none));

        zmq::message_t rep(0);
        sock.send(rep, zmq::send_flags::none);

        if (req.size() != expected_frame_size)
        {
            std::cerr << "Received frame of unexpected size: " << req.size() << std::endl;
            continue;
        }

        const std::lock_guard<std::mutex> guard(matrix_mutex);
        const auto data = static_cast<const uint32_t *>(req.data());
        for (auto y = 0; y < matrix_height; ++y)
        {
            for (auto x = 0; x < matrix_width; ++x)
            {
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

void control_loop()
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind(CONTROL_ENDPOINT);

    while (true)
    {
        zmq::message_t req;
        static_cast<void>(sock.recv(req, zmq::recv_flags::none));

        const auto data = static_cast<const uint8_t *>(req.data());
        const auto type = static_cast<ControlMessageType>(data[0]);

        const std::lock_guard<std::mutex> guard(matrix_mutex);
        switch (type)
        {
        case ControlMessageType::Brightness:
        {
            const BrightnessMessage *msg = reinterpret_cast<const BrightnessMessage *>(data);
            if (msg->args.brightness > 100)
            {
                std::cerr << "Received invalid brightness: " << std::to_string(msg->args.brightness) << std::endl;
                break;
            }

            std::cout << "Setting brightness to " << std::to_string(msg->args.brightness) << std::endl;
            matrix->SetBrightness(msg->args.brightness);
        }
        break;
        case ControlMessageType::Temperature:
        {
            const TemperatureMessage *msg = reinterpret_cast<const TemperatureMessage *>(data);
            if (msg->args.temperature < 2000 || msg->args.temperature > 6500)
            {
                std::cerr << "Received invalid temperature: " << std::to_string(msg->args.temperature) << std::endl;
                break;
            }

            std::cout << "Setting temperature to " << std::to_string(msg->args.temperature) << std::endl;

            const auto temperature = static_cast<float>(msg->args.temperature);
            color_temp_current = color_temp::get(temperature);
        }
        break;
        }

        zmq::message_t rep(0);
        sock.send(rep, zmq::send_flags::none);
    }
}

int main()
{
    setup();

    auto frame_thread = std::thread(frame_loop);
    auto control_thread = std::thread(control_loop);

    frame_thread.join();
    control_thread.join();

    return 0;
}
