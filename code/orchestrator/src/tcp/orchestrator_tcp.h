#pragma once

#include "event_queue.h"
#include <netinet/in.h>
#include <string>
#include <stop_token>
#include <thread>
#include <optional>
#include <unistd.h>
#include <atomic>

class OrchestratorTCP
{
public:
	OrchestratorTCP(int port, EventQueue &queue);
	~OrchestratorTCP()
	{
		if (listenFd_ >= 0)
			close(listenFd_);
		for (auto &t : threads_)
		{
			if (t.joinable())
				t.join();
		}
	}

	void start();

private:
	std::optional<DetectedEvent> jsonToStruct(const std::string &jsonLine);

	void pushToQueue(DetectedEvent &ev);

	std::string recieveJson(int fd, std::string &buf);

	void receiveLoop(int fd, int threadId);

	int port_;
	int listenFd_ = -1;
	sockaddr_in addr_;
	EventQueue &queue_;
	std::thread threads_[2];
	std::atomic<bool> done_[2]{true, true};
};
