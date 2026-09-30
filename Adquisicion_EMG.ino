#include <Arduino.h>
#include <ADC.h>

ADC *adc = new ADC();

// Pines y variables
const int pins[] = {14, 15, 20, 21, 22, 23};
const int numPins = 6;

// Filtro Moving Average
const int windowSize = 400; 
float windows[numPins][windowSize];
float sums[numPins] = {0};
int idx = 0;

// Tiempos de muestreo y envio
const long sampleInterval = 667; 
unsigned long prevMicros = 0;
unsigned long lastSend = 0;
const int sendInterval = 20; 

void setup() {
  Serial.begin(115200);
  Serial1.begin(2000000);

  // ADC Config
  for (int i = 0; i < 2; i++) {
    ADC_Module *module = (i == 0) ? adc->adc0 : adc->adc1;
    module->setResolution(12);
    module->setAveraging(4); 
    module->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    module->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
  }

  // Inicializar buffers
  for (int i = 0; i < numPins; i++) {
    for (int j = 0; j < windowSize; j++) windows[i][j] = 0;
  }
}

void loop() {
  unsigned long currentMicros = micros();

  // Muestreo a 1500 Hz
  if (currentMicros - prevMicros >= sampleInterval) {
    prevMicros = currentMicros;

    for (int i = 0; i < numPins; i++) {
      float val = (adc->analogRead(pins[i]) * 3.3) / 4095.0;
      float signal = abs(val - 1.5);

      sums[i] -= windows[i][idx];
      windows[i][idx] = signal;
      sums[i] += windows[i][idx];
    }
    idx = (idx + 1) % windowSize;
  }

  // Envio de datos a 50 Hz
  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    
    for (int i = 0; i < numPins; i++) {
      float avg = sums[i] / windowSize;
      float norm = (avg / 0.5) * 1.57;
      
      // Limites
      if (norm > 1.0) norm = 1.0;
      if (norm < 0.0) norm = 0.0;
      
      int out = (int)(norm * 1000);

      // Salida Serial1 (ESP32)
      Serial1.print(out);
      if (i < numPins - 1) Serial1.print(",");

      // Salida Plotter
      Serial.print("M");
      Serial.print(i + 1);
      Serial.print(":");
      Serial.print(norm);
      if (i < numPins - 1) Serial.print(",");
    }
    
    Serial1.println();
    Serial.println();
  }
}
