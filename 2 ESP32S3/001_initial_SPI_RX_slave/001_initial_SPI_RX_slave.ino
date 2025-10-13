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

// Solo queremos 1 byte de transferencia
static constexpr size_t BUFFER_SIZE = 1;
static constexpr size_t QUEUE_SIZE = 1;

// Buffers de transmisión y recepción de 1 byte
uint8_t tx_buf[BUFFER_SIZE] = {0xAA};  // Valor de prueba
uint8_t rx_buf[BUFFER_SIZE] = {0x00};

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Configuración del esclavo SPI
    slave.setDataMode(SPI_MODE0);   // Modo SPI 0 (CPOL=0, CPHA=0)
    slave.setQueueSize(QUEUE_SIZE); // Cola de una transacción

    // Iniciar SPI en modo esclavo (por defecto usa HSPI)
    // slave.begin();  

    // Si estás usando un ESP32-S3-DevKitC o similar, estos pines pueden variar ligeramente dependiendo del módulo.
    slave.begin(HSPI, /*SCK=*/12, /*MISO=*/13, /*MOSI=*/14, /*SS=*/15);


    Serial.println("SPI Slave iniciado - esperando datos...");
}

void loop()
{
    // Inicializa buffers antes de cada transferencia
    initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);

    // Transferencia de UN solo byte
    const size_t received_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

    // Si se recibió algo, mostrarlo
    if (received_bytes > 0) {
        Serial.print("Dato recibido del maestro: 0x");
        Serial.println(rx_buf[0], HEX);
    }

    delay(10); // Pequeña pausa
}
