#include "node_tcp.h"
#include "event_queue.h"

#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <stop_token>
#include <optional>

NodeTCP::NodeTCP(std::string serverIp, int serverPort, EventQueue& queue) : queue_(queue) {
	fd_ = socket(AF_INET, SOCK_STREAM, 0);

	addr_.sin_family = AF_INET; // IPv4
	addr_.sin_port = serverPort;
	inet_pton(AF_INET, serverIp.c_str(), &addr_.sin_addr);
}

void NodeTCP::start() {
	while (!running) {
		int connected = connect(fd_, (sockaddr*)&addr_, sizeof(addr_));
		if (connected < 0) {
			std::cout << "connection failed, retrying...\n";
		} else {
			std::cout << "Connected to orchestrator!\n";
			running = true;
		}
	}

	thread_ = std::jthread([this](std::stop_token st) { senderLoop(st); });

}

void NodeTCP::senderLoop(std::stop_token st) {
	while (running) {
		auto ev = queue_.pop(st);
		if (ev != std::nullopt) {

		}
	}

	// Connection lost, reconnect
	start();
}

void NodeTCP::sendAsJSON(DetectedEvent& ev) {
	
}