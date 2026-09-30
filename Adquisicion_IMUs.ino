#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <ArduinoJson.h> 

// Config IMU
#define NUM_IMUS 6
#define NUM_BUSES 3

TwoWire* buses[NUM_BUSES] = { &Wire, &Wire1, &Wire2 };
uint8_t addrs[] = { 0x28, 0x29 };
Adafruit_BNO055* imus[NUM_IMUS];

int calibCountdown[NUM_IMUS] = { 0 };

// Tiempos (50 Hz)
const unsigned long imuInterval = 20; 
unsigned long lastImuTime = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  delay(500);

  // Inicializar Buses
  for (int i = 0; i < NUM_BUSES; i++) {
    buses[i]->begin();
  }

  // Inicializar IMUs
  for (int i = 0; i < NUM_IMUS; i++) {
    int bIdx = i / 2;
    int aIdx = i % 2;

    imus[i] = new Adafruit_BNO055(55 + i, addrs[aIdx], buses[bIdx]);

    if (!imus[i]->begin()) {
      Serial.printf("Error IMU %d\n", i + 1);
      imus[i] = nullptr;
      continue;
    }

    imus[i]->setExtCrystalUse(true);
    calibCountdown[i] = -1;
    Serial.printf("IMU %d OK\n", i + 1);
  }
}

void loop() {
  unsigned long now = millis();

  // Lectura y envio JSON
  if (now - lastImuTime >= imuInterval) {
    lastImuTime = now;

    StaticJsonDocument<2048> json;
    JsonArray imusArr = json.createNestedArray("imus");
    int activas = 0;

    for (int i = 0; i < NUM_IMUS; i++) {
      if (imus[i] == nullptr) continue;

      imu::Quaternion q = imus[i]->getQuat();
      if (isnan(q.w())) continue;

      activas++;

      JsonObject obj = imusArr.createNestedObject();
      obj["id"] = i + 1;
      obj["w"] = q.w();
      obj["x"] = q.x();
      obj["y"] = q.y();
      obj["z"] = q.z();
      obj["temp"] = imus[i]->getTemp();

      uint8_t s, g, a, m;
      imus[i]->getCalibration(&s, &g, &a, &m);

      if (s == 3 && g == 3 && a == 3 && m == 3 && calibCountdown[i] == -1)
        calibCountdown[i] = 15;

      if (calibCountdown[i] == -1 || calibCountdown[i] > 0) {
        JsonObject cObj = obj.createNestedObject("calib");
        cObj["sys"] = s;
        cObj["gyro"] = g;
        cObj["accel"] = a;
        cObj["mag"] = m;
        if (calibCountdown[i] > 0) calibCountdown[i]--;
      }
    }

    if (activas > 0) {
      String out;
      serializeJson(json, out);
      Serial1.println(out);
      Serial.printf("Enviado: %d IMUs\n", activas);
    }
  }
}
