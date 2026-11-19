// Towel dryer - ESP32 NodeMCU (38 pin)
// Libs: Adafruit SHT31 Library, Adafruit AHTX0, OneWire, DallasTemperature
//
// Pins
//   GPIO25 -> MOSFET trig (both fans, in parallel)
//   GPIO26 -> MOSFET trig (PTC 1)
//   GPIO27 -> MOSFET trig (PTC 2)  (mirrors PTC 1; leave unconnected if one MOSFET drives both)
//   GPIO4  -> DS18B20 data (4.7k pull-up to 3V3)
//   GPIO21 -> SDA (SHT31 + AHT20)
//   GPIO22 -> SCL (SHT31 + AHT20)
//   GPIO34 -> TCRT5000 DO (input only pin)

#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_AHTX0.h>

#define PIN_FAN      25
#define PIN_PTC1     26
#define PIN_PTC2     27
#define PIN_ONEWIRE   4
#define PIN_TOWEL    34

// TCRT5000 module: DO usually goes LOW when something is in front of it.
// If towel detection is inverted, flip this.
#define TOWEL_ACTIVE_LOW true

// --- Tunables ---
const float    T_HEAT_ON   = 40.0;   // PTC turns on below this (air out, C)
const float    T_HEAT_OFF  = 45.0;   // PTC turns off above this
const float    T_HARD_CUT  = 55.0;   // emergency cut
const float    DRY_MARGIN  = 5.0;    // towel dry when inside RH <= room RH + this
const uint32_t DRY_HOLD_MS = 5UL * 60UL * 1000UL;   // must stay dry this long
const uint32_t MIN_RUN_MS  = 2UL * 60UL * 1000UL;   // ignore dry check at start
const uint32_t MAX_RUN_MS  = 4UL * 3600UL * 1000UL; // hard timeout
const uint32_t COOL_MS     = 2UL * 60UL * 1000UL;   // fan-only cooldown
const uint8_t  TOWEL_DEBOUNCE = 3;                   // seconds

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);
Adafruit_SHT31 sht;
Adafruit_AHTX0 aht;

enum State { IDLE, RUN, COOL, DONE };
State state = IDLE;

bool shtOk = false, ahtOk = false;
bool ptcOn = false;
uint32_t stateStart = 0, dryStart = 0, lastTick = 0;
uint8_t towelCount = 0;
bool towelPresent = false;

void setPTC(bool on) {
  ptcOn = on;
  digitalWrite(PIN_PTC1, on);
  digitalWrite(PIN_PTC2, on);
}
void setFan(bool on) { digitalWrite(PIN_FAN, on); }

void enter(State s) {
  state = s;
  stateStart = millis();
  dryStart = 0;
  Serial.printf("STATE -> %d\n", s);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_PTC1, OUTPUT);
  pinMode(PIN_PTC2, OUTPUT);
  setFan(false);
  setPTC(false);
  pinMode(PIN_TOWEL, INPUT);

  Wire.begin(21, 22);
  shtOk = sht.begin(0x44);
  ahtOk = aht.begin();
  pinMode(PIN_ONEWIRE, INPUT_PULLUP);  // no external 4.7k available
  ds.begin();
  ds.setWaitForConversion(false);
  Serial.printf("SHT31 %s, AHT20 %s, DS18B20 count %d\n",
                shtOk ? "ok" : "FAIL", ahtOk ? "ok" : "FAIL", ds.getDeviceCount());
}

void loop() {
  if (millis() - lastTick < 1000) return;
  lastTick = millis();

  // --- read sensors ---
  ds.requestTemperatures();
  delay(200);
  float tOut = ds.getTempCByIndex(0);
  bool tempOk = (tOut > -100.0 && tOut < 125.0);

  float hIn = shtOk ? sht.readHumidity() : NAN;
  sensors_event_t hum, tmp;
  float hRoom = NAN;
  if (ahtOk) { aht.getEvent(&hum, &tmp); hRoom = hum.relative_humidity; }
  bool humOk = !isnan(hIn) && !isnan(hRoom);

  // towel presence (debounced)
  bool raw = digitalRead(PIN_TOWEL) == (TOWEL_ACTIVE_LOW ? LOW : HIGH);
  if (raw != towelPresent) {
    if (++towelCount >= TOWEL_DEBOUNCE) { towelPresent = raw; towelCount = 0; }
  } else towelCount = 0;

  // --- state machine ---
  uint32_t el = millis() - stateStart;
  switch (state) {
    case IDLE:
      setFan(false); setPTC(false);
      if (towelPresent) enter(RUN);
      break;

    case RUN:
      setFan(true);
      if (!tempOk || tOut >= T_HARD_CUT) setPTC(false);        // fail-safe
      else if (tOut < T_HEAT_ON) setPTC(true);
      else if (tOut > T_HEAT_OFF) setPTC(false);

      if (!towelPresent) { setPTC(false); enter(COOL); break; }
      if (el > MAX_RUN_MS) { setPTC(false); enter(COOL); break; }

      if (humOk && el > MIN_RUN_MS) {
        if (hIn <= hRoom + DRY_MARGIN) {
          if (!dryStart) dryStart = millis();
          if (millis() - dryStart > DRY_HOLD_MS) { setPTC(false); enter(COOL); }
        } else dryStart = 0;
      }
      break;

    case COOL:
      setPTC(false); setFan(true);
      if (el > COOL_MS) { setFan(false); enter(DONE); }
      break;

    case DONE:
      setFan(false); setPTC(false);
      if (!towelPresent) enter(IDLE);   // wait for towel removal before re-arming
      break;
  }

  Serial.printf("st=%d towel=%d Tout=%.1f hIn=%.1f hRoom=%.1f fan=%d ptc=%d t=%lus\n",
                state, towelPresent, tOut, hIn, hRoom, digitalRead(PIN_FAN), ptcOn, el / 1000);
}
