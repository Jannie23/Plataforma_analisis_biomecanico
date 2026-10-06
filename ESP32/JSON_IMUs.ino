#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Credenciales y Servidor
const char* ssid = "jim";
const char* password = "mariposa";
const char* host = "10.39.29.78";
const int port = 82;

String jsonBuffer = "";

void setup() {
  Serial.begin(115200);
  
  // Comunicacion Teensy
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado");
}

void loop() {
  // Lectura Serial2
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      jsonBuffer.trim();
      if (jsonBuffer.length() > 0) {
        procesarJSON(jsonBuffer);
      }
      jsonBuffer = "";
    } else if (c != '\r') {
      jsonBuffer += c;
    }
  }
}

void procesarJSON(String& data) {
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.print("Error JSON: ");
    Serial.println(error.c_str());
    return;
  }

  int numImus = 0;
  if (doc.containsKey("imus")) {
    numImus = doc["imus"].size();
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }

  // Envio Flask
  HTTPClient http;
  String url = String("http://") + host + ":" + port + "/update_imu";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(data);
  http.end();

  if (code == 200) {
    Serial.printf("Enviado: %d IMUs\n", numImus);
  } else {
    Serial.printf("Error HTTP: %d\n", code);
  }
}
