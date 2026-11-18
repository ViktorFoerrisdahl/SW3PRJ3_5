#include "orchestrator_tcp.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cerrno>
#include <iostream>
#include <json_dto/pub.hpp>

OrchestratorTCP::OrchestratorTCP(int port, EventQueue &queue) : port_(port), queue_(queue)
{
	addr_ = {};
	addr_.sin_family = AF_INET;							 // IPv4
	addr_.sin_addr.s_addr = htonl(INADDR_ANY);			 // Lyt på alle netkort
	addr_.sin_port = htons(static_cast<uint16_t>(port)); // Indstil port i network byte order

	// Indstil fd for lytte socket
	listenFd_ = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM = TCP
	if (listenFd_ < 0)
		throw std::runtime_error("socket() failed");

	// Sørg for at port kan genbruges efter genstart
	int yes = 1;
	if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		close(listenFd_);
		throw std::runtime_error("setsockopt() failed");
	}

	// Knyt socket til porten, saa nodes kan finde serveren
	if (bind(listenFd_, (sockaddr *)&addr_, sizeof(addr_)) < 0)
	{
		close(listenFd_);
		throw std::runtime_error("bind() failed");
	}

	// Gør socket til en lytte-socket
	if (listen(listenFd_, 2) < 0)
	{ // 2 er antal forbindelser OS må holde i kø
		close(listenFd_);
		throw std::runtime_error("listen() failed");
	}
}

void OrchestratorTCP::start()
{
	while (true)
	{
		int fd = accept(listenFd_, nullptr, nullptr); // Venter her indtil der er oprettet en ny forbindelse
		if (fd < 0)
			continue;
		bool assigned = false;
		for (int i = 0; i < 2; ++i)
		{
			if (done_[i])
			{ // Hvis tråd i er afsluttet
				if (threads_[i].joinable())
					threads_[i].join(); //  Sørg for at tråden er joinet, før ny tråd startes
				done_[i] = false;
				threads_[i] = std::thread(&OrchestratorTCP::receiveLoop, this, fd, i); // Opret ny tråd
				assigned = true;
				break;
			}
		}
		if (!assigned)
			close(fd);
	}
};

std::optional<DetectedEvent> OrchestratorTCP::jsonToStruct(const std::string &jsonLine)
{
	DetectedEvent ev;

	try
	{
		ev = json_dto::from_json<DetectedEvent>(jsonLine); // Throws exception if struct not valid
	}
	catch (const json_dto::ex_t &e)
	{
		return std::nullopt;
	}

	return ev;
}

void OrchestratorTCP::pushToQueue(DetectedEvent &ev)
{
	queue_.push(std::move(ev));
}

std::string OrchestratorTCP::recieveJson(int fd, std::string &buf)
{
	char chunk[4096];
	while (true)
	{
		size_t pos = buf.find('\n');
		if (pos != std::string::npos)
		{ // En hel struct (JSON format) er allerede i buffer
			std::string line = buf.substr(0, pos);
			buf.erase(0, pos + 1);
			return line;
		}
		ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
		if (n == 0)
			throw std::runtime_error("node disconnected");
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("recv() failed");
		}
		buf.append(chunk, n);
	}
}

void OrchestratorTCP::receiveLoop(int fd, int threadId)
{
	std::string buffer; // Buffer til recieveJson()
	while (true)
	{
		try
		{
			std::string jsonLine = recieveJson(fd, buffer);

			auto ev = jsonToStruct(jsonLine);
			if (ev != std::nullopt)
			{
				pushToQueue(*ev);
			}
			else
			{
				std::cout << "Failed to convert JSON to struct\n";
			}
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << "Node " << threadId << ": " << e.what() << "\n";
			close(fd);
			done_[threadId] = true;
			return;
		}
	}
}
