#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <stop_token>
#include <vector>

#include "detected_event.h" // DetectedEvent (fra common/detected_event.h)

// To hændelser fra hver sin node, som hører til samme klap.
// Vektoren har altid størrelse 2, og [0] er altid hændelsen fra noden med
// det laveste node_id.
using EventPair = std::vector<DetectedEvent>;

// Trådsikker kø mellem OrchestratorTCP (producent) og TDOA (forbruger).
// Er køen fuld, smides det ældste par væk, så det nyeste altid kommer med.
class EventPairQueue
{
public:
	explicit EventPairQueue(std::size_t max_size) : max_size_(max_size) {}

	// Lægger et par i køen og vækker en ventende forbruger.
	void push(EventPair &&pair)
	{
		{
			std::lock_guard lock(mutex_);
			if (max_size_ > 0 && queue_.size() >= max_size_)
			{
				queue_.pop_front();
				++dropped_;
			}
			queue_.push_back(std::move(pair));
		}
		cv_.notify_one();
	}

	// Venter på næste par. Returnerer std::nullopt, hvis der anmodes om
	// stop (via stop_token), før der kommer et par.
	std::optional<EventPair> pop(std::stop_token st)
	{
		std::unique_lock lock(mutex_);
		if (!cv_.wait(lock, st, [this]
					  { return !queue_.empty(); }))
		{
			return std::nullopt;
		}
		EventPair pair = std::move(queue_.front());
		queue_.pop_front();
		return pair;
	}

	// Henter et par uden at vente, hvis der er et.
	std::optional<EventPair> tryPop()
	{
		std::lock_guard lock(mutex_);
		if (queue_.empty())
		{
			return std::nullopt;
		}
		EventPair pair = std::move(queue_.front());
		queue_.pop_front();
		return pair;
	}

	std::size_t size() const
	{
		std::lock_guard lock(mutex_);
		return queue_.size();
	}

	// Antal par der er smidt væk, fordi køen var fuld.
	uint64_t dropped() const
	{
		std::lock_guard lock(mutex_);
		return dropped_;
	}

private:
	mutable std::mutex mutex_;
	std::condition_variable_any cv_; // _any, så pop() kan afbrydes med en stop_token
	std::deque<EventPair> queue_;
	std::size_t max_size_;
	uint64_t dropped_ = 0;
};
