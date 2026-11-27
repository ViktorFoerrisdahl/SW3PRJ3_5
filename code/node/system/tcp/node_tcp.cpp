#include "node_tcp.h"
#include "event_queue.h"
#include "ptp_service.h"

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
#include <stdexcept>

#include <json_dto/pub.hpp>

std::string eventToJSONMessage(const DetectedEvent &ev)
{
	std::string msg = json_dto::to_json(ev);
	msg += '\n';
	return msg;
}

std::string ptpStatusToJSONMessage(const PTP_STATUS &status)
{
	std::string msg = json_dto::to_json(status);
	msg += '\n';
	return msg;
}

NodeTCP::NodeTCP(std::string serverIp, int serverPort, EventQueue &queue,
				 std::uint16_t node_id, const PtpService *ptp)
	: queue_(queue), node_id_(node_id), ptp_(ptp)
{
	addr_ = {};
	addr_.sin_family = AF_INET;								   // IPv4
	addr_.sin_port = htons(static_cast<uint16_t>(serverPort)); // porten skal være i network byte order
	if (inet_pton(AF_INET, serverIp.c_str(), &addr_.sin_addr) != 1)
	{
		throw std::invalid_argument("Ugyldig IP-adresse: " + serverIp);
	}
}

NodeTCP::~NodeTCP()
{
	stop();
}

void NodeTCP::start()
{
	if (sender_thread_.joinable())
		return;
	if (ptp_ != nullptr)
	{
		status_thread_ = std::jthread([this](std::stop_token st)
								  { statusLoop(st); });
	}
	sender_thread_ = std::jthread([this](std::stop_token st)
								  { senderLoop(st); });
}

void NodeTCP::stop()
{
	if (sender_thread_.joinable())
	{
		sender_thread_.request_stop();
		sender_thread_.join();
	}
	if (status_thread_.joinable())
	{
		status_thread_.request_stop();
		status_thread_.join();
	}
}

void NodeTCP::connectToOrchestrator(std::stop_token st)
{
	while (!running && !st.stop_requested())
	{
		// En socket kan ikke genbruges efter et fejlet connect(), så der laves en ny hver gang
		fd_ = socket(AF_INET, SOCK_STREAM, 0);
		if (fd_ >= 0 && connect(fd_, (sockaddr *)&addr_, sizeof(addr_)) == 0)
		{
			std::cout << "Connected to orchestrator!\n";
			running = true;
		}
		else
		{
			std::cout << "connection failed, retrying...\n";
			if (fd_ >= 0)
			{
				close(fd_);
				fd_ = -1;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

void NodeTCP::senderLoop(std::stop_token st)
{
	while (!st.stop_requested())
	{
		connectToOrchestrator(st);
		std::uint64_t sent_status_version = 0;

		while (running && !st.stop_requested())
		{
			std::uint64_t status_version = 0;
			const auto status = latestStatus(status_version);
			if (status && status_version != sent_status_version)
			{
				if (!sendAsJSON(*status))
					break;
				sent_status_version = status_version;
			}

			auto ev = queue_.popFor(st, std::chrono::milliseconds(100));
			if (ev != std::nullopt)
			{
				sendAsJSON(*ev);
			}
		}

		// Forbindelsen er tabt (eller der er bedt om stop): luk socket og forbind igen
		if (fd_ >= 0)
		{
			close(fd_);
			fd_ = -1;
		}
		running = false;
	}
}

void NodeTCP::statusLoop(std::stop_token st)
{
	while (!st.stop_requested())
	{
		std::int64_t offset_budget_ns = 0;
		const bool synchronized = ptp_->readOffsetBudgetNs(offset_budget_ns);
		const PTP_STATUS status{
			node_id_,
			synchronized ? offset_budget_ns : 0,
			synchronized,
		};
		{
			std::lock_guard lock(status_mutex_);
			latest_status_ = status;
			++status_version_;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

std::optional<PTP_STATUS> NodeTCP::latestStatus(std::uint64_t &version) const
{
	std::lock_guard lock(status_mutex_);
	version = status_version_;
	return latest_status_;
}

bool NodeTCP::sendAsJSON(const DetectedEvent &ev)
{
	return sendMessage(eventToJSONMessage(ev));
}

bool NodeTCP::sendAsJSON(const PTP_STATUS &status)
{
	return sendMessage(ptpStatusToJSONMessage(status));
}

bool NodeTCP::sendMessage(const std::string &message)
{

	// send() kan sende færre bytes end bedt om, så der sendes i en løkke
	// indtil hele beskeden er afsendt.
	std::size_t sent = 0;
	while (sent < message.size())
	{
		ssize_t n = send(fd_, message.data() + sent, message.size() - sent, MSG_NOSIGNAL);
		if (n <= 0)
		{
			if (n < 0 && errno == EINTR)
			{
				continue;
			}
			std::cerr << "send failed, connection lost\n";
			running = false;
			return false;
		}
		sent += static_cast<std::size_t>(n);
	}
	return true;
}
