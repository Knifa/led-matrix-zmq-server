# led-matrix-zmq-server

A tool for interacting with [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix/) over ZeroMQ.

Looking for the ye olde version? Check out the [2019 tag](https://github.com/Knifa/led-matrix-zmq-server/tree/2019).

## Building

- Depends on `cppzmq` and friends.
- Requires a built [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix/). Tell CMake where it is with `-DRPI_RGB_LED_MATRIX_DIR`.
- Build with `cmake`, e.g.:

  ```shell
  mkdir build
  cd build
  cmake -DRPI_RGB_LED_MATRIX_DIR=/path/to/rpi-rgb-led-matrix ..
  make
  ```

## Usage

### Running the Server

See `led-matrix-zmq-server --help` for available options. These mostly passthrough to the `rpi-rgb-led-matrix` library.

```shell
sudo ./led-matrix-zmq-server \
  --cols 64 \
  --rows 64 \
  --chain-length 2 \
  --max-brightness 50 \
  # ...etc
```

### Sending Frames

The server is a simple ZMQ REQ-REP loop. All you need to do is send your frame as a big ol' byte chunk then wait for an empty message back. Each frame should be in a RGBA32 format.

#### Just Pipe It

[led-matrix-zmq-pipe](src/pipe_main.cpp) is both a bit of an example and a handy tool. It reads raw RGBA32 frames from stdin and sends them to the server.

```shell
# Use ffmpeg to convert a video to raw data.
ffmpeg -re -i input.mp4 -vf scale=128:64 -f rawvideo -pix_fmt rgba - \
  | sudo ./led-matrix-zmq-pipe -w 128 -h 64

# Use imagemagick to resize and convert an image to raw data.
convert input.png -resize 128x64^ rgba:- \
  | sudo ./led-matrix-zmq-pipe -w 128 -h 64

# Play a YouTube video.
yt-dlp -f "bv*[height<=480]" "https://www.youtube.com/watch?v=FtutLA63Cp8" -o - \
  | ffmpeg -re -i pipe: -vf scale=128:64 -f rawvideo -pix_fmt rgba - \
  | sudo ./led-matrix-zmq-pipe -w 128 -h 64
```

### Control Messages

Brightness and color temperature can be get/set through another simple REQ-REP loop.

See `led-matrix-zmq-control --help` for available options, or see [the source](src/control_main.cpp) to dig deeper.


## License

GNU GPL v3. See [LICENSE](LICENSE).
