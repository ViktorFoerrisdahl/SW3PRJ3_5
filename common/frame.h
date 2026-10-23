#pragma once
#include <cstdint>

/** @brief One frame: one S32_LE sample from each of the node's two microphones. */
struct Frame
{
  int32_t ch[2];
};
static_assert(sizeof(Frame) == 2 * sizeof(int32_t), "Frame must match ALSA's interleaved layout");
