// firmware/src/pins.h
// Pin map per board. The right block is chosen automatically by the build target.
#pragma once

#if defined(ARDUINO_ARCH_ESP8266)
// ---------- ESP8266 NodeMCU (D-label -> GPIO) ----------
// Boot rules: GPIO0 (D3) and GPIO2 (D4) must be HIGH at boot, GPIO15 (D8) must be LOW,
// and GPIO16 (D0) is HIGH at boot. So loads sit only on D5-D8, and D0 is input-only.
constexpr int PIN_SDA     = 4;   // D2 - SHT31 + AHT20
constexpr int PIN_SCL     = 5;   // D1
constexpr int PIN_ONEWIRE = 2;   // D4 - DS18B20 (4.7k pull-up to 3V3 keeps it HIGH at boot)
constexpr int PIN_TOWEL   = 16;  // D0 - TCRT5000 D0, input only (needs external 10k pull-up)
constexpr int PIN_FANS    = 14;  // D5 - D4184 #1 -> both fans
constexpr int PIN_PTC1    = 12;  // D6 - D4184 #2 -> PTC 1
constexpr int PIN_PTC2    = 13;  // D7 - D4184 #3 -> PTC 2
constexpr int PIN_UVC     = 15;  // D8 - D4184 #4 -> UV-C (held LOW at boot, safest pin)

#elif defined(ARDUINO_ARCH_ESP32)
// ---------- ESP32 DevKitC V4 (ESP32-WROOM-32D) ----------
constexpr int PIN_SDA     = 21;
constexpr int PIN_SCL     = 22;
constexpr int PIN_ONEWIRE = 4;
constexpr int PIN_TOWEL   = 34;  // input-only, no internal pull-up
constexpr int PIN_FANS    = 25;
constexpr int PIN_PTC1    = 26;
constexpr int PIN_PTC2    = 27;
constexpr int PIN_UVC     = 33;  // not IO14: that pin pulses at boot

#else
#error "Unsupported board: add a pin map to pins.h"
#endif