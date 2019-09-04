#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <czmq.h>
#include <led-matrix-c.h>


volatile bool running = true;


void handle_interrupt(int signo) {
	running = false;
}


int main(int argc, char *argv[]) {
	struct RGBLedMatrixOptions options;
	struct RGBLedMatrix* matrix;
	struct LedCanvas* canvas;

	zsock_t* zrep = zsock_new_rep("tcp://*:8182");

	signal(SIGINT, handle_interrupt);
	signal(SIGTERM, handle_interrupt);
	
	memset(&options, 0, sizeof(options));
	options.rows = 32;
	options.cols = 64;

	matrix = led_matrix_create_from_options(&options, &argc, &argv);
	if (matrix == NULL)
		return 1;

	canvas = led_matrix_create_offscreen_canvas(matrix);

	while (running) {
		byte* image = NULL;
		size_t image_size;

		zsock_recv(zrep, "b", &image, &image_size);
		zsock_send(zrep, "z");
		
		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < 64; x++) {
				int r = image[y * 64 * 4 + x * 4 + 2];
			       	int g = image[y * 64 * 4 + x * 4 + 1];
				int b = image[y * 64 * 4 + x * 4 + 0];

				r = (int) (pow(r / 255.0f, 2.2f) * 255.0f);
				g = (int) (pow(g / 255.0f, 2.2f) * 255.0f);
				b = (int) (pow(b / 255.0f, 2.2f) * 255.0f);

				led_canvas_set_pixel(
					canvas, 
					x, 
					y, 
					r,
					g,
					b
				);
			}
		}

		canvas = led_matrix_swap_on_vsync(matrix, canvas);
	
		free(image);
	}

	led_matrix_delete(matrix);
	zsock_destroy(&zrep);

	printf("Bye!\n");
	return 0;
}
