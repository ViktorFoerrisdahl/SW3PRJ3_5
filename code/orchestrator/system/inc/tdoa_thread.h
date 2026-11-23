#pragma once
#include <stop_token>
#include "event_pair_queue.h"   // EventPairQueue, EventPair (from TCP)
#include "microphones.h"
#include "plot.h"

// Waits for a pair from TCP, computes the position, logs it and starts a plot thread.
// Ends when stop is requested on st.
void TdoaThread(std::stop_token st, EventPairQueue& queue, microphones& mics, plot& p);
