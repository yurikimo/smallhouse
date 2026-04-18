#include "small_house_app.h"

#include "small_house_config.h"
#include "small_house_display.h"
#include "small_house_player.h"
#include "small_house_state.h"
#include "small_house_storage.h"

namespace
{
SmallHouseState g_state;
}  // namespace

void small_house_setup(void)
{
  Serial.begin(small_house_config::kSerialBaudRate);

  small_house_display_init();
  g_state.display_width = small_house_display_get()->width();
  g_state.display_height = small_house_display_get()->height();

  if (!small_house_storage_init())
  {
    return;
  }

  small_house_player_init(&g_state);
  small_house_load_mjpeg_files(&g_state);
}

void small_house_loop(void)
{
  if (g_state.mjpeg_count == 0)
  {
    return;
  }

  small_house_play_selected_mjpeg(&g_state, g_state.current_mjpeg_index);

  g_state.current_mjpeg_index++;
  if (g_state.current_mjpeg_index >= g_state.mjpeg_count)
  {
    g_state.current_mjpeg_index = 0;
  }
}
