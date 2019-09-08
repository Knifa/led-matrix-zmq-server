#include <csignal>
#include <cstdlib>
#include <cmath>

#include <czmq.h>

#include <led-matrix.h>

volatile bool running = true;

void handle_interrupt(int signo)
{
    running = false;
}

int env_arg_atoi_or_zero(char *name)
{
    char *arg_str = getenv(name);
    if (arg_str == NULL)
    {
        return 0;
    }

    return atoi(arg_str);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_interrupt);
    std::signal(SIGTERM, handle_interrupt);

    rgb_matrix::RGBMatrix::Options led_options;
    rgb_matrix::RuntimeOptions runtime;

    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &led_options, &runtime))
    {
        rgb_matrix::PrintMatrixFlags(stdout);
        return 1;
    }

    rgb_matrix::RGBMatrix *matrix = CreateMatrixFromOptions(led_options, runtime);
    if (matrix == NULL)
    {
        return 1;
    }

    rgb_matrix::FrameCanvas *canvas = matrix->CreateFrameCanvas();

    zsock_t *zrep = zsock_new_rep("tcp://*:8182");

    while (running)
    {
        byte *image = NULL;
        size_t image_size;

        zsock_recv(zrep, "b", &image, &image_size);
        zsock_send(zrep, "z");

        for (int y = 0; y < 32; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                int r = image[y * 64 * 4 + x * 4 + 2];
                int g = image[y * 64 * 4 + x * 4 + 1];
                int b = image[y * 64 * 4 + x * 4 + 0];

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

        free(image);
    }

    delete matrix;
    zsock_destroy(&zrep);
    return 0;
}
