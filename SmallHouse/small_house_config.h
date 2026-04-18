#ifndef SMALL_HOUSE_CONFIG_H
#define SMALL_HOUSE_CONFIG_H

#include <Arduino.h>

namespace small_house_config
{
constexpr int kSerialBaudRate = 115200;

constexpr int kDisplayBacklightPin = 6;

constexpr int kDisplaySpiMisoPin = 2;
constexpr int kDisplaySpiMosiPin = 1;
constexpr int kDisplaySpiSclkPin = 5;
constexpr int kDisplayDcPin = 3;
constexpr int kDisplayCsPin = -1;
constexpr int kDisplayResetPin = -1;
constexpr int kDisplayRotation = 2;
constexpr bool kDisplayIsIps = true;
constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 480;

constexpr int kSdClkPin = 11;
constexpr int kSdCmdPin = 10;
constexpr int kSdD0Pin = 9;
constexpr char kSdMountPoint[] = "/sdcard";
constexpr bool kSdUseOneBitMode = true;

constexpr uint8_t kIoExpanderAddress = 0x20;
constexpr uint8_t kDisplayResetExpanderPin = 1;

constexpr char kMjpegFolder[] = "/videos";
constexpr int kMaxFiles = 20;
constexpr int kMjpegPathBufferSize = 128;
}  // namespace small_house_config

#define SMALL_HOUSE_DEBUG_LOGGING 1

#if SMALL_HOUSE_DEBUG_LOGGING
#define DEBUG_PRINTLN(message) Serial.println(message)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINTLN(message) \
  do                           \
  {                            \
  } while (0)
#define DEBUG_PRINTF(...) \
  do                      \
  {                       \
  } while (0)
#endif

#endif
