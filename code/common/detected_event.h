#pragma once

#include <cstdint>
#include <vector>

#include <json_dto/pub.hpp>

#include "frame.h" // Frame (fra common/frame.h)

// Fortæller json_dto, hvordan std::vector<Frame> skrives/læses som JSON.
// Frames bliver til ét fladt array med kanalerne interleaved:
// [ch0, ch1, ch0, ch1, ...], samme rækkefølge som i ALSA-bufferen.
struct FramesJsonRW
{
	void write(const std::vector<Frame> &frames, rapidjson::Value &to,
			   rapidjson::MemoryPoolAllocator<> &allocator) const
	{
		to.SetArray();
		to.Reserve(static_cast<rapidjson::SizeType>(frames.size() * 2), allocator);
		for (const Frame &f : frames)
		{
			to.PushBack(f.ch[0], allocator);
			to.PushBack(f.ch[1], allocator);
		}
	}

	void read(std::vector<Frame> &frames, const rapidjson::Value &from) const
	{
		if (!from.IsArray() || from.Size() % 2 != 0)
		{
			throw json_dto::ex_t{"frames: forventede et array med et lige antal samples"};
		}
		frames.clear();
		frames.reserve(from.Size() / 2);
		for (rapidjson::SizeType i = 0; i < from.Size(); i += 2)
		{
			frames.push_back(Frame{{from[i].GetInt(), from[i + 1].GetInt()}});
		}
	}
};

// Én detekteret hændelse. Den er hændelsesdetekteringens output på noden og
// sendes som én linje JSON (se json_io) fra NodeTCP til OrchestratorTCP.
struct DetectedEvent
{
	uint16_t node_id = 0;		 // hvilken mikrofonnode hændelsen kommer fra
	uint32_t event_seq = 0;		 // løbenummer for hændelsen på denne node
	uint32_t sample_rate = 0;	 // fx 48000
	int64_t t_first_ns = 0;		 // PTP-synkroniseret timestamp for udsnittets første frame
	uint32_t trigger_offset = 0; // triggerpunktets placering i udsnittet (frame-nr.)
	std::vector<Frame> frames;	 // de rå S32_LE-samples, kanalerne interleaved i hver frame

	// Bestemmer JSON-formatet for json_dto (bruges både til at skrive og læse).
	template <typename Json_Io>
	void json_io(Json_Io &io)
	{
		io & json_dto::mandatory("node_id", node_id)
		   & json_dto::mandatory("event_seq", event_seq)
		   & json_dto::mandatory("sample_rate", sample_rate)
		   & json_dto::mandatory("t_first_ns", t_first_ns)
		   & json_dto::mandatory("trigger_offset", trigger_offset)
		   & json_dto::mandatory(FramesJsonRW{}, "frames", frames);
	}
};
