#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stop_token>
#include <vector>
#include "event_queue.h"
#include "microphones.h"
#include "plot.h"

// One event from each node
using EventPair = std::vector<DetectedEvent>;

// TDOA thread
void TdoaThread(std::stop_token st,
                std::queue<EventPair>& queue, std::mutex& event_lock,
                std::condition_variable_any& cv_tdoa,
                microphones& mics, std::mutex& data_lock, plot& p);