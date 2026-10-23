#pragma once

#include "event_queue.h"

#include <netinet/in.h>
#include <string>
#include <thread>

class NodeTCP {
public:

	NodeTCP(std::string serverIp, int serverPort, EventQueue& queue);

	void start();

private:

	void senderLoop(std::stop_token st);

	void sendAsJSON(DetectedEvent& ev);

	int fd_;
	sockaddr_in addr_;
	EventQueue& queue_;
	bool running = false;
	std::jthread thread_;
};