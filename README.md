# SmallHouse

`SmallHouse` is an ESP32-S3 project that plays MJPEG videos from an SD card on a 320x480 ST7796 display. The code is organized into small Arduino-friendly C++ modules.

## Hardware

- ESP32-S3 board
- ST7796 SPI display
- microSD card connected through `SD_MMC`
- TCA9554 I2C I/O expander for display reset

## How It Works

On boot, the firmware:

1. Initializes the display and backlight
2. Mounts the SD card
3. Allocates the MJPEG decode buffer
4. Scans `/videos` for `.mjpeg` files
5. Plays each file in sorted order and loops forever

## Build And Run

1. Open [SmallHouse.ino](/Users/yurikimo/Developer/Arduino/smallhouse/SmallHouse/SmallHouse.ino) in Arduino IDE.
2. Configure `Tools` in Arduino IDE:
   - Select `ESP32S3 Dev Module`
   - Enable `USB CDC On Boot` for serial debugging
   - Set `Flash Size` to `16MB (128Mb)`
   - Set `Partition Scheme` to `16M Flash (3MB APP/9.9MB FATFS)`
   - Set `PSRAM` to `OPI PSRAM`
3. Install the required libraries:
   - `Arduino_GFX_Library`
   - `TCA9554`
   - `JPEGDEC`
4. Insert an SD card containing `.mjpeg` files in `/videos`.
5. Build and upload.

## Project Layout

```text
smallhouse/
├── README.md
├── SmallHouse/
│   ├── SmallHouse.ino
│   ├── MjpegClass.h
│   ├── JpegFunc.h
│   ├── small_house_app.cpp
│   ├── small_house_app.h
│   ├── small_house_config.h
│   ├── small_house_display.cpp
│   ├── small_house_display.h
│   ├── small_house_player.cpp
│   ├── small_house_player.h
│   ├── small_house_state.h
│   ├── small_house_storage.cpp
│   └── small_house_storage.h
└── libraries/
```

## Notes

- `JpegFunc.h` is retained from the original project history but is not part of the active MJPEG playback path.
- Buffer sizes in the decoder and player are intentionally unchanged; tune them only after measuring on hardware.

## Planned Hardware Update

A future revision will add a reed switch to the door:

- when the door opens, the ESP32-S3 will boot and start playback automatically
- when the door closes, the firmware will place the ESP32-S3 into deep sleep

## Reference

- Waveshare wiki: [ESP32-S3-Touch-LCD-3.5](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5)
