// firmware/src/config.h
// Tunable settings. Nothing here touches hardware.
#pragma once
#include <stdint.h>

// 1 = manual bench-test mode (control loads from the serial monitor while wiring)
// 0 = automatic drying (+ UV-C phase after drying)
#ifndef BENCH_MODE
#define BENCH_MODE 1
#endif

// ---------- Towel sensor ----------
constexpr bool TOWEL_ACTIVE_LOW = true;    // LM393 D0 goes LOW when it sees the towel

// ---------- Temperature limits (deg C) ----------
constexpr float T_LOW      = 40.0;         // PTC on below this (air-out sensor)
constexpr float T_HIGH     = 45.0;         // PTC off at/above this (air-out sensor)
constexpr float T_AUX_MAX  = 55.0;         // PTC off at/above this (second DS18B20)
constexpr float T_AUX_HYST = 5.0;          // PTC may resume only below T_AUX_MAX - this

// ---------- Dry detection ----------
constexpr float RH_DIFF_DRY = 5.0;         // %RH: frame - room below this = dry

// ---------- Timing ----------
constexpr uint32_t DRY_HOLD_MS       = 5UL * 60 * 1000;
constexpr uint32_t COOLDOWN_MS       = 2UL * 60 * 1000;
constexpr uint32_t UVC_MS            = 10UL * 60 * 1000;  // UV-C phase after drying
constexpr uint32_t UVC_BENCH_MAX_MS  = 60UL * 1000;       // bench test auto-off
constexpr uint32_t PTC_DELAY_MS      = 3000;              // fans first, then PTC
constexpr uint32_t MAX_DRY_MS        = 5UL * 60 * 60 * 1000;
constexpr uint32_t TOWEL_DEBOUNCE_MS = 2000;
constexpr uint32_t SENSOR_BAD_MS     = 30UL * 1000;
constexpr uint32_t SENSOR_RETRY_MS   = 10UL * 1000;       // re-init missing I2C sensors
constexpr uint32_t READ_PERIOD_MS    = 2000;

// ---------- DS18B20 roles ----------
// All zeros = auto: first device on the bus is "air-out", second is "aux".
// To pin a role to one physical sensor, run bench mode, press 'd', and paste its address here.
// Role meaning: OUT = air leaving the frame (controls the PTC hysteresis).
//               AUX = second guard sensor with its own cutoff (T_AUX_MAX). Mount it where
//                     the heat is highest (near the PTC outlet), or leave it unconnected.
constexpr uint8_t DS_ADDR_OUT[8] = {0, 0, 0, 0, 0, 0, 0, 0};
constexpr uint8_t DS_ADDR_AUX[8] = {0, 0, 0, 0, 0, 0, 0, 0};