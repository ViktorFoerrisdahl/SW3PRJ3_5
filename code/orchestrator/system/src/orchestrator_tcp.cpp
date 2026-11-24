#include "orchestrator_tcp.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <json_dto/pub.hpp>

OrchestratorTCP::OrchestratorTCP(int port, EventPairQueue &queue) : queue_(queue)
{
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
	if (listen(listenFd_, MAX_NODES) < 0)
	{ // MAX_NODES er antal forbindelser OS må holde i kø
		close(listenFd_);
		throw std::runtime_error("listen() failed");
	}
}

OrchestratorTCP::~OrchestratorTCP()
{
	stop(); // TCP-tråden lukker selv node-forbindelserne, når den stopper
	if (listenFd_ >= 0)
		close(listenFd_);
}

void OrchestratorTCP::start()
{
	if (thread_.joinable())
		return; // kører allerede
	thread_ = std::jthread([this](std::stop_token st)
						   { run(st); });
}

void OrchestratorTCP::stop()
{
	if (thread_.joinable())
	{
		thread_.request_stop();
		thread_.join();
	}
}

void OrchestratorTCP::run(std::stop_token st)
{
	while (!st.stop_requested())
	{
		// Byg listen over sockets, som poll() skal holde øje med:
		// plads 0 er lytte-socket, resten er de forbundne nodes.
		pollfd fds[1 + MAX_NODES];
		int slotOf[1 + MAX_NODES]; // hvilken plads i connections_ fds[k] hører til
		nfds_t count = 0;

		fds[count++] = {listenFd_, POLLIN, 0};
		for (int i = 0; i < MAX_NODES; ++i)
		{
			if (connections_[i].fd >= 0)
			{
				slotOf[count] = i;
				fds[count++] = {connections_[i].fd, POLLIN, 0};
			}
		}

		// Venter til der er data, en ny forbindelse eller timeout,
		// så stop_token og gamle hændelser bliver tjekket jævnligt.
		int ready = poll(fds, count, POLL_TIMEOUT_MS);

		discardOldPending();

		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			std::cerr << "poll() failed: " << std::strerror(errno) << "\n";
			break;
		}
		if (ready == 0)
			continue; // timeout

		// Læs fra de nodes, der har sendt noget
		for (nfds_t k = 1; k < count; ++k)
		{
			Connection &conn = connections_[slotOf[k]];
			if (conn.fd != fds[k].fd)
				continue; // forbindelsen er lukket undervejs i denne runde
			if (fds[k].revents & (POLLIN | POLLHUP | POLLERR))
			{
				if (!readFromConnection(conn))
					closeConnection(conn);
			}
		}

		// Ny node vil forbinde
		if (fds[0].revents & POLLIN)
			acceptConnection();
	}

	// Ryd op, når tråden stopper
	for (Connection &conn : connections_)
		closeConnection(conn);
	pending_.clear();
}

void OrchestratorTCP::acceptConnection()
{
	int fd;
	do
	{
		fd = accept(listenFd_, nullptr, nullptr);
	} while (fd < 0 && errno == EINTR);

	if (fd < 0)
	{
		std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
		return;
	}

	for (int i = 0; i < MAX_NODES; ++i)
	{
		Connection &conn = connections_[i];
		if (conn.fd < 0)
		{
			enableKeepAlive(fd);
			conn.fd = fd;
			conn.nodeId = -1;
			conn.buffer.clear();
			std::cout << "Node forbundet (plads " << i << ")\n";
			return;
		}
	}

	// Begge pladser er optaget: afvis, så noden prøver igen senere
	std::cerr << "Afviser forbindelse: der er allerede " << MAX_NODES << " nodes forbundet\n";
	close(fd);
}

// Opdager en node, der er forsvundet uden at lukke forbindelsen
// (fx mistet strøm), efter ca. 2 + 3 * 1 = 5 sekunder.
void OrchestratorTCP::enableKeepAlive(int fd)
{
	int yes = 1, idle = 2, interval = 1, count = 3;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
	setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
}

// Navn på forbindelsen til udskrift, fx "Node 2 (plads 1)". Er node_id ikke
// kendt endnu (ingen hændelse modtaget), står der "Ukendt node (plads 1)".
std::string OrchestratorTCP::nodeLabel(const Connection &conn) const
{
	const int slot = static_cast<int>(&conn - connections_);
	if (conn.nodeId < 0)
		return "Ukendt node (plads " + std::to_string(slot) + ")";
	return "Node " + std::to_string(conn.nodeId) + " (plads " + std::to_string(slot) + ")";
}

void OrchestratorTCP::closeConnection(Connection &conn)
{
	if (conn.fd < 0)
		return;
	close(conn.fd);
	conn.fd = -1;
	conn.nodeId = -1;
	conn.buffer.clear();
}

// Læser det, der er klar på forbindelsen, og behandler alle hele linjer.
// Returnerer false, hvis forbindelsen skal lukkes.
bool OrchestratorTCP::readFromConnection(Connection &conn)
{
	char chunk[16384];
	ssize_t n;
	do
	{
		n = recv(conn.fd, chunk, sizeof(chunk), 0);
	} while (n < 0 && errno == EINTR);

	if (n == 0)
	{
		std::cerr << nodeLabel(conn) << " mistede forbindelsen (lukket af noden)\n";
		return false;
	}
	if (n < 0)
	{
		std::cerr << nodeLabel(conn) << " mistede forbindelsen: " << std::strerror(errno) << "\n";
		return false;
	}
	conn.buffer.append(chunk, static_cast<std::size_t>(n));

	// Tag alle hele linjer (én JSON-hændelse pr. linje) ud af bufferen
	std::size_t start = 0;
	std::size_t pos;
	while ((pos = conn.buffer.find('\n', start)) != std::string::npos)
	{
		handleLine(conn, conn.buffer.substr(start, pos - start));
		start = pos + 1;
	}
	conn.buffer.erase(0, start);

	// Beskyt mod en node, der sender data uden '\n'
	if (conn.buffer.size() > MAX_BUFFER_SIZE)
	{
		std::cerr << nodeLabel(conn) << ": besked for stor, lukker forbindelsen\n";
		return false;
	}
	return true;
}

void OrchestratorTCP::handleLine(Connection &conn, const std::string &line)
{
	auto ev = jsonToStruct(line);
	if (ev == std::nullopt)
	{
		std::cerr << "Failed to convert JSON to struct\n";
		return;
	}
	if (!isValid(*ev))
	{
		std::cerr << "Ugyldig hændelse #" << ev->event_seq << " fra node " << ev->node_id << ", ignoreret\n";
		return;
	}

	// Husk hvilken node forbindelsen tilhører. Har en anden forbindelse samme
	// node_id, er det en gammel forbindelse fra før noden genstartede.
	if (conn.nodeId != ev->node_id)
	{
		conn.nodeId = ev->node_id;
		for (Connection &other : connections_)
		{
			if (&other != &conn && other.fd >= 0 && other.nodeId == conn.nodeId)
			{
				std::cerr << nodeLabel(conn) << " har forbundet igen, lukker den gamle forbindelse\n";
				closeConnection(other);
			}
		}
	}

	pairOrStore(std::move(*ev));
}

std::optional<DetectedEvent> OrchestratorTCP::jsonToStruct(const std::string &jsonLine)
{
	try
	{
		return json_dto::from_json<DetectedEvent>(jsonLine); // Kaster exception, hvis JSON ikke passer til structen
	}
	catch (const json_dto::ex_t &)
	{
		return std::nullopt;
	}
}

// Tjekker at hændelsen kan bruges, før den parres.
bool OrchestratorTCP::isValid(const DetectedEvent &ev)
{
	return ev.sample_rate > 0 && !ev.frames.empty() && ev.trigger_offset < ev.frames.size();
}

// Finder den ventende hændelse fra den anden node, der ligger tættest på i tid.
// Er der en inden for PAIR_WINDOW_NS, lægges parret i køen, ellers venter
// hændelsen selv på sin makker.
void OrchestratorTCP::pairOrStore(DetectedEvent &&ev)
{
	const int64_t t = clapTimeNs(ev);

	auto best = pending_.end();
	int64_t bestDiff = PAIR_WINDOW_NS;
	for (auto it = pending_.begin(); it != pending_.end(); ++it)
	{
		if (it->ev.node_id == ev.node_id)
			continue; // skal være fra den anden node
		const int64_t diff = std::llabs(it->clapTimeNs - t);
		if (diff <= bestDiff)
		{
			bestDiff = diff;
			best = it;
		}
	}

	if (best == pending_.end())
	{
		pending_.push_back({std::move(ev), t, std::chrono::steady_clock::now()});
		return;
	}

	DetectedEvent mate = std::move(best->ev);
	pending_.erase(best);

	// [0] er altid noden med laveste node_id
	EventPair pair;
	pair.reserve(2);
	if (mate.node_id < ev.node_id)
	{
		pair.push_back(std::move(mate));
		pair.push_back(std::move(ev));
	}
	else
	{
		pair.push_back(std::move(ev));
		pair.push_back(std::move(mate));
	}
	queue_.push(std::move(pair));
}

// Kasserer hændelser, der har ventet for længe på en makker
// (fx fordi kun den ene node hørte klappet).
void OrchestratorTCP::discardOldPending()
{
	const auto now = std::chrono::steady_clock::now();
	std::erase_if(pending_, [&](const Pending &p)
				  {
		if (now - p.arrived <= PENDING_TIMEOUT)
			return false;
		std::cerr << "Ingen makker til hændelse #" << p.ev.event_seq << " fra node " << p.ev.node_id << ", kasseret\n";
		return true; });
}

// Nodens PTP-tid for triggerpunktet: første frames tid + trigger_offset frames.
int64_t OrchestratorTCP::clapTimeNs(const DetectedEvent &ev)
{
	return ev.t_first_ns + static_cast<int64_t>(ev.trigger_offset) * 1'000'000'000LL / ev.sample_rate;
}
