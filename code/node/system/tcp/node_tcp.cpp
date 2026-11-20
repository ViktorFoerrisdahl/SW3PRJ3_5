#include "node_tcp.h"
#include "event_queue.h"

#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <stop_token>
#include <optional>
#include <cerrno>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <json_dto/pub.hpp>

std::string eventToJSONMessage(const DetectedEvent& ev) {
	std::string msg = json_dto::to_json(ev);
	msg += '\n';
	return msg;
}

NodeTCP::NodeTCP(std::string serverIp, int serverPort, EventQueue& queue) : queue_(queue) {
	addr_ = {};
	addr_.sin_family = AF_INET; // IPv4
	addr_.sin_port = htons(static_cast<uint16_t>(serverPort)); // porten skal være i network byte order
	inet_pton(AF_INET, serverIp.c_str(), &addr_.sin_addr);
}

void NodeTCP::start() {
	thread_ = std::jthread([this](std::stop_token st) { senderLoop(st); });
}

void NodeTCP::connectToOrchestrator(std::stop_token st) {
	while (!running && !st.stop_requested()) {
		// En socket kan ikke genbruges efter et fejlet connect(), så der laves en ny hver gang
		fd_ = socket(AF_INET, SOCK_STREAM, 0);
		if (fd_ >= 0 && connect(fd_, (sockaddr*)&addr_, sizeof(addr_)) == 0) {
			std::cout << "Connected to orchestrator!\n";
			running = true;
		} else {
			std::cout << "connection failed, retrying...\n";
			if (fd_ >= 0) {
				close(fd_);
				fd_ = -1;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

void NodeTCP::senderLoop(std::stop_token st) {
	while (!st.stop_requested()) {
		connectToOrchestrator(st);

		while (running && !st.stop_requested()) {
			auto ev = queue_.pop(st);
			if (ev != std::nullopt) {
				sendAsJSON(*ev);
			}
		}

		// Forbindelsen er tabt (eller der er bedt om stop): luk socket og forbind igen
		if (fd_ >= 0) {
			close(fd_);
			fd_ = -1;
		}
		running = false;
	}
}

void NodeTCP::sendAsJSON(DetectedEvent& ev) {
	const std::string msg = eventToJSONMessage(ev);

	// send() kan sende færre bytes end bedt om, så der sendes i en løkke
	// indtil hele beskeden er afsendt.
	std::size_t sent = 0;
	while (sent < msg.size()) {
		ssize_t n = send(fd_, msg.data() + sent, msg.size() - sent, MSG_NOSIGNAL);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			std::cout << "send failed, connection lost\n";
			running = false;
			return;
		}
		sent += static_cast<std::size_t>(n);
	}
}
