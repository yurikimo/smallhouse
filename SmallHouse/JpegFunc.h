#include <FFat.h>
#include <SD_MMC.h>
#include "MjpegClass.h"          
#include <Arduino_GFX_Library.h>
#include "TCA9554.h"

#define GFX_BL 6

// Pin definitions
#define SPI_MISO 2
#define SPI_MOSI 1
#define SPI_SCLK 5
#define LCD_DC 3
#define LCD_CS -1 
#define LCD_RST -1
#define LCD_HOR_RES 320
#define LCD_VER_RES 480


MjpegClass mjpeg;
int total_frames;
unsigned long total_read_video;
unsigned long total_decode_video;
unsigned long total_show_video;
unsigned long start_ms, curr_ms;

long output_buf_size, estimateBufferSize;
uint8_t *mjpeg_buf;
uint16_t *output_buf;
static int currentMjpegIndex = 0;
int mjpegCount = 0;
const char *MJPEG_FOLDER = "/videos";

// Storage for files to read on the SD card
#define MAX_FILES 20 // Maximum number of files, adjust as needed
String mjpegFileList[MAX_FILES];
uint32_t mjpegFileSizes[MAX_FILES] = {0}; 

// SD Pins
int clk = 11;
int cmd = 10;
int d0 = 9;
int frameCount = 1; // Start at 0001.jpg
int totalFrames = 80;
TCA9554 TCA(0x20);
Arduino_DataBus* bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, SPI_SCLK, SPI_MOSI, SPI_MISO);
Arduino_GFX* gfx = new Arduino_ST7796(bus, LCD_RST, 2, true, LCD_HOR_RES, LCD_VER_RES);

// File management for sequence
File root;

void lcd_reset(void) {
  TCA.write1(1, 1); delay(10);
  TCA.write1(1, 0); delay(10);
  TCA.write1(1, 1); delay(200);
}

void setup(void) {
  Serial.begin(115200);

  TCA.begin();
  TCA.pinMode1(1, OUTPUT);
  lcd_reset();

  if (!gfx->begin()) Serial.println("gfx->begin() failed!");
  
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->fillScreen(RGB565_BLACK);

  if (!SD_MMC.setPins(clk, cmd, d0)) return;
  
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println(F("SD Mount Failed!"));
    gfx->println(F("SD Mount Failed!"));
  } 
  else {
    Serial.println(F("SD Mounted. Scanning for videos..."));
  }

  // Buffer allocation for mjpeg playing
  Serial.println("Buffer allocation");
  output_buf_size = gfx->width() * 4 * 2;
  output_buf = (uint16_t *)heap_caps_aligned_alloc(16, output_buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (!output_buf)
  {
      Serial.println("output_buf aligned_alloc failed!");
      while (true)
      {
          /* no need to continue */
      }
  }
  estimateBufferSize = gfx->width() * gfx->height() * 2 / 5;
  mjpeg_buf = (uint8_t *)heap_caps_malloc(estimateBufferSize, MALLOC_CAP_8BIT);
  if (!mjpeg_buf)
  {
      Serial.println("mjpeg_buf allocation failed!");
      while (true)
      {
          /* no need to continue */
      }
  }

  loadMjpegFilesList();
}

void loop() {
  playSelectedMjpeg(currentMjpegIndex);
    currentMjpegIndex++;
    if (currentMjpegIndex >= mjpegCount)
    {
        currentMjpegIndex = 0;
    }
}

// Play the current mjpeg
void playSelectedMjpeg(int mjpegIndex)
{
    // Build the full path for the selected mjpeg
    String fullPath = String(MJPEG_FOLDER) + "/" + mjpegFileList[mjpegIndex];
    char mjpegFilename[128];
    fullPath.toCharArray(mjpegFilename, sizeof(mjpegFilename));

    Serial.printf("Playing %s\n", mjpegFilename);
    mjpegPlayFromSDCard(mjpegFilename);
}

// Callback function to draw a JPEG
int jpegDrawCallback(JPEGDRAW *pDraw)
{
    unsigned long s = millis();
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    total_show_video += millis() - s;
    return 1;
}

// Play a mjpeg stored on the SD card
void mjpegPlayFromSDCard(char *mjpegFilename)
{
    Serial.printf("Opening %s\n", mjpegFilename);
    File mjpegFile = SD_MMC.open(mjpegFilename, "r");

    if (!mjpegFile || mjpegFile.isDirectory())
    {
        Serial.printf("ERROR: Failed to open %s file for reading\n", mjpegFilename);
    }
    else
    {
        Serial.println("MJPEG start");
        gfx->fillScreen(RGB565_BLACK);

        start_ms = millis();
        curr_ms = millis();
        total_frames = 0;
        total_read_video = 0;
        total_decode_video = 0;
        total_show_video = 0;

        mjpeg.setup(
            &mjpegFile, mjpeg_buf, jpegDrawCallback, true /* useBigEndian */,
            0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);

        while (mjpegFile.available() && mjpeg.readMjpegBuf())
        {
            // Read video
            total_read_video += millis() - curr_ms;
            curr_ms = millis();

            // Play video
            mjpeg.drawJpg();
            total_decode_video += millis() - curr_ms;

            curr_ms = millis();
            total_frames++;
        }
        

        int time_used = millis() - start_ms;
        Serial.println(F("MJPEG end"));
        mjpegFile.close();

        float fps = 1000.0 * total_frames / time_used;
        total_decode_video -= total_show_video;
        Serial.printf("Total frames: %d\n", total_frames);
        Serial.printf("Time used: %d ms\n", time_used);
        Serial.printf("Average FPS: %0.1f\n", fps);
        Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
        Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
        Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
        Serial.printf("Video size (wxh): %d×%d, scale factor=%d\n",mjpeg.getWidth(),mjpeg.getHeight(),mjpeg.getScale());
    }
    
}

// Read the mjpeg file list in the mjpeg folder of the SD card
void loadMjpegFilesList()
{
    File mjpegDir = SD_MMC.open(MJPEG_FOLDER);
    if (!mjpegDir)
    {
        Serial.printf("Failed to open %s folder\n", MJPEG_FOLDER);
        while (true)
        {
            /* code */
        }
    }
    mjpegCount = 0;
    while (true)
    {
        File file = mjpegDir.openNextFile();
        if (!file)
            break;
        if (!file.isDirectory())
        {
            String name = file.name();
            if (name.endsWith(".mjpeg"))
            {
                mjpegFileList[mjpegCount] = name;
                mjpegFileSizes[mjpegCount] = file.size(); // Save file size (in bytes)
                mjpegCount++;
                if (mjpegCount >= MAX_FILES)
                    break;
            }
        }
        file.close();
    }
    mjpegDir.close();
    Serial.printf("%d mjpeg files read\n", mjpegCount);
    // Optionally, print out each file's size for debugging:
    for (int i = 0; i < mjpegCount; i++)
    {
        Serial.printf("File %d: %s, Size: %lu bytes (%s)\n", i, mjpegFileList[i].c_str(), mjpegFileSizes[i],formatBytes(mjpegFileSizes[i]).c_str());
    }
}

// Function helper display sizes on the serial monitor
String formatBytes(size_t bytes)
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
