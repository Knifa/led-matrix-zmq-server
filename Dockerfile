FROM arm32v7/debian:buster-slim as builder

RUN apt-get update \
  && apt-get -y install \
    build-essential \
    cmake \
    git \
    libzmq3-dev \
    libzmq5

COPY ./rpi-rgb-led-matrix /root/rpi-rgb-led-matrix
WORKDIR /root/rpi-rgb-led-matrix/lib
RUN make

WORKDIR /root
COPY ./Makefile /root/Makefile
COPY ./src /root/src
RUN make


FROM arm32v7/debian:buster-slim

WORKDIR /root

RUN apt-get update \
  && apt-get -y install \
    libzmq5

COPY --from=builder /root/bin/led-matrix-zmq-server ./led-matrix-zmq-server
RUN chmod +x ./led-matrix-zmq-server

EXPOSE 42042/tcp
ENTRYPOINT /root/led-matrix-zmq-server
