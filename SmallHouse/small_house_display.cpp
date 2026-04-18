#include "small_house_display.h"

#include "TCA9554.h"
#include "small_house_config.h"

namespace
{
TCA9554 g_io_expander(small_house_config::kIoExpanderAddress);
Arduino_DataBus *g_display_bus = new Arduino_ESP32SPI(
    small_house_config::kDisplayDcPin,
    small_house_config::kDisplayCsPin,
    small_house_config::kDisplaySpiSclkPin,
    small_house_config::kDisplaySpiMosiPin,
    small_house_config::kDisplaySpiMisoPin);
Arduino_GFX *g_display = new Arduino_ST7796(
    g_display_bus,
    small_house_config::kDisplayResetPin,
    small_house_config::kDisplayRotation,
    small_house_config::kDisplayIsIps,
    small_house_config::kDisplayWidth,
    small_house_config::kDisplayHeight);

void small_house_lcd_reset(void)
{
  g_io_expander.write1(small_house_config::kDisplayResetExpanderPin, 1);
  delay(10);
  g_io_expander.write1(small_house_config::kDisplayResetExpanderPin, 0);
  delay(10);
  g_io_expander.write1(small_house_config::kDisplayResetExpanderPin, 1);
  delay(200);
}
}  // namespace

bool small_house_display_init(void)
{
  g_io_expander.begin();
  g_io_expander.pinMode1(small_house_config::kDisplayResetExpanderPin, OUTPUT);
  small_house_lcd_reset();

  if (!g_display->begin())
  {
    DEBUG_PRINTLN("gfx->begin() failed!");
  }

  pinMode(small_house_config::kDisplayBacklightPin, OUTPUT);
  digitalWrite(small_house_config::kDisplayBacklightPin, HIGH);

  g_display->fillScreen(RGB565_BLACK);
  return true;
}

Arduino_GFX *small_house_display_get(void)
{
  return g_display;
}
