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
static constexpr size_t SPI_data_send = 39;
uint8_t spi_rx_buf[SPI_data_send] = { 0 };
uint8_t spi_tx_buf[SPI_data_send] = { 0 };

/* Parámetros de sensado */
uint16_t ADC_sampling_rate = 13;           
uint8_t Channels_to_convert = 0;
bool DSP_enabled = false;
bool C2_enabled = false;
bool Absolute_value_mode = false;
float DSP_cutoff_frequency = 1000.0;        
uint8_t Initial_channel = 0;
float fc_high_magnitude = 0;
char fc_high_unit = 'k';
float fc_low_A = 0;
float fc_low_B = 0;
char amplifier_cutoff = 'A';

bool RX_BT = false;

bool ACK_param = false;


/* SPI Pins */
const int STIM_INDICATOR_PIN = 9;
uint8_t HANDSHAKE_READY_PIN = 5;
uint8_t HANDSHAKE_ACK_PIN = 6;
uint8_t HANDSHAKE_SEND_PIN = 14;
const int NEW_PARAM_PIN = 11;
const int ACK_PARAM_PIN = 12;
const int ESP32_CONNECTED_PIN = 13;
uint8_t MISO_PIN_PARAM = 17;
uint8_t MOSI_PIN_PARAM = 18;
uint8_t SCLK_PIN_PARAM = 16;
uint8_t CS_ESP32_PIN_PARAM = 15;

static constexpr size_t buf_size = 60;
uint8_t tx_buf_ECG[buf_size] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
uint8_t rx_buf[buf_size] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t spi_data_index = 0;
static constexpr size_t spi_data_array_size_const = 240;
uint8_t spi_data_array[spi_data_array_size_const] = { 0 };


/* SENSE PARAMETERS*/
uint8_t high_ADC_sampling_rate;
uint8_t low_ADC_sampling_rate;
uint8_t high_high_byte_DSP_cutoff_frequency;
uint8_t high_byte_DSP_cutoff_frequency;
uint8_t low_byte_DSP_cutoff_frequency;
uint8_t low_low_byte_DSP_cutoff_frequency;
uint8_t high_high_byte_fc_high_magnitude;
uint8_t high_byte_fc_high_magnitude;
uint8_t low_byte_fc_high_magnitude;
uint8_t low_low_byte_fc_high_magnitude;
uint8_t high_high_byte_fc_low_A;
uint8_t high_byte_fc_low_A;
uint8_t low_byte_fc_low_A;
uint8_t low_low_byte_fc_low_A;
uint8_t high_high_byte_fc_low_B;
uint8_t high_byte_fc_low_B;
uint8_t low_byte_fc_low_B;
uint8_t low_low_byte_fc_low_B;

/* STIM PARAMETERS */
uint16_t resting_time = 15; //15 seconds
uint16_t stimulation_time = 5; //5 seconds
uint8_t high_byte_stimulation_time = 5; //5 seconds
uint8_t low_byte_stimulation_time = 5; //5 seconds
uint8_t number_of_stimulations = 0; //number of trains of pulses to be sent
uint8_t number_of_stimulations_done = 0;

uint8_t high_byte_resting_time;
uint8_t low_byte_resting_time;
uint8_t high_byte_stimulation_on;
uint8_t low_byte_stimulation_on;
uint8_t high_byte_stimulation_off_time;
uint8_t low_byte_stimulation_off_time;

bool bipolar_stim;


uint16_t stimulation_on_time = 0; // milisegundos
uint16_t stimulation_off_time = 0; // milisegundos



uint8_t positive_current_magnitude = 0; // milisegundos
uint8_t negative_current_magnitude = 0; // milisegundos

uint8_t high_byte_step_DAC;
uint8_t low_byte_step_DAC;

uint16_t step_DAC;

/* OTHER PARAMETERS */

bool printed = false;

bool received_ack = false;
bool sent = false;
bool parameters_updated = false;
uint32_t received_bytes;
uint32_t received_bytes_ECG;
uint32_t received_bytes_counter;




/*
  PROTOCOLO DE COMUNICACIÓN CON ESP32
*/

typedef enum {REENVIO, NUEVO_SPI_ENCOLADO, ESPERA_ACK_ON, ESPERA_FIN_ACK, RESET} Estados;
Estados general_state = REENVIO;


typedef enum {STIM_ON, STIM_OFF} Estados_estimulacion;
Estados_estimulacion previous_state = STIM_OFF;

/*
  FLAGS
*/
/* CONTROL DEL PIN DE AVISO DE NUEVOS PARÁMETROS: NEW_PARAM*/
void ON_NEW_PARAM_PIN(){
  digitalWrite(NEW_PARAM_PIN, HIGH);
}

void OFF_NEW_PARAM_PIN(){
  digitalWrite(NEW_PARAM_PIN, LOW);
}

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



bool ACK_RECEIVED_READ(){
  return digitalRead(ACK_PARAM_PIN);
}



void STIM_INDICATOR_SETUP(){
  pinMode(STIM_INDICATOR_PIN, INPUT);
  Serial.println("Inicializado pin STIM_INDICATOR_PIN");
}

void HANDSHAKE_SETUP(){
  pinMode(HANDSHAKE_READY_PIN, OUTPUT);
  pinMode(HANDSHAKE_ACK_PIN, INPUT);
  pinMode(HANDSHAKE_SEND_PIN, INPUT);

  Serial.println("Inicializado pin HANDSHAKE_READY_PIN");
}

void HANDSHAKE_READY_HIGH(){
  digitalWrite(HANDSHAKE_READY_PIN, HIGH);
  // Serial.println("\n READY high value");
}

void HANDSHAKE_READY_LOW(){
  digitalWrite(HANDSHAKE_READY_PIN, LOW);
  // Serial.println("\n READY low value");
}



bool HANDSHAKE_ACK_VALUE(){
  bool handshake_ack_value = digitalRead(HANDSHAKE_ACK_PIN);
  Serial.printf("ACK value: %d\n", handshake_ack_value);
  return handshake_ack_value;
}

bool HANDSHAKE_SEND_VALUE(){
  bool send_value = digitalRead(HANDSHAKE_SEND_PIN);
  // Serial.printf("SEND value: %d\n", send_value);

  return send_value;
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

    // Serial.print("Datos recibidos: ");
    // Serial.println(value);

    // Parse robusto por comas
    int max_tokens = 21;
    String tokens[max_tokens]; // Esperamos 10 parámetros
    int start = 0;
    int commaIndex = value.indexOf(',');
    int i = 0;
    while (commaIndex != -1) {
      tokens[i++] = value.substring(start, commaIndex);
      start = commaIndex + 1;
      commaIndex = value.indexOf(',', start);
    }
    tokens[i++] = value.substring(start); // último token

    if (i == max_tokens) { // Validamos cantidad correcta
      /* SENSING PARAMETERS */
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
      C2_enabled = tokens[10].toInt();
      Absolute_value_mode = tokens[11].toInt();

      /* STIMULATION PARAMETERS */
      number_of_stimulations = tokens[12].toInt();
      resting_time = tokens[13].toInt();
      stimulation_time = tokens[14].toInt();
      stimulation_on_time = tokens[15].toInt();
      stimulation_off_time = tokens[16].toInt();
      bipolar_stim = (bool)tokens[17].toInt();

      positive_current_magnitude = tokens[18].toInt();
      negative_current_magnitude = tokens[19].toInt();
      step_DAC = (uint16_t)tokens[20].toInt();

      number_of_stimulations_done = 0;

      Serial.println("Parámetros actualizados correctamente:");
      Serial.printf("----------SENSE PARAMETERS-----\n");
      Serial.printf("ADC: %d, DSP freq: %.2f, Channels: %d\n", ADC_sampling_rate, DSP_cutoff_frequency, Channels_to_convert);
      Serial.printf("DSP enabled: %d, Initial ch: %d\n", DSP_enabled, Initial_channel);
      Serial.printf("fc_high: %.2f%c, fc_low_A: %.2f, fc_low_B: %.2f\n", fc_high_magnitude, fc_high_unit, fc_low_A, fc_low_B);
      Serial.printf("amplifier_cutoff: %c\n", amplifier_cutoff);

      Serial.printf("C2_enabled: %d\n", C2_enabled);
      Serial.printf("Absolute_value_mode: %d\n", Absolute_value_mode);

      Serial.printf("----------STIMULATION PARAMETERS-----\n");
      Serial.printf("number_of_stimulations: %d\n", number_of_stimulations);
      Serial.printf("resting_time: %d\n", resting_time);
      Serial.printf("stimulation_time: %d\n", stimulation_time);
      Serial.printf("stimulation_on_time: %d\n", stimulation_on_time);
      Serial.printf("stimulation_off_time: %d\n", stimulation_off_time);
      Serial.printf("positive_current_magnitude: %d\n", positive_current_magnitude);
      Serial.printf("negative_current_magnitude: %d\n", negative_current_magnitude);
      Serial.printf("step_DAC: %d\n", step_DAC);
      Serial.printf("bipolar_stim: %d\n", bipolar_stim);




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




void STIM_INDICATOR() {
  if (!digitalRead(STIM_INDICATOR_PIN)) {
    if (previous_state == STIM_OFF) {
      previous_state = STIM_ON;
      // Serial.printf("Stimulation change: ON\n");

      // 🔹 Enviar mensaje BLE indicando ON
      if (pCharacteristicTX != nullptr) {
        // Serial.printf("STIM ON packet sent");
        const char *msg = "STIM_ON";
        pCharacteristicTX->setValue((uint8_t*)msg, strlen(msg));
        pCharacteristicTX->notify();
      }
    }
  } else {
    if (previous_state == STIM_ON) {
      previous_state = STIM_OFF;
      // Serial.printf("Stimulation change: OFF\n");
      // 🔹 Enviar mensaje BLE indicando OFF
      if (pCharacteristicTX != nullptr) {
        // Serial.printf("STIM OFF packet sent");
        const char *msg = "STIM_OFF";
        pCharacteristicTX->setValue((uint8_t*)msg, strlen(msg));
        pCharacteristicTX->notify();
      }
    }
  }
}



/* ---------------- SETUP ---------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  STIM_INDICATOR_SETUP();
  NEW_PARAMETERS_SETUP();
  ACK_RECEIVE_SETUP();
  ESP32_CONNECTED();
  SPI_SETUP();
  BLE_SETUP();
  HANDSHAKE_SETUP();
  HANDSHAKE_READY_HIGH();
  initializeBuffers(tx_buf_ECG, rx_buf, buf_size);

}

/* ---------------- LOOP ---------------- */
void loop() {
  STIM_INDICATOR();
  switch(general_state){
    case REENVIO: // Reenvío por BLE de lo obtenido por SPI
      if(!printed){
        Serial.println("\n Estado 1: Evío del ECG por SPI.");
        printed = true;
      }
      if (RX_BT == 1){
        general_state = NUEVO_SPI_ENCOLADO;
        printed = false;
        break;

      }else{

        // ---- SPI ----

        initializeBuffers(tx_buf_ECG, rx_buf, buf_size);
        
        //check send value with timeout
        unsigned long t0 = millis();
        while (HANDSHAKE_SEND_VALUE()) {
          if (millis() - t0 > 50) {   // 1 ms
            // Serial.println("Timeout SEND");
            HANDSHAKE_READY_HIGH();
            return;
          }
        }

        HANDSHAKE_READY_LOW();
        uint32_t received_bytes_ECG = slave.transfer(tx_buf_ECG, rx_buf, buf_size);
        
        for (uint8_t i = 0; i < buf_size; i++) {
          spi_data_array[spi_data_index++] = rx_buf[i];
        }

        // Serial.printf("%d \n", spi_data_index);
        if (spi_data_index >= spi_data_array_size_const) {
          delayMicroseconds(1);        
          if (pCharacteristicTX != nullptr) {
            pCharacteristicTX->setValue(spi_data_array, spi_data_array_size_const);
            pCharacteristicTX->notify();
          }
          // Serial.print("\n SENT VALUES BLE");
          HANDSHAKE_READY_HIGH();
          spi_data_index = 0;
        }

        HANDSHAKE_READY_HIGH();
      }
      break;

    case NUEVO_SPI_ENCOLADO: // Nuevo SPI encolado

      if(!printed){
        Serial.println("\n Estado 2: Encolar nuevo SPI.");
        printed = true;
      }

      RX_BT = 0;

      
      // ---- CONVERSIÓN DE FLOATS A BYTES ----
      uint8_t dsp_bytes[4];
      uint8_t fc_high_bytes[4];
      uint8_t fc_low_A_bytes[4];
      uint8_t fc_low_B_bytes[4];

      memcpy(dsp_bytes, &DSP_cutoff_frequency, 4);
      memcpy(fc_high_bytes, &fc_high_magnitude, 4);
      memcpy(fc_low_A_bytes, &fc_low_A, 4);
      memcpy(fc_low_B_bytes, &fc_low_B, 4);

      
      high_byte_stimulation_on = (stimulation_on_time >> 8) & 0xFF;
      low_byte_stimulation_on = stimulation_on_time & 0xFF;

      high_byte_stimulation_time = (stimulation_time >> 8) & 0xFF;
      low_byte_stimulation_time = stimulation_time & 0xFF;

      high_byte_stimulation_off_time= (stimulation_off_time>> 8) & 0xFF;
      low_byte_stimulation_off_time= stimulation_off_time& 0xFF;

      high_byte_resting_time = (resting_time >> 8) & 0xFF;
      low_byte_resting_time = resting_time & 0xFF;

      high_byte_step_DAC = (step_DAC >> 8) & 0xFF;
      low_byte_step_DAC = step_DAC & 0xFF;

      // ---- RELLENAR BUFFER SPI ----
      // SENSING PARAMETERS
      spi_tx_buf[0] = (ADC_sampling_rate >> 8) & 0xFF;
      spi_tx_buf[1] = ADC_sampling_rate & 0xFF;

      spi_tx_buf[2] = Channels_to_convert;
      spi_tx_buf[3] = DSP_enabled;

      spi_tx_buf[4] = dsp_bytes[0];
      spi_tx_buf[5] = dsp_bytes[1];
      spi_tx_buf[6] = dsp_bytes[2];
      spi_tx_buf[7] = dsp_bytes[3];

      spi_tx_buf[8] = Initial_channel;

      spi_tx_buf[9]  = fc_high_bytes[0];
      spi_tx_buf[10] = fc_high_bytes[1];
      spi_tx_buf[11] = fc_high_bytes[2];
      spi_tx_buf[12] = fc_high_bytes[3];

      spi_tx_buf[13] = fc_high_unit;

      spi_tx_buf[14] = fc_low_A_bytes[0];
      spi_tx_buf[15] = fc_low_A_bytes[1];
      spi_tx_buf[16] = fc_low_A_bytes[2];
      spi_tx_buf[17] = fc_low_A_bytes[3];

      spi_tx_buf[18] = fc_low_B_bytes[0];
      spi_tx_buf[19] = fc_low_B_bytes[1];
      spi_tx_buf[20] = fc_low_B_bytes[2];
      spi_tx_buf[21] = fc_low_B_bytes[3];

      spi_tx_buf[22] = amplifier_cutoff;

      spi_tx_buf[23] = C2_enabled;
      spi_tx_buf[24] = Absolute_value_mode;

      // STIMULATION PARAMETERS
      spi_tx_buf[25] = number_of_stimulations;

      spi_tx_buf[26] = high_byte_resting_time;
      spi_tx_buf[27] = low_byte_resting_time;
      spi_tx_buf[28] = high_byte_stimulation_time;
      spi_tx_buf[29] = low_byte_stimulation_time;
      spi_tx_buf[30] = high_byte_stimulation_on;   // high_byte_stimulation_on
      spi_tx_buf[31] = low_byte_stimulation_on;    // low_byte_stimulation_on
      spi_tx_buf[32] = high_byte_stimulation_off_time;
      spi_tx_buf[33] = low_byte_stimulation_off_time;
      spi_tx_buf[34] = positive_current_magnitude;
      spi_tx_buf[35] = negative_current_magnitude;
      spi_tx_buf[36] = high_byte_step_DAC;         // high_byte_step_DAC
      spi_tx_buf[37] = low_byte_step_DAC;          // low_byte_step_DAC
      spi_tx_buf[38] = bipolar_stim;

      ON_NEW_PARAM_PIN();
      Serial.println("Before transfer parameters.");

      received_bytes = slave.transfer(spi_tx_buf, spi_rx_buf, SPI_data_send);

      Serial.println("After transfer parameters.");


      general_state = ESPERA_ACK_ON;

      printed = false;
      
      break;

    case ESPERA_ACK_ON: // Espera al ACK

      if(!printed){
        Serial.println("\n Estado 3: espera al ACK.");
        printed = true;
      }
      
      delay(1000);
      ACK_param = ACK_RECEIVED_READ();
      if(ACK_param){
        general_state = ESPERA_FIN_ACK;
        printed = false;
      }

      break;
    
    case ESPERA_FIN_ACK: // Espera al fin del ACK
      if(!printed){
        Serial.println("\n Estado 4: espera al fin del ACK.");
        printed = true;
      }


      ACK_param = ACK_RECEIVED_READ();
      if(!ACK_param){
        general_state = RESET;
        printed = false;
      }
      OFF_NEW_PARAM_PIN();


      break;

    case RESET:
      if(!printed){
        Serial.println("\n Estado 5: reset");
        printed = true;
      }

      general_state = REENVIO;
      printed = false;
      break;
    default:
      Serial.println("Opción de estado no válida");
      break;

  }
}
