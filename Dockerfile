FROM arm32v7/alpine:latest as builder

RUN apk update \
  && apk add \
    build-base \
    git \
    libzmq \
    libzmq-dev

COPY ./rpi-rgb-led-matrix /root/rpi-rgb-led-matrix
WORKDIR /root/rpi-rgb-led-matrix/lib
RUN make

WORKDIR /root
COPY ./Makefile /root/Makefile
COPY ./src /root/src
RUN make


FROM arm32v7/alpine:latest

WORKDIR /root

RUN apk update \
  && apk add libzmq

COPY --from=builder /root/bin/led-matrix-zmq-server ./led-matrix-zmq-server
RUN chmod +x ./led-matrix-zmq-server

EXPOSE 8182/tcp
ENTRYPOINT /root/led-matrix-zmq-server
