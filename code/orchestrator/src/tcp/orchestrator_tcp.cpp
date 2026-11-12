#include "orchestrator_tcp.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

OrchestratorTCP::OrchestratorTCP(int port, EventQueue& queue) : port_(port) queue_(queue) {
	addr_ = {};
	addr_.sin_family = AF_INET; // IPv4
	addr_.sin_addr.s_addr = htonl(INADDR_ANY); // Lyt på alle netkort
	addr_.sin_port = htons(static_cast<uint16_t>(port)); // Indstil port i network byte order

	// Indstil fd for lytte socket
	listenFd_ = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM = TCP
	if (listenFd_ < 0) throw std::runtime_error("socket() failed");

	// Sørg for at port kan genbruges efter genstart
	int yes = 1;
	setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	// Knyt socket til porten, saa nodes kan finde serveren
	if (bind(listenFd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		close(listenFd_);
		throw std::runtime_error("bind() failed");
	}

	// Gør socket til en lytte-socket
	if (listen(listenFd_, 2) < 0) { // 2 er antal forbindelser OS må holde i kø
		close(listenFd_);
		throw std::runtime_error("listen() failed");
	}
}

void OrchestratorTCP::start();

std::optional<DetectedEvent> OrchestratorTCP::jsonToStruct(const std::string& jsonLine){
	DetectedEvent ev;

	try {
		ev = json_dto::from_json<DetectedEvent>(jsonLine); // Throws exception if struct not valid
	} catch(const json_dto::ex_t& e) {
		return std::nullopt;
	}

	return ev;
}

void OrchestratorTCP::pushToQueue(DetectedEvent& ev){
    queue_.push(std::move(ev));
}

std::string OrchestratorTCP::recieveJson() {
	std::string buffer;

}

void OrchestratorTCP::receiveLoop(){}