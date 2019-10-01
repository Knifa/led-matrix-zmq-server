FROM arm32v7/alpine:latest as builder

RUN apk update \
  && apk add \
    build-base \
    czmq-dev \
    git \
    libzmq

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
  && apk add czmq

COPY --from=builder /root/bin/matryx-server ./matryx-server
RUN chmod +x ./matryx-server

EXPOSE 8182/tcp
ENTRYPOINT /root/matryx-server
