#include "sliding_rms.h"

#include <algorithm>
#include <stdexcept>

SlidingRms::SlidingRms(uint32_t window_frames) : squares_(window_frames, 0) {
    if (window_frames == 0) {
        throw std::invalid_argument("SlidingRms: vinduet skal indeholde mindst ét sample");
    }
}

void SlidingRms::push(int32_t x) {
    const int64_t sq = static_cast<int64_t>(x) * x;
    // Træk det ældste kvadrat fra (0 indtil vinduet er fyldt første gang),
    // læg det nye til og gem det på det ældstes plads.
    sum_ += sq - squares_[pos_];
    squares_[pos_] = sq;
    pos_ = (pos_ + 1) % squares_.size();
    if (count_ < squares_.size()) {
        ++count_;
    }
}

void SlidingRms::reset() {
    std::fill(squares_.begin(), squares_.end(), 0);
    pos_ = 0;
    count_ = 0;
    sum_ = 0;
}
