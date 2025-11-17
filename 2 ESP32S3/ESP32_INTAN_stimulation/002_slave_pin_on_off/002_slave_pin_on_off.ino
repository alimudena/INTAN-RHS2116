/************************************************************
 *                CONEXIÓN SPI - ESP32-S3 (ESCLAVO)
 * ----------------------------------------------------------
 *  Señal SPI | Dirección | ESP32-S3 (Slave) | Maestro SPI
 * ----------------------------------------------------------
 *  MISO      |  →        | GPIO 13  -  D13  | MOSI
 *  MOSI      |  ←        | GPIO 14  -  A4   | MISO
 *  SCLK      |  ←        | GPIO 12  -  D12  | SCLK
 *  CS (SS)   |  ←        | GPIO 15  -  A5   | CS / SS
 *  GND       |  ↔        | GND              | GND
 * ----------------------------------------------------------
 *  Nota:
 *  - Las flechas indican la dirección del flujo de datos.
 *  - El maestro controla las líneas SCLK y CS.
 *  - La librería ESP32SPISlave usa HSPI (SPI2) por defecto.
 *  - Puedes reasignar los pines si es necesario usando:
 *       slave.begin(HSPI, misoPin, mosiPin, sclkPin, ssPin);
 ************************************************************/


#include <ESP32SPISlave.h>

// ---- EXTERNAL PIN CONTROL ----
const int PIN_CONTROL = 2;  // El pin que quieres poner a HIGH y luego LOW
unsigned long previousMillis = 0;
bool pinState = LOW;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Configuración del pin de salida
    pinMode(PIN_CONTROL, OUTPUT);
    digitalWrite(PIN_CONTROL, LOW);

    Serial.println("Inicializado pin control...");
}

void loop()
{
    unsigned long currentMillis = millis();

    // Si el pin está en LOW, esperamos 3 segundos (3000 ms)
    if (pinState == LOW && currentMillis - previousMillis >= 3000) {
        digitalWrite(PIN_CONTROL, HIGH);
        pinState = HIGH;
        previousMillis = currentMillis;
        Serial.println("→ Pin puesto en HIGH");
    }

    // Si el pin está en HIGH, esperamos 3 milisegundos para apagarlo
    if (pinState == HIGH && currentMillis - previousMillis >= 3) {
        digitalWrite(PIN_CONTROL, LOW);
        pinState = LOW;
        previousMillis = currentMillis;
        Serial.println("→ Pin vuelto a LOW");
    }

    // Pequeña pausa opcional para evitar saturar el loop
    delay(1);
}
