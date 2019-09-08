#include <csignal>
#include <cstdlib>
#include <cmath>

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

    runtime_opts->gpio_slowdown = env_arg_atoi_or_default("MATRIX_GPIO_SLOWDOWN", runtime_opts->gpio_slowdown);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_interrupt);
    std::signal(SIGTERM, handle_interrupt);

    rgb_matrix::RGBMatrix::Options matrix_opts;
    rgb_matrix::RuntimeOptions runtime_opts;
    set_options_from_env(&matrix_opts, &runtime_opts);

    if (!rgb_matrix::ParseOptionsFromFlags(NULL, NULL, &matrix_opts, &runtime_opts))
    {
        rgb_matrix::PrintMatrixFlags(stdout);
        return 1;
    }

    auto matrix = CreateMatrixFromOptions(matrix_opts, runtime_opts);
    if (matrix == NULL)
    {
        return 1;
    }

    auto canvas = matrix->CreateFrameCanvas();

    zsock_t *zrep = zsock_new_rep("tcp://*:8182");

    while (running)
    {
        byte *image = NULL;
        size_t image_size;

        zsock_recv(zrep, "b", &image, &image_size);
        zsock_send(zrep, "z");

        for (int y = 0; y < canvas->height(); y++)
        {
            for (int x = 0; x < canvas->width(); x++)
            {
                int r = image[y * canvas->height() * 4 + x * 4 + 2];
                int g = image[y * canvas->height() * 4 + x * 4 + 1];
                int b = image[y * canvas->height() * 4 + x * 4 + 0];

                r = (int)(pow(r / 255.0f, 2.2f) * 255.0f);
                g = (int)(pow(g / 255.0f, 2.2f) * 255.0f);
                b = (int)(pow(b / 255.0f, 2.2f) * 255.0f);

                canvas->SetPixel(
                    x,
                    y,
                    r,
                    g,
                    b);
            }
        }

        canvas = matrix->SwapOnVSync(canvas);
        delete image;
    }

    delete matrix;
    zsock_destroy(&zrep);
    return 0;
}
