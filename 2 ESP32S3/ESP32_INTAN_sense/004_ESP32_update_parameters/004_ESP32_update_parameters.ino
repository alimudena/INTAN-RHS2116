/*
  SPI + BLE Receptor ESP32
*/

#include <ESP32SPISlave.h>
#include "helper.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

ESP32SPISlave slave;

/* SPI reenvío de nuevos parámetros */
static constexpr size_t SPI_data_send = 10;
uint8_t spi_rx_buf[SPI_data_send] = { 0 };
uint8_t spi_tx_buf[SPI_data_send] = { 0 };

/* Parámetros de sensado */
uint16_t ADC_sampling_rate = 13;           
uint8_t Channels_to_convert = 0;
bool DSP_enabled = false;
float DSP_cutoff_frequency = 1000.0;        
uint8_t Initial_channel = 0;
float fc_high_magnitude = 0;
char fc_high_unit = 'k';
float fc_low_A = 0;
float fc_low_B = 0;
char amplifier_cutoff = 'A';

bool RX_BT = false;

/* SPI Pins */
const int NEW_PARAM_PIN = 11;
const int ACK_PARAM_PIN = 12;
const int ESP32_CONNECTED_PIN = 13;
uint8_t MISO_PIN_PARAM = 17;
uint8_t MOSI_PIN_PARAM = 18;
uint8_t SCLK_PIN_PARAM = 16;
uint8_t CS_ESP32_PIN_PARAM = 15;

static constexpr size_t buf_size = 3;
uint8_t tx_buf_ECG[buf_size] = { 0xAA };
uint8_t rx_buf[buf_size] = { 0x00 };
uint32_t spi_data_index = 0;
static constexpr size_t spi_data_array_size_const = 300;
uint8_t spi_data_array[spi_data_array_size_const] = { 0 };

void NEW_PARAMETERS_SETUP() {
  pinMode(NEW_PARAM_PIN, OUTPUT);
  digitalWrite(NEW_PARAM_PIN, LOW);
  Serial.println("Inicializado pin NEW_PARAM_PIN");
}

void ACK_RECEIVE_SETUP() {
  pinMode(ACK_PARAM_PIN, INPUT);
  Serial.println("Inicializado pin ACK_PARAM_PIN");
}

void SPI_SETUP() {
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(SPI_data_send);
  slave.begin(SPI2_HOST, SCLK_PIN_PARAM, MISO_PIN_PARAM, MOSI_PIN_PARAM, CS_ESP32_PIN_PARAM);
  Serial.println("Inicializado SPI Slave_PARAM.");
}

void ESP32_CONNECTED() {
  pinMode(ESP32_CONNECTED_PIN, OUTPUT);
  digitalWrite(ESP32_CONNECTED_PIN, HIGH);
  Serial.println("ESP32 conectado");
}

/* ---------------- BLE CONFIG ---------------- */
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define SERVICE_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pCharacteristicTX;
BLECharacteristic *pCharacteristicRX;

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    Serial.print("Datos recibidos: ");
    Serial.println(value);

    // Parse robusto por comas
    String tokens[10]; // Esperamos 10 parámetros
    int start = 0;
    int commaIndex = value.indexOf(',');
    int i = 0;
    while (commaIndex != -1 && i < 9) {
      tokens[i++] = value.substring(start, commaIndex);
      start = commaIndex + 1;
      commaIndex = value.indexOf(',', start);
    }
    tokens[i++] = value.substring(start); // último token

    if (i == 10) { // Validamos cantidad correcta
      ADC_sampling_rate = tokens[0].toInt();
      DSP_cutoff_frequency = tokens[1].toFloat();
      Channels_to_convert = tokens[2].toInt();
      DSP_enabled = tokens[3].toInt();
      Initial_channel = tokens[4].toInt();
      fc_high_magnitude = tokens[5].toFloat();
      fc_high_unit = tokens[6].charAt(0);
      fc_low_A = tokens[7].toFloat();
      fc_low_B = tokens[8].toFloat();
      amplifier_cutoff = tokens[9].charAt(0);

      Serial.println("Parámetros actualizados correctamente:");
      Serial.printf("ADC: %d, DSP freq: %.2f, Channels: %d\n", ADC_sampling_rate, DSP_cutoff_frequency, Channels_to_convert);
      Serial.printf("DSP enabled: %d, Initial ch: %d\n", DSP_enabled, Initial_channel);
      Serial.printf("fc_high: %.2f%c, fc_low_A: %.2f, fc_low_B: %.2f\n", fc_high_magnitude, fc_high_unit, fc_low_A, fc_low_B);
      Serial.printf("amplifier_cutoff: %c\n", amplifier_cutoff);

      RX_BT = true;
    } else {
      Serial.println("Error: número de parámetros incorrecto");
    }
  }
};

void BLE_SETUP() {
  Serial.println("Iniciando BLE ESP32...");
  BLEDevice::init("ESP32S3_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // RX
  pCharacteristicRX = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicRX->setCallbacks(new MyCallbacks());

  // TX
  pCharacteristicTX = pService->createCharacteristic(
    SERVICE_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("Esperando conexión BLE...");
}

/* ---------------- SETUP ---------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  NEW_PARAMETERS_SETUP();
  ACK_RECEIVE_SETUP();
  ESP32_CONNECTED();
  SPI_SETUP();
  BLE_SETUP();
}

/* ---------------- LOOP ---------------- */
void loop() {
  // ---- SPI ----
  initializeBuffers(tx_buf_ECG, rx_buf, buf_size);
  uint32_t received_bytes_ECG = slave.transfer(tx_buf_ECG, rx_buf, buf_size);

  for (uint8_t i = 0; i < buf_size; i++) {
    spi_data_array[spi_data_index++] = rx_buf[i];
  }

  if (spi_data_index >= spi_data_array_size_const) {
    delayMicroseconds(1);
    if (pCharacteristicTX != nullptr) {
      pCharacteristicTX->setValue(spi_data_array, spi_data_array_size_const);
      pCharacteristicTX->notify();
    }
    spi_data_index = 0;
  }
}
