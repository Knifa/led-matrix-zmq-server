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
  --privileged \
  --rm \
  knifa/led-matrix-zmq-server \
    --led-brightness=50 \
    --led-cols=64 \
    --led-slowdown-gpio=2 \
    --led-pwm-lsb-nanoseconds=50 \
    --led-rows=32 \
    --zmq-endpoint=tcp://*:1337
```

The following server arguments are available:

- `--zmq-endpoint`: The ZMQ endpoint to listen on. Default is `tcp://*:42024`.
- `--bytes-per-pixel`: Number of *bytes* per pixel. Default is `3` (i.e., `24BPP`)

The remaining matrix arguments are passed directly to `rpi-rgb-led-matrix`. [Check out their README for details](https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/README.md).

### Sending Frames

The server is a simple ZMQ REQ-REP loop. All you need to do is send your frame as a big ol' byte chunk then wait for an empty message back.

Each frame should be in a BGR24 (or however large you passed into `--bytes-per-pixel`) format, with exact size depending on your matrix setup (i.e., whatever `rpi-rgb-led-matrix` says your final canvas size is.)

## License

GNU GPL v3. See [LICENSE](LICENSE).
