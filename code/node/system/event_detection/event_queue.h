#pragma once

#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stop_token>

#include "detected_event.h" // DetectedEvent (fra common/detected_event.h)

// Trådsikker kø mellem detektortråden (producent) og TCP-modulet (forbruger).
// Detektoren venter aldrig på netværket: er køen fuld, smides den ældste
// hændelse væk, så den nyeste altid kommer med.
class EventQueue
{
public:
	explicit EventQueue(std::size_t max_size) : max_size_(max_size) {}

	// Lægger en hændelse i køen og vækker en ventende forbruger.
	void push(DetectedEvent &&ev)
	{
		{
			std::lock_guard lock(mutex_);
			if (max_size_ > 0 && queue_.size() >= max_size_)
			{
				queue_.pop_front();
				++dropped_;
			}
			queue_.push_back(std::move(ev));
		}
		cv_.notify_one();
	}

	// Venter på næste hændelse. Returnerer std::nullopt, hvis der anmodes om
	// stop (via stop_token), før der kommer en hændelse.
	std::optional<DetectedEvent> pop(std::stop_token st)
	{
		std::unique_lock lock(mutex_);
		if (!cv_.wait(lock, st, [this]
					  { return !queue_.empty(); }))
		{
			return std::nullopt;
		}
		DetectedEvent ev = std::move(queue_.front());
		queue_.pop_front();
		return ev;
	}

	// Henter en hændelse uden at vente, hvis der er en.
	std::optional<DetectedEvent> tryPop()
	{
		std::lock_guard lock(mutex_);
		if (queue_.empty())
		{
			return std::nullopt;
		}
		DetectedEvent ev = std::move(queue_.front());
		queue_.pop_front();
		return ev;
	}

	std::size_t size() const
	{
		std::lock_guard lock(mutex_);
		return queue_.size();
	}

	// Antal hændelser der er smidt væk, fordi køen var fuld.
	uint64_t dropped() const
	{
		std::lock_guard lock(mutex_);
		return dropped_;
	}

private:
	mutable std::mutex mutex_;
	std::condition_variable_any cv_; // _any, så pop() kan afbrydes med en stop_token
	std::deque<DetectedEvent> queue_;
	std::size_t max_size_;
	uint64_t dropped_ = 0;
};
