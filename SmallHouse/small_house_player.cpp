#include "small_house_player.h"

#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include "MjpegClass.h"
#include "small_house_display.h"

namespace
{
MjpegClass g_mjpeg;
SmallHouseState *g_state = nullptr;

int small_house_jpeg_draw_callback(JPEGDRAW *draw)
{
  unsigned long draw_start = millis();

  small_house_display_get()->draw16bitBeRGBBitmap(
      draw->x,
      draw->y,
      draw->pPixels,
      draw->iWidth,
      draw->iHeight);
  g_state->total_show_video += millis() - draw_start;
  return 1;
}

void small_house_play_mjpeg_from_sd_card(
    SmallHouseState *state,
    char *mjpeg_filename)
{
  File mjpeg_file = SD_MMC.open(mjpeg_filename, "r");

  DEBUG_PRINTF("Opening %s\n", mjpeg_filename);
  if (!mjpeg_file || mjpeg_file.isDirectory())
  {
    DEBUG_PRINTF("ERROR: Failed to open %s file for reading\n", mjpeg_filename);
    return;
  }

  DEBUG_PRINTLN("MJPEG start");
  small_house_display_get()->fillScreen(RGB565_BLACK);

  state->start_ms = millis();
  state->curr_ms = millis();
  state->total_frames = 0;
  state->total_read_video = 0;
  state->total_decode_video = 0;
  state->total_show_video = 0;

  if (!g_mjpeg.setup(
          &mjpeg_file,
          state->mjpeg_buffer,
          small_house_jpeg_draw_callback,
          true,
          0,
          0,
          state->display_width,
          state->display_height))
  {
    DEBUG_PRINTLN("ERROR: MJPEG decoder setup failed");
    mjpeg_file.close();
    return;
  }

  while (mjpeg_file.available() && g_mjpeg.readMjpegBuf())
  {
    state->total_read_video += millis() - state->curr_ms;
    state->curr_ms = millis();

    g_mjpeg.drawJpg();
    state->total_decode_video += millis() - state->curr_ms;

    state->curr_ms = millis();
    state->total_frames++;
  }

  {
    int time_used = millis() - state->start_ms;
    float fps = 1000.0 * state->total_frames / time_used;

    DEBUG_PRINTLN(F("MJPEG end"));
    mjpeg_file.close();

    state->total_decode_video -= state->total_show_video;
    DEBUG_PRINTF("Total frames: %d\n", state->total_frames);
    DEBUG_PRINTF("Time used: %d ms\n", time_used);
    DEBUG_PRINTF("Average FPS: %0.1f\n", fps);
    DEBUG_PRINTF("Read MJPEG: %lu ms (%0.1f %%)\n", state->total_read_video, 100.0 * state->total_read_video / time_used);
    DEBUG_PRINTF("Decode video: %lu ms (%0.1f %%)\n", state->total_decode_video, 100.0 * state->total_decode_video / time_used);
    DEBUG_PRINTF("Show video: %lu ms (%0.1f %%)\n", state->total_show_video, 100.0 * state->total_show_video / time_used);
    DEBUG_PRINTF("Video size (wxh): %d×%d, scale factor=%d\n", g_mjpeg.getWidth(), g_mjpeg.getHeight(), g_mjpeg.getScale());
  }
}
}  // namespace

void small_house_player_init(SmallHouseState *state)
{
  g_state = state;

  DEBUG_PRINTLN("Buffer allocation");

  // Tune only after measuring peak MJPEG size and playback stability on hardware.
  state->estimate_buffer_size = state->display_width * state->display_height * 2 / 5;
  state->mjpeg_buffer = (uint8_t *)heap_caps_malloc(
      state->estimate_buffer_size,
      MALLOC_CAP_8BIT);
  if (!state->mjpeg_buffer)
  {
    DEBUG_PRINTLN("mjpeg_buf allocation failed!");
    while (true)
    {
      /* Playback cannot continue without MJPEG staging memory. */
    }
  }
}

void small_house_play_selected_mjpeg(SmallHouseState *state, int mjpeg_index)
{
  if (state->mjpeg_count <= 0)
  {
    DEBUG_PRINTLN("No MJPEG files available for playback.");
    return;
  }

  snprintf(
      state->mjpeg_path_buffer,
      sizeof(state->mjpeg_path_buffer),
      "%s/%s",
      small_house_config::kMjpegFolder,
      state->mjpeg_file_list[mjpeg_index]);

  DEBUG_PRINTF("Playing %s\n", state->mjpeg_path_buffer);
  small_house_play_mjpeg_from_sd_card(state, state->mjpeg_path_buffer);
}
