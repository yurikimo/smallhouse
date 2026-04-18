#include <FFat.h>
#include <SD_MMC.h>
#include "MjpegClass.h"
#include <Arduino_GFX_Library.h>
#include "TCA9554.h"

static const int GFX_BL = 6;

// Display SPI pins
static const int SPI_MISO = 2;
static const int SPI_MOSI = 1;
static const int SPI_SCLK = 5;
static const int LCD_DC = 3;
static const int LCD_CS = -1;
static const int LCD_RST = -1;

// Display geometry
static const int LCD_HOR_RES = 320;
static const int LCD_VER_RES = 480;

// SD_MMC pins
static const int SD_CLK_PIN = 11;
static const int SD_CMD_PIN = 10;
static const int SD_D0_PIN = 9;

// MJPEG storage
static const char MJPEG_FOLDER[] = "/videos";
static const int MAX_FILES = 20;
static const int MJPEG_PATH_BUFFER_SIZE = 128;

MjpegClass mjpeg;
int total_frames;
unsigned long total_read_video;
unsigned long total_decode_video;
unsigned long total_show_video;
unsigned long start_ms;
unsigned long curr_ms;

long estimateBufferSize;
uint8_t *mjpeg_buf;
static int currentMjpegIndex = 0;
int mjpegCount = 0;
int displayWidth = 0;
int displayHeight = 0;
char mjpegPathBuffer[MJPEG_PATH_BUFFER_SIZE];

char mjpegFileList[MAX_FILES][MJPEG_PATH_BUFFER_SIZE];
uint32_t mjpegFileSizes[MAX_FILES] = {0};

TCA9554 TCA(0x20);
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, SPI_SCLK, SPI_MOSI, SPI_MISO);
Arduino_GFX *gfx = new Arduino_ST7796(bus, LCD_RST, 2, true, LCD_HOR_RES, LCD_VER_RES);

static void lcd_reset(void)
{
  TCA.write1(1, 1);
  delay(10);
  TCA.write1(1, 0);
  delay(10);
  TCA.write1(1, 1);
  delay(200);
}

static String format_bytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0, 2) + " KB";
  }
  else
  {
    return String(bytes / 1024.0 / 1024.0, 2) + " MB";
  }
}

static void sort_mjpeg_files_list(void)
{
  for (int i = 0; i < mjpegCount - 1; i++)
  {
    for (int j = i + 1; j < mjpegCount; j++)
    {
      if (strcmp(mjpegFileList[i], mjpegFileList[j]) > 0)
      {
        char temp_name[MJPEG_PATH_BUFFER_SIZE];
        uint32_t temp_size;

        memcpy(temp_name, mjpegFileList[i], sizeof(temp_name));
        memcpy(mjpegFileList[i], mjpegFileList[j], sizeof(mjpegFileList[i]));
        memcpy(mjpegFileList[j], temp_name, sizeof(mjpegFileList[j]));

        temp_size = mjpegFileSizes[i];
        mjpegFileSizes[i] = mjpegFileSizes[j];
        mjpegFileSizes[j] = temp_size;
      }
    }
  }
}

static void load_mjpeg_files_list(void)
{
  File mjpeg_dir = SD_MMC.open(MJPEG_FOLDER);
  if (!mjpeg_dir)
  {
    Serial.printf("Failed to open %s folder\n", MJPEG_FOLDER);
    while (true)
    {
      /* Storage is required for playback. */
    }
  }

  mjpegCount = 0;
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
        name.toCharArray(mjpegFileList[mjpegCount], sizeof(mjpegFileList[mjpegCount]));
        mjpegFileSizes[mjpegCount] = file.size();
        mjpegCount++;

        if (mjpegCount >= MAX_FILES)
        {
          file.close();
          break;
        }
      }
    }
    file.close();
  }
  mjpeg_dir.close();

  sort_mjpeg_files_list();

  Serial.printf("%d mjpeg files read\n", mjpegCount);
  for (int i = 0; i < mjpegCount; i++)
  {
    Serial.printf(
        "File %d: %s, Size: %lu bytes (%s)\n",
        i,
        mjpegFileList[i],
        mjpegFileSizes[i],
        format_bytes(mjpegFileSizes[i]).c_str());
  }
}

static int jpeg_draw_callback(JPEGDRAW *pDraw)
{
  unsigned long draw_start = millis();
  gfx->draw16bitBeRGBBitmap(
      pDraw->x,
      pDraw->y,
      pDraw->pPixels,
      pDraw->iWidth,
      pDraw->iHeight);
  total_show_video += millis() - draw_start;
  return 1;
}

static void play_mjpeg_from_sd_card(char *mjpeg_filename)
{
  File mjpegFile = SD_MMC.open(mjpeg_filename, "r");

  Serial.printf("Opening %s\n", mjpeg_filename);
  if (!mjpegFile || mjpegFile.isDirectory())
  {
    Serial.printf("ERROR: Failed to open %s file for reading\n", mjpeg_filename);
    return;
  }

  Serial.println("MJPEG start");
  gfx->fillScreen(RGB565_BLACK);

  start_ms = millis();
  curr_ms = millis();
  total_frames = 0;
  total_read_video = 0;
  total_decode_video = 0;
  total_show_video = 0;

  if (!mjpeg.setup(
          &mjpegFile,
          mjpeg_buf,
          jpeg_draw_callback,
          true,
          0,
          0,
          displayWidth,
          displayHeight))
  {
    Serial.println("ERROR: MJPEG decoder setup failed");
    mjpegFile.close();
    return;
  }

  while (mjpegFile.available() && mjpeg.readMjpegBuf())
  {
    total_read_video += millis() - curr_ms;
    curr_ms = millis();

    mjpeg.drawJpg();
    total_decode_video += millis() - curr_ms;

    curr_ms = millis();
    total_frames++;
  }

  {
    int time_used = millis() - start_ms;
    float fps = 1000.0 * total_frames / time_used;

    Serial.println(F("MJPEG end"));
    mjpegFile.close();

    total_decode_video -= total_show_video;
    Serial.printf("Total frames: %d\n", total_frames);
    Serial.printf("Time used: %d ms\n", time_used);
    Serial.printf("Average FPS: %0.1f\n", fps);
    Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
    Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
    Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
    Serial.printf("Video size (wxh): %d×%d, scale factor=%d\n", mjpeg.getWidth(), mjpeg.getHeight(), mjpeg.getScale());
  }
}

static void play_selected_mjpeg(int mjpeg_index)
{
  if (mjpegCount <= 0)
  {
    Serial.println("No MJPEG files available for playback.");
    return;
  }

  snprintf(
      mjpegPathBuffer,
      sizeof(mjpegPathBuffer),
      "%s/%s",
      MJPEG_FOLDER,
      mjpegFileList[mjpeg_index]);

  Serial.printf("Playing %s\n", mjpegPathBuffer);
  play_mjpeg_from_sd_card(mjpegPathBuffer);
}

void setup(void)
{
  Serial.begin(115200);

  TCA.begin();
  TCA.pinMode1(1, OUTPUT);
  lcd_reset();

  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }

  displayWidth = gfx->width();
  displayHeight = gfx->height();

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->fillScreen(RGB565_BLACK);

  if (!SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN))
  {
    return;
  }

  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println(F("SD Mount Failed!"));
    gfx->println(F("SD Mount Failed!"));
  }
  else
  {
    Serial.println(F("SD Mounted. Scanning for videos..."));
  }

  Serial.println("Buffer allocation");
  estimateBufferSize = displayWidth * displayHeight * 2 / 5;
  mjpeg_buf = (uint8_t *)heap_caps_malloc(estimateBufferSize, MALLOC_CAP_8BIT);
  if (!mjpeg_buf)
  {
    Serial.println("mjpeg_buf allocation failed!");
    while (true)
    {
      /* no need to continue */
    }
  }

  load_mjpeg_files_list();
}

void loop(void)
{
  if (mjpegCount == 0)
  {
    return;
  }

  play_selected_mjpeg(currentMjpegIndex);

  currentMjpegIndex++;
  if (currentMjpegIndex >= mjpegCount)
  {
    currentMjpegIndex = 0;
  }
}
