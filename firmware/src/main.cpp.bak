#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

namespace {
// Mo phong phan mem: cac ngõ ra chi la bien trong bo nho, khong noi GPIO.
constexpr unsigned long kLoopIntervalMs = 100;
constexpr unsigned long kStatusIntervalMs = 1000;
constexpr unsigned long kTowelDebounceMs = 500;
constexpr unsigned long kAirflowSettleMs = 3000;
constexpr unsigned long kDryConfirmMs = 15000;
constexpr unsigned long kCooldownMs = 5000;
constexpr unsigned long kSimulationMaxRunMs = 120000;
constexpr float kHeaterOnC = 40.0f;
constexpr float kHeaterOffC = 45.0f;
constexpr float kOverheatC = 55.0f;
constexpr size_t kCommandBufferSize = 80;

enum class State {
  IDLE,
  AIRFLOW_CHECK,
  DRYING,
  COOLING,
  WAIT_FOR_REMOVAL,
  FAULT,
};

struct Inputs {
  bool sensorsValid = true;  // Gia lap mac dinh de co the thu bang Serial.
  bool towelRequested = false;
  bool towelPresent = false;
  bool dryCondition = false;
  float outletTemperatureC = 25.0f;
};

struct VirtualOutputs {
  bool fansOn = false;
  bool heatersOn = false;
  bool uvOn = false;
};

Inputs inputs;
VirtualOutputs outputs;
State state = State::IDLE;
unsigned long lastLoopMs = 0;
unsigned long lastStatusMs = 0;
unsigned long towelChangeMs = 0;
unsigned long stateStartedMs = 0;
unsigned long dryConditionStartedMs = 0;
bool dryTimerRunning = false;
char faultReason[48] = "";
char commandBuffer[kCommandBufferSize];
size_t commandLength = 0;

const char* stateName(State value) {
  switch (value) {
    case State::IDLE: return "CHO";
    case State::AIRFLOW_CHECK: return "KIEM_TRA_GIO";
    case State::DRYING: return "DANG_SAY";
    case State::COOLING: return "LAM_NGUOI";
    case State::WAIT_FOR_REMOVAL: return "CHO_LAY_KHAN_RA";
    case State::FAULT: return "LOI";
  }
  return "KHONG_RO";
}

void enterState(State next, unsigned long now) {
  if (state == next) return;
  state = next;
  stateStartedMs = now;
  Serial.print("Chuyen trang thai: ");
  Serial.println(stateName(state));
}

void latchFault(const char* reason, unsigned long now) {
  outputs.heatersOn = false;
  outputs.uvOn = false;
  outputs.fansOn = false;
  strncpy(faultReason, reason, sizeof(faultReason) - 1);
  faultReason[sizeof(faultReason) - 1] = '\0';
  enterState(State::FAULT, now);
}

void printHelp() {
  Serial.println("Lenh mo phong (khong dieu khien GPIO):");
  Serial.println("  help             - hien lenh");
  Serial.println("  status           - in trang thai va gia tri mo phong");
  Serial.println("  sensors ok|fail  - mo phong cam bien tot/loi");
  Serial.println("  towel on|off     - mo phong treo/lay khan");
  Serial.println("  temp <do_C>      - dat nhiet do gio ra mo phong");
  Serial.println("  dry on|off       - mo phong dieu kien kho da hieu chinh");
  Serial.println("  reset            - xoa loi mo phong");
}

void printStatus(unsigned long now) {
  Serial.print("Trang thai=");
  Serial.print(stateName(state));
  Serial.print(" | cam_bien=");
  Serial.print(inputs.sensorsValid ? "OK" : "LOI");
  Serial.print(" | khan=");
  Serial.print(inputs.towelPresent ? "CO" : "KHONG");
  Serial.print(" | nhiet_do=");
  Serial.print(inputs.outletTemperatureC, 1);
  Serial.print(" C | dieu_kien_kho=");
  Serial.print(inputs.dryCondition ? "CO" : "KHONG");
  Serial.print(" | quat_ao=");
  Serial.print(outputs.fansOn ? "BAT" : "TAT");
  Serial.print(" | gia_nhiet_ao=");
  Serial.print(outputs.heatersOn ? "BAT" : "TAT");
  if (state == State::FAULT) {
    Serial.print(" | loi=");
    Serial.print(faultReason);
  }
  if (state == State::DRYING) {
    Serial.print(" | giay_mo_phong=");
    Serial.print((now - stateStartedMs) / 1000);
  }
  Serial.println();
}

void processCommand(char* line, unsigned long now) {
  char* command = strtok(line, " \t\r\n");
  if (command == nullptr) return;

  if (strcmp(command, "help") == 0) {
    printHelp();
  } else if (strcmp(command, "status") == 0) {
    printStatus(now);
  } else if (strcmp(command, "sensors") == 0) {
    char* value = strtok(nullptr, " \t\r\n");
    if (value == nullptr) {
      Serial.println("Thieu gia tri: sensors ok|fail");
    } else if (strcmp(value, "ok") == 0) {
      inputs.sensorsValid = true;
      Serial.println("Da mo phong: cam bien hop le.");
    } else if (strcmp(value, "fail") == 0) {
      inputs.sensorsValid = false;
      Serial.println("Da mo phong: cam bien loi.");
    } else {
      Serial.println("Gia tri khong hop le. Dung: sensors ok|fail");
    }
  } else if (strcmp(command, "towel") == 0) {
    char* value = strtok(nullptr, " \t\r\n");
    if (value == nullptr) {
      Serial.println("Thieu gia tri: towel on|off");
    } else if (strcmp(value, "on") == 0) {
      inputs.towelRequested = true;
      towelChangeMs = now;
      Serial.println("Da nhan lenh mo phong: treo khan.");
    } else if (strcmp(value, "off") == 0) {
      inputs.towelRequested = false;
      towelChangeMs = now;
      Serial.println("Da nhan lenh mo phong: lay khan ra.");
    } else {
      Serial.println("Gia tri khong hop le. Dung: towel on|off");
    }
  } else if (strcmp(command, "temp") == 0) {
    char* value = strtok(nullptr, " \t\r\n");
    if (value == nullptr) {
      Serial.println("Thieu gia tri. Vi du: temp 42");
    } else {
      char* end = nullptr;
      const float parsed = strtof(value, &end);
      if (end == value || *end != '\0' || parsed < -40.0f || parsed > 150.0f) {
        Serial.println("Nhiet do khong hop le (-40 den 150 C).");
      } else {
        inputs.outletTemperatureC = parsed;
        Serial.println("Da cap nhat nhiet do mo phong.");
      }
    }
  } else if (strcmp(command, "dry") == 0) {
    char* value = strtok(nullptr, " \t\r\n");
    if (value == nullptr) {
      Serial.println("Thieu gia tri: dry on|off");
    } else if (strcmp(value, "on") == 0) {
      inputs.dryCondition = true;
      Serial.println("Da mo phong: dieu kien kho dat.");
    } else if (strcmp(value, "off") == 0) {
      inputs.dryCondition = false;
      dryTimerRunning = false;
      Serial.println("Da mo phong: dieu kien kho chua dat.");
    } else {
      Serial.println("Gia tri khong hop le. Dung: dry on|off");
    }
  } else if (strcmp(command, "reset") == 0) {
    if (state != State::FAULT) {
      Serial.println("May khong o trang thai loi.");
    } else if (!inputs.sensorsValid) {
      Serial.println("Chua reset: cam bien mo phong van dang loi.");
    } else {
      faultReason[0] = '\0';
      dryTimerRunning = false;
      outputs = {};
      enterState(State::IDLE, now);
      Serial.println("Da reset loi mo phong.");
    }
  } else {
    Serial.println("Khong nhan ra lenh. Nhap help de xem lenh.");
  }
}

void readSerialCommands(unsigned long now) {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\n' || ch == '\r') {
      if (commandLength > 0) {
        commandBuffer[commandLength] = '\0';
        processCommand(commandBuffer, now);
        commandLength = 0;
      }
    } else if (commandLength < kCommandBufferSize - 1) {
      commandBuffer[commandLength++] = ch;
    } else {
      commandLength = 0;
      Serial.println("Lenh qua dai, da bo qua.");
    }
  }
}

void updateTowelDebounce(unsigned long now) {
  if (inputs.towelRequested != inputs.towelPresent &&
      now - towelChangeMs >= kTowelDebounceMs) {
    inputs.towelPresent = inputs.towelRequested;
    Serial.print("Trang thai khan da loc nhieu: ");
    Serial.println(inputs.towelPresent ? "CO KHAN" : "KHONG CO KHAN");
  }
}

void updateController(unsigned long now) {
  updateTowelDebounce(now);

  if (state == State::FAULT) {
    outputs.heatersOn = false;
    outputs.uvOn = false;
    outputs.fansOn = false;
    return;
  }

  if (!inputs.sensorsValid) {
    latchFault("Cam bien mo phong khong hop le", now);
    return;
  }

  switch (state) {
    case State::IDLE:
      outputs = {};
      if (inputs.towelPresent) {
        outputs.fansOn = true;
        enterState(State::AIRFLOW_CHECK, now);
      }
      break;

    case State::AIRFLOW_CHECK:
      outputs.fansOn = true;
      outputs.heatersOn = false;
      if (!inputs.towelPresent) {
        outputs.fansOn = false;
        enterState(State::IDLE, now);
      } else if (inputs.outletTemperatureC >= kOverheatC) {
        latchFault("Qua nhiet khi kiem tra gio", now);
      } else if (now - stateStartedMs >= kAirflowSettleMs) {
        enterState(State::DRYING, now);
      }
      break;

    case State::DRYING:
      outputs.fansOn = true;
      if (inputs.outletTemperatureC >= kOverheatC) {
        latchFault("Qua nhiet", now);
        break;
      }
      if (!inputs.towelPresent) {
        outputs.heatersOn = false;
        dryTimerRunning = false;
        enterState(State::COOLING, now);
        break;
      }
      if (inputs.outletTemperatureC < kHeaterOnC) {
        outputs.heatersOn = true;
      } else if (inputs.outletTemperatureC >= kHeaterOffC) {
        outputs.heatersOn = false;
      }

      if (inputs.dryCondition) {
        if (!dryTimerRunning) {
          dryTimerRunning = true;
          dryConditionStartedMs = now;
        } else if (now - dryConditionStartedMs >= kDryConfirmMs) {
          outputs.heatersOn = false;
          dryTimerRunning = false;
          enterState(State::COOLING, now);
          break;
        }
      } else {
        dryTimerRunning = false;
      }

      if (now - stateStartedMs >= kSimulationMaxRunMs) {
        outputs.heatersOn = false;
        dryTimerRunning = false;
        Serial.println("Da het gioi han mo phong; can kiem tra khan.");
        enterState(State::COOLING, now);
      }
      break;

    case State::COOLING:
      outputs.heatersOn = false;
      outputs.fansOn = inputs.sensorsValid && inputs.outletTemperatureC < kOverheatC;
      if (!outputs.fansOn || now - stateStartedMs >= kCooldownMs) {
        outputs.fansOn = false;
        if (inputs.towelPresent) {
          enterState(State::WAIT_FOR_REMOVAL, now);
        } else {
          enterState(State::IDLE, now);
        }
      }
      break;

    case State::WAIT_FOR_REMOVAL:
      outputs = {};
      if (!inputs.towelPresent) {
        enterState(State::IDLE, now);
      }
      break;

    case State::FAULT:
      break;
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  lastLoopMs = millis();
  lastStatusMs = lastLoopMs;
  Serial.println();
  Serial.println("MO PHONG DIEU KHIEN MAY SAY KHAN");
  Serial.println("Chi mo phong logic; khong co GPIO va khong dieu khien tai that.");
  Serial.println("Gia tri mac dinh: cam bien OK, khong co khan, nhiet do 25 C.");
  Serial.print("Thoi gian mo phong: kiem tra gio ");
  Serial.print(kAirflowSettleMs / 1000);
  Serial.print(" s; xac nhan kho ");
  Serial.print(kDryConfirmMs / 1000);
  Serial.print(" s; gioi han chu ky ");
  Serial.print(kSimulationMaxRunMs / 1000);
  Serial.println(" s.");
  printHelp();
}

void loop() {
  const unsigned long now = millis();
  readSerialCommands(now);
  if (now - lastLoopMs >= kLoopIntervalMs) {
    lastLoopMs = now;
    updateController(now);
  }
  if (now - lastStatusMs >= kStatusIntervalMs) {
    lastStatusMs = now;
    printStatus(now);
  }
}
