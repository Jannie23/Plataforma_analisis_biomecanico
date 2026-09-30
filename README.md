# Plataforma de análisis biomecánico de la sentadilla mediante IMUs y EMGs

Este repositorio contiene los scripts de adquisición de las señales, procesamiento digital, el envío de la información y el servidor de comunicación para la representación 3D del movimiento de nuestra plataforma de análisis. Además, los videos de las diferentes pruebas y muestras tomadas a lo largo del desarrollo del proyecto.

## 📁 Estructura del Proyecto

### 1. Adquisición y Emisión de Datos (Arduino/Teensy)
* `Adquisicion_EMG.ino`: Captura directa y procesamiento de señales electromiográficas. (Cargar en Teensy)
* `Adquisicion_IMUs.ino`: Captura de datos inerciales para la cinemática. (Cargar en Teensy)
* `JSON_EMG.ino`: Estructuración de datos EMG en formato JSON para transmisión (Cargar en ESP32)
* `JSON_IMUs.ino`: Estructuración de datos IMU en formato JSON para transmisión serie/red. (Cargar en ESP32)

### 2. Procesamiento Digital (MATLAB)
* `finalDigital.m`: Script de análisis, filtrado digital avanzado y procesamiento de señales.

### 3. Servidor y Modelo 3D (Python)
* `ServidorFlask.py`: Servidor web local en Flask encargado de recibir los datos en tiempo y gestionarlos.
* `Modelo3D.py`: Script de visualización 3D e integración de la cinemática.
