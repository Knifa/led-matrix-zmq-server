#include <string>
#include <iostream>

#include <zmq.hpp>

#include "messages.h"

const auto CONTROL_ENDPOINT = "ipc:///var/run/matryx-control";

int main(int argc, char *argv[]) {

    if (argc < 2) {
        return 1;
    }

    const auto type = std::string(argv[1]);

    zmq::context_t ctx;
    zmq::socket_t socket(ctx, ZMQ_REQ);
    socket.connect(CONTROL_ENDPOINT);

    if (type == "brightness") {
        if (argc < 3) {
            return 1;
        }

        const auto brightness = std::stoi(argv[2]);
        std::cout << "brightness: " << brightness << std::endl;

        BrightnessMessage msg;
        msg.type = ControlMessageType::Brightness;
        msg.args.brightness = brightness;

        zmq::message_t req(sizeof(msg));
        std::memcpy(req.data(), &msg, sizeof(msg));

        socket.send(req, zmq::send_flags::none);
    } else if (type == "temperature") {
        if (argc < 3) {
            return 1;
        }

        const auto temperature = std::stoi(argv[2]);
        std::cout << "temperature: " << temperature << std::endl;

        TemperatureMessage msg;
        msg.type = ControlMessageType::Temperature;
        msg.args.temperature = temperature;

        zmq::message_t req(sizeof(msg));
        std::memcpy(req.data(), &msg, sizeof(msg));

        socket.send(req, zmq::send_flags::none);
    } else {
        return 1;
    }

    zmq::message_t rep;
    static_cast<void>(socket.recv(rep, zmq::recv_flags::none));

    return 0;
}
