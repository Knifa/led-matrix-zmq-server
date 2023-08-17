#include <iostream>
#include <mutex>
#include <thread>

#include <led-matrix.h>
#include <zmq.hpp>

#include "messages.h"

const auto FRAME_ENDPOINT = "ipc:///var/run/matryx";
const auto CONTROL_ENDPOINT = "ipc:///var/run/matryx-control";

rgb_matrix::RGBMatrix::Options matrix_opts;
rgb_matrix::RuntimeOptions matrix_runtime_opts;
rgb_matrix::RGBMatrix *matrix;

std::mutex matrix_mutex;

void setup()
{
    matrix_opts.rows = 32;
    matrix_opts.cols = 64;
    matrix_opts.chain_length = 10;
    matrix_opts.parallel = 3;
    matrix_opts.pixel_mapper_config = "Z-mapper";
    matrix_opts.show_refresh_rate = 0;
    matrix_runtime_opts.daemon = 0;
    matrix_runtime_opts.drop_privileges = 0;

    // Stop fucking with these because they're set right, OK?
    matrix_opts.pwm_lsb_nanoseconds = 100;
    matrix_opts.pwm_bits = 9;
    matrix_opts.pwm_dither_bits = 2;
    matrix_opts.limit_refresh_rate_hz = 200;
    matrix_runtime_opts.gpio_slowdown = 4;

    const std::lock_guard<std::mutex> guard(matrix_mutex);
    matrix = rgb_matrix::CreateMatrixFromOptions(matrix_opts, matrix_runtime_opts);
    matrix->set_luminance_correct(true);
}

void frame_loop()
{
    const auto matrix_width = matrix->width();
    const auto matrix_height = matrix->height();
    const size_t expected_frame_size = matrix_width * matrix_height * 3;

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
        for (auto y = 0; y < matrix_height; ++y)
        {
            for (auto x = 0; x < matrix_width; ++x)
            {
                const auto data = static_cast<const uint8_t *>(req.data());
                const auto offset = (y * matrix_width + x) * 3;
                const auto r = data[offset + 0];
                const auto g = data[offset + 1];
                const auto b = data[offset + 2];

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
            if (msg->args.brightness < 1 || msg->args.brightness > 100)
            {
                std::cerr << "Received invalid brightness: " << std::to_string(msg->args.brightness) << std::endl;
                break;
            }

            std::cout << "Setting brightness to " << std::to_string(msg->args.brightness) << std::endl;
            matrix->SetBrightness(msg->args.brightness);
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
