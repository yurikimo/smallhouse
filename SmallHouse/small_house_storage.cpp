#include "small_house_storage.h"

#include <SD_MMC.h>

#include "small_house_display.h"

namespace
{
String small_house_format_bytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0, 2) + " KB";
  }
  return String(bytes / 1024.0 / 1024.0, 2) + " MB";
}

void small_house_sort_mjpeg_files(SmallHouseState *state)
{
  for (int i = 0; i < state->mjpeg_count - 1; i++)
  {
    for (int j = i + 1; j < state->mjpeg_count; j++)
    {
      if (strcmp(state->mjpeg_file_list[i], state->mjpeg_file_list[j]) > 0)
      {
        char temp_name[small_house_config::kMjpegPathBufferSize];
        uint32_t temp_size;

        memcpy(temp_name, state->mjpeg_file_list[i], sizeof(temp_name));
        memcpy(state->mjpeg_file_list[i], state->mjpeg_file_list[j], sizeof(state->mjpeg_file_list[i]));
        memcpy(state->mjpeg_file_list[j], temp_name, sizeof(state->mjpeg_file_list[j]));

        temp_size = state->mjpeg_file_sizes[i];
        state->mjpeg_file_sizes[i] = state->mjpeg_file_sizes[j];
        state->mjpeg_file_sizes[j] = temp_size;
      }
    }
  }
}
}  // namespace

bool small_house_storage_init(void)
{
  if (!SD_MMC.setPins(
          small_house_config::kSdClkPin,
          small_house_config::kSdCmdPin,
          small_house_config::kSdD0Pin))
  {
    return false;
  }

  if (!SD_MMC.begin(
          small_house_config::kSdMountPoint,
          small_house_config::kSdUseOneBitMode))
  {
    DEBUG_PRINTLN(F("SD Mount Failed!"));
    small_house_display_get()->println(F("SD Mount Failed!"));
  }
  else
  {
    DEBUG_PRINTLN(F("SD Mounted. Scanning for videos..."));
  }

  return true;
}

void small_house_load_mjpeg_files(SmallHouseState *state)
{
  File mjpeg_dir = SD_MMC.open(small_house_config::kMjpegFolder);
  if (!mjpeg_dir)
  {
    DEBUG_PRINTF("Failed to open %s folder\n", small_house_config::kMjpegFolder);
    while (true)
    {
      /* Storage is required for playback. */
    }
  }

  state->mjpeg_count = 0;
  while (true)
  {
    File file = mjpeg_dir.openNextFile();
    if (!file)
    {
      break;
    }

    if (!file.isDirectory())
    {
      String name = file.name();
      if (name.endsWith(".mjpeg"))
      {
        name.toCharArray(
            state->mjpeg_file_list[state->mjpeg_count],
            sizeof(state->mjpeg_file_list[state->mjpeg_count]));
        state->mjpeg_file_sizes[state->mjpeg_count] = file.size();
        state->mjpeg_count++;

        if (state->mjpeg_count >= small_house_config::kMaxFiles)
        {
          file.close();
          break;
        }
      }
    }

    file.close();
  }
  mjpeg_dir.close();

  small_house_sort_mjpeg_files(state);

  DEBUG_PRINTF("%d mjpeg files read\n", state->mjpeg_count);
  for (int i = 0; i < state->mjpeg_count; i++)
  {
    DEBUG_PRINTF(
        "File %d: %s, Size: %lu bytes (%s)\n",
        i,
        state->mjpeg_file_list[i],
        state->mjpeg_file_sizes[i],
        small_house_format_bytes(state->mjpeg_file_sizes[i]).c_str());
  }
}
