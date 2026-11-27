// Mikrofonnode (Raspberry Pi 5)
//
// Brug: atlas_node <orchestrator-ip>
//
// Starter PTP, venter på sync og kører derefter tre tråde:
//   capture  (AlsaCapture::run)     ALSA -> RingBuffer + tidsankre
//   detektor (EventDetector::run)   RingBuffer -> DetectedEvent -> EventQueue
//   TCP      (NodeTCP::senderLoop)  EventQueue -> orchestrator
// Main venter på SIGINT/SIGTERM og lukker det hele pænt ned.
// Se docs/Construction/Activity diagram node.drawio.

#include "alsa_capture.h"    // AlsaCapture, ALSA_CONFIG
#include "detector_config.h" // DetectorConfig, thresholdRaw()
#include "event_detection.h" // EventDetector
#include "event_queue.h"     // EventQueue, DetectedEvent
#include "node_tcp.h"        // NodeTCP
#include "ptp_service.h"     // PtpService
#include "ring_buffer.h"     // RingBuffer

#include <atomic>
#include <csignal> // sigset_t, sigwait, sigtimedwait, kill
#include <cstdint>
#include <ctime> // timespec
#include <exception>
#include <iostream>
#include <string>
#include <thread>   // std::jthread
#include <unistd.h> // getpid, gethostname

namespace
{

// ---- Faste indstillinger. Kun orchestratorens IP kommer fra kommandolinjen ----
constexpr int ORCHESTRATOR_PORT = 5000;        // samme som i TCP-testene
constexpr const char *ALSA_DEVICE = "hw:0,0";  // find den med `arecord -l`
constexpr uint32_t SAMPLE_RATE = 48000;
constexpr uint32_t PERIOD_FRAMES = 512;        // ca. 10,7 ms pr. læsning
constexpr uint32_t PERIODS = 4;                // ALSA-buffer ca. 43 ms
constexpr std::size_t RING_FRAMES = 131072;    // ca. 2,7 s
constexpr double MIC_SENSITIVITY_DBFS = -26.0; // SPH0645LM4H-B
constexpr double THRESHOLD_DB_SPL = 80.0;      // IKKE fastsat endnu: gruppen beslutter

// node_id læses fra værtsnavnet, hvis sidste tegn skal være 0 eller 1,
// fx "atlas-node0" og "atlas-node1". Så kan begge Pi'er køre samme program.
bool NodeIdFromHostname(uint16_t &node_id)
{
	char name[256] = {};
	if (gethostname(name, sizeof(name) - 1) != 0)
		return false;
	const std::string host(name);
	if (host.empty() || (host.back() != '0' && host.back() != '1'))
		return false;
	node_id = static_cast<uint16_t>(host.back() - '0');
	return true;
}

// Venter på PTP-sync og prøver igen hvert sekund. sigtimedwait() bruges som
// sleep, så Ctrl+C stadig virker imens. Returnerer false, hvis der kom et signal.
bool WaitForPtpSync(const PtpService &ptp, const sigset_t &sigs)
{
	const timespec one_second{1, 0};
	while (true)
	{
		std::int64_t offset_ns = 0;
		if (ptp.readOffsetBudgetNs(offset_ns))
		{
			std::cout << "PTP synkroniseret (offset " << offset_ns << " ns)\n";
			return true;
		}
		std::cout << "Venter på PTP-sync...\n";
		if (sigtimedwait(&sigs, nullptr, &one_second) > 0)
			return false;
	}
}

int RunNode(const std::string &orchestrator_ip, const sigset_t &sigs)
{
	// ---- Hvem er vi? ----
	uint16_t node_id = 0;
	if (!NodeIdFromHostname(node_id))
	{
		std::cerr << "Værtsnavnet skal ende på 0 eller 1 (fx atlas-node0)\n";
		return 1;
	}
	std::cout << "Node " << node_id << ", orchestrator " << orchestrator_ip << ":" << ORCHESTRATOR_PORT << "\n";

	// ---- Opret alt, der kan fejle, før PTP startes (ingen tråde endnu) ----
	DetectorConfig cfg;
	cfg.node_id = node_id;
	cfg.sample_rate = SAMPLE_RATE;
	const double thr = thresholdRaw(18, MIC_SENSITIVITY_DBFS, THRESHOLD_DB_SPL, 94.0);
	cfg.threshold_raw = {thr, thr};

	RingBuffer rb(RING_FRAMES, SAMPLE_RATE);
	EventQueue queue(cfg.max_queued_events);
	EventDetector detector(rb, queue, cfg);                  // kaster, hvis cfg er ugyldig
	PtpService ptp(node_id == 0 ? PtpRole::Grandmaster : PtpRole::Slave);
	NodeTCP tcp(orchestrator_ip, ORCHESTRATOR_PORT, queue, node_id, &ptp);  // kaster ved ugyldig IP
	AlsaCapture capture(rb);

	// ---- PTP: node 0 er grandmaster, node 1 er slave ----
	if (!ptp.start())
	{
		std::cerr << "Kunne ikke starte PTP (kører programmet som root?)\n";
		ptp.stop();
		return 1;
	}
	if (!WaitForPtpSync(ptp, sigs))
	{
		ptp.stop();
		return 0; // stoppet med Ctrl+C under ventetiden
	}

	std::string error;
	if (!capture.open({ALSA_DEVICE, SAMPLE_RATE, PERIOD_FRAMES, PERIODS}, error))
	{
		std::cerr << "ALSA: " << error << "\n";
		ptp.stop();
		return 1;
	}

	// ---- Start trådene (fork). Forbrugerne først, så intet går tabt ----
	detector.start();
	tcp.start();
	std::atomic<bool> capture_failed{false};
	std::jthread capture_thread([&](std::stop_token st)
								{
		if (!capture.run(st))
		{
			// Fx hvis snd_pcm_recover() fejler. kill() sender signalet til hele
			// processen, så main vågner i sigwait() og lukker ned.
			capture_failed = true;
			kill(getpid(), SIGTERM);
		} });

	std::cout << "Noden kører. Stop med Ctrl+C.\n";

	// ---- Main sover, til der kommer SIGINT/SIGTERM ----
	int sig = 0;
	sigwait(&sigs, &sig);
	std::cout << "\nModtog " << (sig == SIGINT ? "SIGINT" : "SIGTERM") << ", lukker ned\n";

	// ---- Nedlukning ----
	capture_thread.request_stop(); // snd_pcm_readi() vender tilbage inden for ~10,7 ms
	capture_thread.join();
	detector.stop();
	tcp.stop();
	ptp.stop();
	return capture_failed ? 1 : 0;
}

} // namespace

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Brug: " << argv[0] << " <orchestrator-ip>\n";
		return 1;
	}

	// Bloker SIGINT/SIGTERM, FØR nogen tråd startes. Trådene arver masken, så
	// signalerne kun bliver modtaget i main via sigwait()/sigtimedwait().
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);
	sigaddset(&sigs, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sigs, nullptr);

	try
	{
		return RunNode(argv[1], sigs);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fejl: " << e.what() << "\n";
		return 1;
	}
}
