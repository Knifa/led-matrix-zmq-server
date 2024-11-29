FROM alpine:latest as build

RUN apk add --no-cache \
  clang \
  cmake \
  cppzmq \
  git \
  make \
  pkgconf

ENV CC="clang"
ENV CXX="clang++"
ENV CFLAGS="-O3"
ENV CXXFLAGS="-O3"
ENV MAKEFLAGS="-j$(nproc)"

RUN mkdir -p /opt/lmz
WORKDIR /opt/lmz

RUN git clone --depth 1 https://github.com/knifa/rpi-rgb-led-matrix.git \
  && cd rpi-rgb-led-matrix/lib \
  && make

RUN mkdir -p /opt/lmz/led-matrix-zmq-server
WORKDIR /opt/lmz/led-matrix-zmq-server
COPY . .
RUN mkdir -p build \
  && cd build \
  && cmake -DRPI_RGB_LED_MATRIX_DIR=/opt/lmz/rpi-rgb-led-matrix .. \
  && make


FROM alpine:latest as run

RUN apk add --no-cache \
  libc++ \
  libzmq

RUN mkdir -p /opt/lmz
WORKDIR /opt/lmz/

COPY --from=build \
  /opt/lmz/led-matrix-zmq-server/build/led-matrix-zmq-* \
  ./

ENV PATH="/opt/lmz:$PATH"
CMD ["echo", "Please run a specific led-matrix-zmq binary!"]
