FROM alpine:latest as build

RUN apk add --no-cache --virtual .build-deps-base \
    clang \
    cmake \
    cppzmq \
    git \
    make \
    pkgconf

ENV CC clang
ENV CXX clang++
ENV CFLAGS "-O3 -march=native -mtune=native"
ENV CXXFLAGS "-O3 -march=native -mtune=native"
ENV MAKEFLAGS "-j$(nproc)"

RUN mkdir -p /build
WORKDIR /build

RUN git clone --depth 1 https://github.com/Knifa/rpi-rgb-led-matrix.git
WORKDIR /build/rpi-rgb-led-matrix
RUN cd lib \
    && make

RUN mkdir -p /build/led-matrix-zmq-server
WORKDIR /build/led-matrix-zmq-server
ADD . .
RUN mkdir -p build \
    && cd build \
    && cmake -DRPI_RGB_LED_MATRIX_DIR=/build/rpi-rgb-led-matrix .. \
    && make

FROM alpine:latest as run

RUN apk add --no-cache \
    libc++ \
    libzmq

RUN mkdir -p /opt/led-matrix-zmq-server
WORKDIR /opt/led-matrix-zmq-server

COPY --from=build \
    /build/led-matrix-zmq-server/build/led-matrix-zmq-server \
    /build/led-matrix-zmq-server/build/led-matrix-zmq-control \
    /build/led-matrix-zmq-server/build/led-matrix-zmq-pipe \
    ./

CMD ["./led-matrix-zmq-server"]
