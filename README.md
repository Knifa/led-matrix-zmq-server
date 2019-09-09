# led-matrix-zmq-server

[![Build Status](https://travis-ci.org/Knifa/led-matrix-zmq-server.svg?branch=master)](https://travis-ci.org/Knifa/led-matrix-zmq-server)

A tool for streaming frames to [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix/) over ZeroMQ.

Available as a Docker image at [knifa/led-matrix-zmq-server](https://hub.docker.com/r/knifa/led-matrix-zmq-server).

## Building

In addition to `rpi-rgb-led-matrix`, requires `czmq`.

## Usage

You can run the image with e.g., the following command. Mostly, you MUST pass `--privileged` as access to `/dev/mem` is required.

```bash
docker run \
  -e MATRIX_CHAIN_LENGTH=4 \
  -e MATRIX_COLS=64 \
  -e MATRIX_MAPPER="U-Mapper" \
  -e MATRIX_ROWS=64 \
  -P \
  --privileged \
  --rm \
  knifa/led-matrix-zmq-server
```

The usual command line arguments for `rpi-rgb-led-matrix` are exposed as envrionment variables. Some that don't really make sense for a container (i.e., `--led-daemon`) aren't exposed. [Check out their README for details](https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/README.md).

| ENV                             | CLI                             |
| ------------------------------- | ------------------------------- |
| MATRIX_BRIGHTNESS               | --led-brightness                |
| MATRIX_CHAIN_LENGTH             | --led-chain-length              |
| MATRIX_COLS                     | --led-cols                      |
| MATRIX_DISABLE_HARDWARE_PULSING | --led-disable-hardware-pulsing  |
| MATRIX_HARDWARE_MAPPING         | --led-hardware-mapping          |
| MATRIX_LED_SEQUENCE             | --led-rgb-sequence              |
| MATRIX_MULTIPLEXING             | --led-multiplexing              |
| MATRIX_PANEL_TYPE               | --led-panel-type                |
| MATRIX_PARALLEL                 | --led-parallel                  |
| MATRIX_PIXEL_MAPPER_CONFIG      | --led-pixel-mapper              |
| MATRIX_PWM_BITS                 | --led-pwm-bits                  |
| MATRIX_PWM_DITHER_BITS          | --led-pwm-dither-bits           |
| MATRIX_PWM_LSB_NANOSECONDS      | --led-pwm-lsb-nanoseconds       |
| MATRIX_ROW_ADDRESS_TYPE         | --led-row-addr-type             |
| MATRIX_ROWS                     | --led-rows                      |
| MATRIX_SCAN_MODE                | --led-scan-mode                 |
| MATRIX_GPIO_SLOWDOWN            | --led-slowdown-gpio             |

### Sending Frames

The server is a simple REQ-REP loop. All you need to do is send your frame as a big ol' byte chunk then wait for an empty message back.

Each frame should be in a BGR32 format, with exact size depending on your matrix setup (i.e., whatever `rpi-rgb-led-matrix` says your final canvas size is.)


## License

GNU GPL v3. See [LICENSE](LICENSE).
