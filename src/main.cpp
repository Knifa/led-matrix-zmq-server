#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <iostream>

#include <czmq.h>

#include <led-matrix.h>

volatile bool running = true;

void handle_interrupt(const int signo)
{
    running = false;
}

int env_arg_atoi_or_default(const char *name, const int d = 0)
{
    char *arg_str = std::getenv(name);
    if (arg_str == NULL)
    {
        return d;
    }

    return std::atoi(arg_str);
}

const char *env_arg_or_default(const char *name, const char *d = NULL)
{
    char *arg_str = std::getenv(name);
    if (arg_str == NULL)
    {
        return d;
    }

    return arg_str;
}

void set_options_from_env(rgb_matrix::RGBMatrix::Options *matrix_opts, rgb_matrix::RuntimeOptions *runtime_opts)
{
    matrix_opts->brightness = env_arg_atoi_or_default("MATRIX_BRIGHTNESS", matrix_opts->brightness);
    matrix_opts->chain_length = env_arg_atoi_or_default("MATRIX_CHAIN_LENGTH", matrix_opts->chain_length);
    matrix_opts->cols = env_arg_atoi_or_default("MATRIX_COLS", matrix_opts->cols);
    matrix_opts->disable_hardware_pulsing = env_arg_atoi_or_default("MATRIX_DISABLE_HARDWARE_PULSING", matrix_opts->disable_hardware_pulsing);
    matrix_opts->hardware_mapping = env_arg_or_default("MATRIX_HARDWARE_MAPPING", matrix_opts->hardware_mapping);
    matrix_opts->inverse_colors = env_arg_atoi_or_default("MATRIX_INVERSE_COLORS", matrix_opts->inverse_colors);
    matrix_opts->led_rgb_sequence = env_arg_or_default("MATRIX_LED_SEQUENCE", matrix_opts->led_rgb_sequence);
    matrix_opts->multiplexing = env_arg_atoi_or_default("MATRIX_MULTIPLEXING", matrix_opts->multiplexing);
    matrix_opts->panel_type = env_arg_or_default("MATRIX_PANEL_TYPE", matrix_opts->panel_type);
    matrix_opts->parallel = env_arg_atoi_or_default("MATRIX_PARALLEL", matrix_opts->parallel);
    matrix_opts->pixel_mapper_config = env_arg_or_default("MATRIX_PIXEL_MAPPER_CONFIG", matrix_opts->pixel_mapper_config);
    matrix_opts->pwm_bits = env_arg_atoi_or_default("MATRIX_PWM_BITS", matrix_opts->pwm_bits);
    matrix_opts->pwm_dither_bits = env_arg_atoi_or_default("MATRIX_PWM_DITHER_BITS", matrix_opts->pwm_dither_bits);
    matrix_opts->pwm_lsb_nanoseconds = env_arg_atoi_or_default("MATRIX_PWM_LSB_NANOSECONDS", matrix_opts->pwm_lsb_nanoseconds);
    matrix_opts->row_address_type = env_arg_atoi_or_default("MATRIX_ROW_ADDRESS_TYPE", matrix_opts->row_address_type);
    matrix_opts->rows = env_arg_atoi_or_default("MATRIX_ROWS", matrix_opts->rows);
    matrix_opts->scan_mode = env_arg_atoi_or_default("MATRIX_SCAN_MODE", matrix_opts->scan_mode);
    matrix_opts->show_refresh_rate = env_arg_atoi_or_default("MATRIX_SHOW_REFRESH_RATE", matrix_opts->show_refresh_rate);

    runtime_opts->daemon = env_arg_atoi_or_default("MATRIX_DAEMON", runtime_opts->daemon);
    runtime_opts->gpio_slowdown = env_arg_atoi_or_default("MATRIX_GPIO_SLOWDOWN", runtime_opts->gpio_slowdown);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_interrupt);
    std::signal(SIGTERM, handle_interrupt);

    rgb_matrix::RGBMatrix::Options matrix_opts;
    rgb_matrix::RuntimeOptions runtime_opts;
    set_options_from_env(&matrix_opts, &runtime_opts);

    auto matrix = CreateMatrixFromOptions(matrix_opts, runtime_opts);
    if (matrix == NULL)
    {
         return 1;
    }

    auto canvas = matrix->CreateFrameCanvas();
    size_t expected_image_size = canvas->width() * canvas->height() * 3;

    zsock_t *zrep = zsock_new_rep("tcp://*:8182");
    std::cout << "led-matrix-zmq-server Listening on tcp://*:8182" << std::endl;

    while (running)
    {
        byte *image = NULL;
        size_t image_size;

        zsock_recv(zrep, "b", &image, &image_size);
        zsock_send(zrep, "z");

        if (image_size != expected_image_size) {
            std::cout << "Image size mismatch! Expected " << expected_image_size << " but got " << image_size << std::endl;
        } else {
            for (uint8_t y = 0; y < canvas->height(); y++)
            {
                for (uint8_t x = 0; x < canvas->width(); x++)
                {
                    uint8_t r = image[y * canvas->width() * 3 + x * 3 + 0];
                    uint8_t g = image[y * canvas->width() * 3 + x * 3 + 1];
                    uint8_t b = image[y * canvas->width() * 3 + x * 3 + 2];

                    r = (uint8_t)(pow(r / 255.0f, 2.2f) * 255.0f);
                    g = (uint8_t)(pow(g / 255.0f, 2.2f) * 255.0f);
                    b = (uint8_t)(pow(b / 255.0f, 2.2f) * 255.0f);

                    canvas->SetPixel(
                        x,
                        y,
                        r,
                        g,
                        b);
                }
            }

            canvas = matrix->SwapOnVSync(canvas);
        }

        std::free(image);
    }

    delete matrix;
    zsock_destroy(&zrep);
    return 0;
}
