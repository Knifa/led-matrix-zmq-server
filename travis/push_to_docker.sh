#!/bin/bash
echo "$DOCKER_PASSWORD" \
  | docker login --username "$DOCKER_USERNAME" --password-stdin

docker push knifa/led-matrix-zmq-server
