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
#include "helper.h"

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 1;
static constexpr size_t QUEUE_SIZE = 1;

uint8_t tx_buf[BUFFER_SIZE] = {0xAA};
uint8_t rx_buf[BUFFER_SIZE] = {0x00};

const int STIM_CONTROL = 1; // Pin para habilitar o no la estimulación
const int TIME_CONTROL = 2;  // Pin avisar del cambio de estimulación

unsigned long previous_STIM_milis = 0;
unsigned long previous_TIME_milis = 0;
bool pinState_STIM = LOW;
bool pinState_TIME = LOW;

// Stimulation timing parameters
uint32_t resting_time = 15; //15 seconds
uint16_t stimulation_time = 5; //5 seconds
uint8_t number_of_stimulations = 3; //number of trains of pulses to be sent


void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Configuración del pin de habilitación de la estimulación
    pinMode(STIM_CONTROL, OUTPUT);
    digitalWrite(STIM_CONTROL, LOW);

    Serial.println("Inicializado pin control habilitación de estimulación");

    // Configuración del pin de cambio de estimulación
    pinMode(TIME_CONTROL, OUTPUT);
    digitalWrite(TIME_CONTROL, LOW);

    Serial.println("Inicializado pin control cambio de estimulación");

    // Configuración del esclavo SPI
    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(QUEUE_SIZE);
    slave.begin(HSPI, /*SCK=*/12, /*MISO=*/13, /*MOSI=*/14, /*SS=*/15);

    Serial.println("Inicializado SPI Slave - esperando datos...");
}

void loop()
{

    // ---- SPI ----
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
    const size_t received_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

    if (received_bytes > 0) {
        Serial.print("Dato recibido del maestro: 0x");
        Serial.println(rx_buf[0], HEX);
    }
    unsigned long currentMillis = millis();


    for(int i = 0; i < number_of_stimulations; i++){ // revisar esto porque aquí ha dejado de recibir los datos del maestro SPI. Habria que hacer un if con un number_of_stimulations_done
        // Encendido del STIM ENABLE tras 5 segundos
        if (pinState_STIM == LOW && currentMillis - previous_STIM_milis >= resting_time*1000) {
            digitalWrite(STIM_CONTROL, HIGH);
            pinState_STIM = HIGH;
            previous_STIM_milis = currentMillis;
            Serial.println("→ STIM puesto en HIGH");
            
        }

        // Apagado del STIM ENABLE tras 15 segundos
        if (pinState_STIM == HIGH && currentMillis - previous_STIM_milis >= stimulation_time*1000) {
            digitalWrite(STIM_CONTROL, LOW);
            pinState_STIM = LOW;
            previous_STIM_milis = currentMillis;
            // Serial.println("→ STIM vuelto a LOW");
        }


        // Envio de un pulso de time control cada 1 segundo. Duración de 3 ms
        if (pinState_STIM == HIGH && pinState_TIME == LOW && currentMillis - previous_TIME_milis >= 1000) {
            digitalWrite(TIME_CONTROL, HIGH);
            pinState_TIME = HIGH;
            previous_TIME_milis = currentMillis;
            // Serial.println("→ TIME puesto en HIGH");
            
        }

        // Si el pin está en HIGH, esperamos 3 milisegundos para apagarlo
        if (pinState_STIM == HIGH && pinState_TIME == HIGH && currentMillis - previous_TIME_milis >= 3) {
            digitalWrite(TIME_CONTROL, LOW);
            pinState_TIME = LOW;
            previous_TIME_milis = currentMillis;
            // Serial.println("→ TIME vuelto a LOW");
        }
    }


    // Pequeña pausa opcional para evitar saturar el loop
    delay(10);
}
