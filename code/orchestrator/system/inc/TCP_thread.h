#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include <netinet/in.h>

#include "detected_event.h"	  // DetectedEvent (fra common/detected_event.h)
#include "event_pair_queue.h" // EventPairQueue, EventPair

// Modtager DetectedEvent fra de to nodes over TCP (én JSON-linje pr. hændelse),
// finder de to hændelser fra hver sin node, der hører til samme klap, og
// lægger dem i EventPairQueue til TDOA (som en vector med to hændelser).
//
// Al netværkskode kører i én tråd, som med poll() holder øje med både
// lytte-socket og begge node-sockets. En node, der mister forbindelsen,
// kan derfor altid forbinde igen.
class OrchestratorTCP
{
public:
	OrchestratorTCP(int port, EventPairQueue &queue);
	~OrchestratorTCP();

	OrchestratorTCP(const OrchestratorTCP &) = delete;
	OrchestratorTCP &operator=(const OrchestratorTCP &) = delete;

	// Starter TCP-tråden og returnerer med det samme.
	void start();

	// Beder TCP-tråden om at stoppe og venter på, at den er færdig.
	void stop();

private:
	static constexpr int MAX_NODES = 2;
	static constexpr int POLL_TIMEOUT_MS = 100;						  // hvor tit stop og gamle hændelser tjekkes
	static constexpr std::size_t MAX_BUFFER_SIZE = 1'000'000;		  // max bytes uden '\n' fra én node
	static constexpr int64_t PAIR_WINDOW_NS = 20'000'000;			  // max |Δt| mellem et par (20 ms)
	static constexpr std::chrono::milliseconds PENDING_TIMEOUT{1000}; // hvor længe en hændelse venter på sin makker

	// Én forbindelse fra en node.
	struct Connection
	{
		int fd = -1;		// -1 = pladsen er ledig
		int nodeId = -1;	// node_id fra første hændelse, -1 = ukendt endnu
		std::string buffer; // modtagne bytes, der endnu ikke er en hel linje
	};

	// En hændelse, der venter på sin makker fra den anden node.
	struct Pending
	{
		DetectedEvent ev;
		int64_t clapTimeNs;							   // nodens PTP-tid for triggerpunktet
		std::chrono::steady_clock::time_point arrived; // orchestratorens tid ved modtagelse
	};

	void run(std::stop_token st);

	void acceptConnection();

	void enableKeepAlive(int fd);

	void closeConnection(Connection &conn);

	std::string nodeLabel(const Connection &conn) const;

	bool readFromConnection(Connection &conn);

	void handleLine(Connection &conn, const std::string &line);

	std::optional<DetectedEvent> jsonToStruct(const std::string &jsonLine);

	bool isValid(const DetectedEvent &ev);

	void pairOrStore(DetectedEvent &&ev);

	void discardOldPending();

	static int64_t clapTimeNs(const DetectedEvent &ev);

	int listenFd_ = -1;
	sockaddr_in addr_{};
	EventPairQueue &queue_;
	Connection connections_[MAX_NODES];
	std::vector<Pending> pending_;
	std::jthread thread_; // skal stå sidst, så tråden stoppes før de andre members nedlægges
};
