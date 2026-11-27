#pragma once

#include "event_queue.h"
#include "ptp_status.h"

#include <cstdint>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <stop_token>
#include <thread>

class PtpService;

// Laver en DetectedEvent om til den besked der sendes til orchestrator:
// ét JSON-objekt efterfulgt af '\n', så orchestrator kan skille beskederne ad.
std::string eventToJSONMessage(const DetectedEvent& ev);
std::string ptpStatusToJSONMessage(const PTP_STATUS& status);

class NodeTCP {
public:

	NodeTCP(std::string serverIp, int serverPort, EventQueue& queue,
			std::uint16_t node_id = 0, const PtpService* ptp = nullptr);
	~NodeTCP();

	void start();

	void stop();

private:

	void connectToOrchestrator(std::stop_token st);

	void senderLoop(std::stop_token st);

	void statusLoop(std::stop_token st);

	std::optional<PTP_STATUS> latestStatus(std::uint64_t& version) const;

	bool sendAsJSON(const DetectedEvent& ev);

	bool sendAsJSON(const PTP_STATUS& status);

	bool sendMessage(const std::string& message);

	int fd_ = -1;
	sockaddr_in addr_;
	EventQueue& queue_;
	std::uint16_t node_id_;
	const PtpService* ptp_;
	mutable std::mutex status_mutex_;
	std::optional<PTP_STATUS> latest_status_;
	std::uint64_t status_version_ = 0;
	bool running = false;
	std::jthread status_thread_;
	std::jthread sender_thread_;
};
