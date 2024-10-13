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

### Sending Frames

The server is a simple ZMQ REQ-REP loop. All you need to do is send your frame as a big ol' byte chunk then wait for an empty message back.

Each frame should be in a BGR32 format.

### Control Messages

Brightness and color temperature can be controlled through another simple REQ-REP loop.

See `led-matrix-zmq-control --help` for available options, or see [the source](src/control_main.cpp) to dig deeper.


## License

GNU GPL v3. See [LICENSE](LICENSE).
