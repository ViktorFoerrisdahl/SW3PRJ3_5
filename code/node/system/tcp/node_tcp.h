#pragma once

#include "event_queue.h"

#include <netinet/in.h>
#include <string>
#include <stop_token>
#include <thread>

// Laver en DetectedEvent om til den besked der sendes til orchestrator:
// ét JSON-objekt efterfulgt af '\n', så orchestrator kan skille beskederne ad.
std::string eventToJSONMessage(const DetectedEvent& ev);

class NodeTCP {
public:

	NodeTCP(std::string serverIp, int serverPort, EventQueue& queue);

	void start();

private:

	void connectToOrchestrator(std::stop_token st);

	void senderLoop(std::stop_token st);

	void sendAsJSON(DetectedEvent& ev);

	int fd_ = -1;
	sockaddr_in addr_;
	EventQueue& queue_;
	bool running = false;
	std::jthread thread_;
};
