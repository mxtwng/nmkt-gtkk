// firmware/src/main.cpp
// Towel drying rack (ESP32 NodeMCU-32S; ESP8266 NodeMCU builds too).
// Loads via D4184 modules: both fans on ONE module, each PTC on its own module, UV-C on the 4th.
// Sensors: 2x DS18B20 (air-out + aux guard), SHT31 (humidity inside frame),
//          AHT20 (room), TCRT5000 (towel).
// Hardware safety: breaker + KSD301 70C thermostat in series with each PTC; software is the extra layer.
// UV-C safety: only ever ON while the TCRT5000 sees a towel; always OFF otherwise.

#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_AHTX0.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "pins.h"
#include "config.h"

// ============================== State ==============================
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);
Adafruit_SHT31 sht;
Adafruit_AHTX0 aht;

bool shtOk = false, ahtOk = false;
DeviceAddress addrOut, addrAux;
bool haveOut = false, haveAux = false;

float tOut = NAN, tAux = NAN, rhIn = NAN, rhRoom = NAN;
bool fans = false, ptc1 = false, ptc2 = false, uvc = false;

// ============================== Inputs ==============================
bool towelPresent() {
  const bool raw = digitalRead(PIN_TOWEL);
  return TOWEL_ACTIVE_LOW ? !raw : raw;
}

// ============================== Outputs ==============================
// Single place that drives the loads, so every safety interlock is enforced here.
void applyOutputs() {
  if (!fans) ptc1 = ptc2 = false;      // PTCs never run without the fans
  if (!towelPresent()) uvc = false;    // UV-C never runs without a towel
  digitalWrite(PIN_FANS, fans);
  digitalWrite(PIN_PTC1, ptc1);
  digitalWrite(PIN_PTC2, ptc2);
  digitalWrite(PIN_UVC, uvc);
}

// Drives every load LOW before switching the pin to output, so nothing glitches on.
void initOutputs() {
  const int pins[] = {PIN_FANS, PIN_PTC1, PIN_PTC2, PIN_UVC};
  for (int p : pins) {
    digitalWrite(p, LOW);
    pinMode(p, OUTPUT);
  }
}

// ============================== I2C helpers ==============================
bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

// SHT31 can sit at 0x44 (ADDR low) or 0x45 (ADDR high).
bool initSht() {
  const uint8_t addrs[] = {0x44, 0x45};
  for (uint8_t a : addrs) {
    if (i2cPresent(a) && sht.begin(a)) return true;
  }
  return false;
}

bool initAht() { return i2cPresent(0x38) && aht.begin(); }

// Re-tries sensors that are missing, at most every SENSOR_RETRY_MS.
void retryMissingSensors() {
  static uint32_t lastTry = 0;
  const uint32_t now = millis();
  if (now - lastTry < SENSOR_RETRY_MS) return;
  lastTry = now;
  if (!shtOk) shtOk = initSht();
  if (!ahtOk) ahtOk = initAht();
}

// ============================== DS18B20 ==============================
bool addrIsSet(const uint8_t* a) {
  for (uint8_t i = 0; i < 8; i++) if (a[i]) return true;
  return false;
}

// Decides which physical DS18B20 plays which role (pinned address first, then bus order).
void assignDsRoles() {
  haveOut = haveAux = false;
  if (addrIsSet(DS_ADDR_OUT)) { memcpy(addrOut, DS_ADDR_OUT, 8); haveOut = true; }
  if (addrIsSet(DS_ADDR_AUX)) { memcpy(addrAux, DS_ADDR_AUX, 8); haveAux = true; }

  const uint8_t count = ds.getDeviceCount();
  DeviceAddress a;
  for (uint8_t i = 0; i < count; i++) {
    if (!ds.getAddress(a, i)) continue;
    if (haveOut && memcmp(a, addrOut, 8) == 0) continue;   // already assigned
    if (haveAux && memcmp(a, addrAux, 8) == 0) continue;
    if (!haveOut)      { memcpy(addrOut, a, 8); haveOut = true; }
    else if (!haveAux) { memcpy(addrAux, a, 8); haveAux = true; }
  }
}

// Returns NAN for a missing sensor or a bogus value (85.0 = power-on default).
float readDs(const uint8_t* addr) {
  const float t = ds.getTempC(addr);
  if (t == DEVICE_DISCONNECTED_C || t < -50.0f || t == 85.0f) return NAN;
  return t;
}

// ============================== Sensor reading ==============================
void readSensors() {
  retryMissingSensors();

  ds.requestTemperatures();
  tOut = haveOut ? readDs(addrOut) : NAN;
  tAux = haveAux ? readDs(addrAux) : NAN;

  rhIn = shtOk ? sht.readHumidity() : NAN;
  if (isnan(rhIn)) shtOk = false;            // forces a re-init on the next retry

  sensors_event_t hum, temp;
  if (ahtOk && aht.getEvent(&hum, &temp)) {
    rhRoom = hum.relative_humidity;
  } else {
    rhRoom = NAN;
    ahtOk = false;
  }
}

// A required temperature sensor is missing -> heaters must stay off.
bool tempFault() { return isnan(tOut) || (haveAux && isnan(tAux)); }

// Either temperature limit reached.
bool overTemp() {
  return (!isnan(tOut) && tOut >= T_HIGH) || (haveAux && !isnan(tAux) && tAux >= T_AUX_MAX);
}

// Aux sensor must have cooled by the hysteresis before the PTC may resume.
bool auxAllowsHeat() { return !haveAux || (!isnan(tAux) && tAux < T_AUX_MAX - T_AUX_HYST); }

void printStatus(const char* tag) {
  Serial.printf("[%s] towel=%d fans=%d ptc1=%d ptc2=%d uvc=%d | Tout=%.1f Taux=%.1f RHin=%.1f RHroom=%.1f\n",
                tag, towelPresent(), fans, ptc1, ptc2, uvc, tOut, tAux, rhIn, rhRoom);
}

// ============================== Setup ==============================
void setup() {
  Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP8266)
  WiFi.mode(WIFI_OFF);  // radio unused: cuts current spikes and heat
#endif
  initOutputs();
  pinMode(PIN_TOWEL, INPUT);
  applyOutputs();  // everything off

  Wire.begin(PIN_SDA, PIN_SCL);
  shtOk = initSht();
  ahtOk = initAht();
  ds.begin();
  ds.setResolution(10);
  assignDsRoles();

  Serial.printf("SHT31 %s | AHT20 %s | DS18B20 count=%d (out=%s aux=%s)\n",
                shtOk ? "ok" : "MISSING", ahtOk ? "ok" : "MISSING", ds.getDeviceCount(),
                haveOut ? "yes" : "NO", haveAux ? "yes" : "no");
#if BENCH_MODE
  Serial.println("BENCH: 1=fans  a=PTC1  b=PTC2  p=both PTC  u=UV-C (needs towel=1, auto-off 60s)");
  Serial.println("       i=I2C scan  d=list DS18B20  0=all off");
#endif
}

#if BENCH_MODE
// ============================== Bench mode ==============================
uint32_t uvcOnSince = 0;

bool ptcAllowed() { return fans && !tempFault() && !overTemp(); }

void i2cScan() {
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (i2cPresent(addr)) { Serial.printf("I2C device at 0x%02X\n", addr); found++; }
  }
  if (!found) Serial.println("I2C: no devices found");
}

// Prints each DS18B20 with its current temperature, formatted to paste into config.h.
// Warm one sensor with your fingers to see which one is which.
void listDs() {
  ds.requestTemperatures();
  const uint8_t n = ds.getDeviceCount();
  Serial.printf("DS18B20 on bus: %u\n", n);
  DeviceAddress a;
  for (uint8_t i = 0; i < n; i++) {
    if (!ds.getAddress(a, i)) continue;
    Serial.printf("  #%u {", i);
    for (uint8_t b = 0; b < 8; b++) Serial.printf("0x%02X%s", a[b], b < 7 ? ", " : "");
    Serial.printf("}  %.2f C\n", ds.getTempC(a));
  }
}

void loop() {
  static uint32_t last = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '1') fans = !fans;
    else if (c == 'a' || c == 'b' || c == 'p') {
      if (!ptcAllowed()) Serial.println("PTC refused: need fans on, valid DS18B20 readings, and below limits");
      else if (c == 'a') ptc1 = !ptc1;
      else if (c == 'b') ptc2 = !ptc2;
      else { const bool v = !(ptc1 && ptc2); ptc1 = ptc2 = v; }
    } else if (c == 'u') {
      if (!towelPresent()) Serial.println("UV-C refused: towel sensor must read towel=1");
      else { uvc = !uvc; if (uvc) uvcOnSince = millis(); }
    } else if (c == '0') { fans = ptc1 = ptc2 = uvc = false; }
    else if (c == 'i') i2cScan();
    else if (c == 'd') listDs();
  }
  if (uvc && millis() - uvcOnSince > UVC_BENCH_MAX_MS) { uvc = false; Serial.println("UV-C auto-off (bench limit)"); }
  if (millis() - last >= READ_PERIOD_MS) {
    last = millis();
    readSensors();
    if ((ptc1 || ptc2) && (tempFault() || overTemp())) { ptc1 = ptc2 = false; Serial.println("PTC auto-off (temp)"); }
    printStatus("bench");
  }
  applyOutputs();
}

#else
// ============================== Automatic mode ==============================
enum State { WAIT, DRY, UVC, COOL };
State state = WAIT;

void loop() {
  static uint32_t lastRead = 0, stateStart = 0, towelSince = 0, dryBelowSince = 0, badSince = 0;
  static bool lockout = false;  // after a cycle, towel must be removed before restarting
  const uint32_t now = millis();

  if (now - lastRead >= READ_PERIOD_MS) {
    lastRead = now;
    readSensors();
    printStatus(state == WAIT ? "wait" : state == DRY ? "dry" : state == UVC ? "uvc" : "cool");
  }

  const bool tempBad = tempFault();
  const bool rhBad = isnan(rhIn) || isnan(rhRoom);
  const bool towel = towelPresent();

  switch (state) {
    case WAIT:
      fans = ptc1 = ptc2 = uvc = false;
      if (!towel) { lockout = false; towelSince = 0; }
      else if (!lockout) {
        if (!towelSince) towelSince = now;
        if (now - towelSince > TOWEL_DEBOUNCE_MS) {
          stateStart = now; dryBelowSince = 0; badSince = 0;
          fans = true;
          state = DRY;
        }
      }
      break;

    case DRY: {
      fans = true;  // fans always on while drying
      uvc = false;

      // PTC: hysteresis T_LOW..T_HIGH on air-out, aux guard cutoff, only after fan head start,
      // off on any sensor error.
      if (tempBad || overTemp()) ptc1 = ptc2 = false;
      else if (now - stateStart > PTC_DELAY_MS && tOut < T_LOW && auxAllowsHeat()) ptc1 = ptc2 = true;

      // dry detection: frame RH close to room RH for DRY_HOLD_MS
      if (!rhBad && (rhIn - rhRoom) < RH_DIFF_DRY) {
        if (!dryBelowSince) dryBelowSince = now;
      } else dryBelowSince = 0;

      if (tempBad) { if (!badSince) badSince = now; } else badSince = 0;

      const bool dryDone = dryBelowSince && (now - dryBelowSince >= DRY_HOLD_MS);
      const bool timeout = now - stateStart >= MAX_DRY_MS;
      const bool sensorDead = badSince && (now - badSince >= SENSOR_BAD_MS);
      if (dryDone && towel) {
        ptc1 = ptc2 = false;
        lockout = true;
        stateStart = now;
        state = UVC;                 // dry -> UV-C phase (fans cool the PTC first)
      } else if (dryDone || timeout || !towel || sensorDead) {
        ptc1 = ptc2 = false;
        lockout = true;
        stateStart = now;
        state = COOL;                // no UV-C after an aborted/failed cycle
      }
      break;
    }

    case UVC:
      ptc1 = ptc2 = false;
      fans = (now - stateStart) < COOLDOWN_MS;   // cool the PTC, then fans off
      uvc = towel;                               // only while a towel is present
      if (!towel || (now - stateStart) >= UVC_MS) {
        uvc = false;
        stateStart = now;
        state = fans ? COOL : WAIT;
      }
      break;

    case COOL:
      ptc1 = ptc2 = false;
      uvc = false;
      fans = true;
      if (now - stateStart >= COOLDOWN_MS) { fans = false; state = WAIT; }
      break;
  }
  applyOutputs();
}
#endif