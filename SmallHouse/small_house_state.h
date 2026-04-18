#ifndef SMALL_HOUSE_STATE_H
#define SMALL_HOUSE_STATE_H

#include <Arduino.h>
#include "small_house_config.h"

struct SmallHouseState
{
  int total_frames = 0;
  unsigned long total_read_video = 0;
  unsigned long total_decode_video = 0;
  unsigned long total_show_video = 0;
  unsigned long start_ms = 0;
  unsigned long curr_ms = 0;

  long estimate_buffer_size = 0;
  uint8_t *mjpeg_buffer = nullptr;

  int current_mjpeg_index = 0;
  int mjpeg_count = 0;
  int display_width = 0;
  int display_height = 0;

  char mjpeg_path_buffer[small_house_config::kMjpegPathBufferSize] = {0};
  char mjpeg_file_list[small_house_config::kMaxFiles][small_house_config::kMjpegPathBufferSize] = {{0}};
  uint32_t mjpeg_file_sizes[small_house_config::kMaxFiles] = {0};
};

#endif
