#pragma once

#include "event_queue.h"
#include <netinet/in.h>
#include <string>
#include <stop_token>
#include <thread>
#include <optional>

class OrchestratorTCP {
public:
	OrchestratorTCP(int port, EventQueue& queue);

	void start();

private:
	std::optional<DetectedEvent> jsonToStruct(const std::string& jsonLine);

	void pushToQueue(DetectedEvent& ev);

	std::string recieveJson();

    void receiveLoop();

	int port_;
	int listenFd_ = -1;
	sockaddr_in addr_;
	EventQueue& queue_;

};