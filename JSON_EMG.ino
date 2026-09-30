#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Credenciales y URL
const char* ssid = "jim";
const char* password = "mariposa";
const char* url = "http://10.39.29.78:82/update_emg";

// Batching EMG
const int CHANNELS = 6;
const int BATCH_SIZE = 50;
int count = 0;
uint16_t buffer[BATCH_SIZE][CHANNELS];

HTTPClient http;

// WiFi
void connectWiFi() {
  Serial.print("WiFi: ");
  WiFi.begin(ssid, password);
  
  int att = 0;
  while (WiFi.status() != WL_CONNECTED && att < 20) {
    delay(500);
    Serial.print(".");
    att++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado");
  } else {
    ESP.restart();
  }
}

// Envio de datos
void sendBatch() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }

  DynamicJsonDocument doc(10000); 
  JsonArray batch = doc.to<JsonArray>();

  for (int i = 0; i < BATCH_SIZE; i++) {
    JsonArray sample = batch.createNestedArray();
    for (int j = 0; j < CHANNELS; j++) {
      sample.add(buffer[i][j]);
    }
  }

  String out;
  serializeJson(doc, out);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(out);
  if (code > 0) {
    Serial.printf("Enviado: %d\n", code);
  } else {
    Serial.printf("Error: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

// Parsear CSV de Teensy
void parseCSV(String line) {
  int last = -1;
  int current = 0;

  for (int i = 0; i < CHANNELS; i++) {
    current = line.indexOf(',', last + 1);
    String val = (current == -1) ? line.substring(last + 1) : line.substring(last + 1, current);

    buffer[count][i] = (uint16_t)val.toInt();
    last = current;

    if (last == -1 && i < CHANNELS - 1) return;
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(2000000, SERIAL_8N1, 16, 17); 
  
  connectWiFi();
}

void loop() {
  if (Serial2.available() > 0) {
    String line = Serial2.readStringUntil('\n');
    line.trim(); 

    if (line.length() > 0) {
      parseCSV(line);
      count++; 

      if (count >= BATCH_SIZE) {
        sendBatch();
        count = 0; 
      }
    }
  }
}