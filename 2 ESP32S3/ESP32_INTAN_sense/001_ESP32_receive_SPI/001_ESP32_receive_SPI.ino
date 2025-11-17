/*
  SPI
*/

#include <ESP32SPISlave.h>
#include "helper.h"

ESP32SPISlave slave;


/* SPI reenvío de nuevos parámetros */
static constexpr size_t SPI_data_send = 10;

uint8_t spi_rx_buf[SPI_data_send] = {0};
uint8_t spi_tx_buf[SPI_data_send] = {0};

uint8_t high_byte_stimulation_on;
uint8_t low_byte_stimulation_on;

uint8_t high_byte_step_DAC;
uint8_t low_byte_step_DAC;



/* ECG reenvío por SPI */
static constexpr size_t BUFFER_SIZE = 50;

uint8_t tx_buf_ECG[3] = {0xAA};
uint8_t rx_buf[3] = {0x00};


uint8_t spi_data_array[BUFFER_SIZE] = {0};
uint32_t spi_data_index = 0;
uint32_t SPI_DATA_ARRAY_SIZE = 50;



/*
  STIMULATION CONTROL
*/
const int NEW_PARAM_PIN = 11; // Pin para pedir mandar los parámetros por SPI
const int ACK_PARAM_PIN = 12;  // Pin para recibir el ACK del enío por SPI
const int ESP32_CONNECTED_PIN = 13;  // Pin para avisar de si está conectado el ESP32 o no 

// Parámetros de tiempos de estimulación
uint8_t resting_time = 15; //15 seconds
uint8_t stimulation_time = 5; //5 seconds
uint8_t number_of_stimulations = 0; //number of trains of pulses to be sent
uint8_t number_of_stimulations_done = 0;

uint16_t stimulation_on_time = 0; // milisegundos
uint8_t stimulation_off_time = 0; // milisegundos

uint8_t positive_current_magnitude = 0; // milisegundos
uint8_t negative_current_magnitude = 0; // milisegundos

uint16_t step_DAC;



bool received_ack = false;
bool sent = false;
bool parameters_updated = false;
uint32_t received_bytes;
uint32_t received_bytes_ECG;
uint32_t received_bytes_counter;

/*
  SPI PINS
*/
uint8_t MISO_PIN_PARAM = 17;
uint8_t MOSI_PIN_PARAM = 18;
uint8_t SCLK_PIN_PARAM = 16;
uint8_t CS_ESP32_PIN_PARAM = 15;


/*
  PROTOCOLO DE COMUNICACIÓN CON ESP32
*/

typedef enum {REENVIO, NUEVO_SPI_ENCOLADO, ESPERA_ACK_ON, ESPERA_FIN_ACK, RESET} Estados;
Estados general_state = REENVIO;

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

bool RX_BT = false;

bool ACK_param = false;

void NEW_PARAMETERS_SETUP(){
    // Configuración del pin aviso de que existen nuevos parámetros
    pinMode(NEW_PARAM_PIN, OUTPUT);
    OFF_NEW_PARAM_PIN();

    Serial.println("Inicializado pin para avisar de que existen nuevos parámetros.");
}

void ACK_RECEIVE_SETUP(){
    // Configuración del pin de recepción del ACK por la existencia de nuevos parámetros
    pinMode(ACK_PARAM_PIN, INPUT);

    Serial.println("Inicializado pin para recibir el ACK por parte del maestro");
}

void SPI_SETUP(){
  // Configuración del esclavo SPI para el envío de parámetros
    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(SPI_data_send);
    // slave.begin(HSPI, 16, 17, 18, 15);//uint8_t spi_bus, int sck, int miso, int mosi, int ss
    slave.begin(SPI2_HOST, SCLK_PIN_PARAM, MISO_PIN_PARAM, MOSI_PIN_PARAM, CS_ESP32_PIN_PARAM);//uint8_t spi_bus, int sck, int miso, int mosi, int ss

    Serial.println("Inicializado SPI Slave_PARAM.");
}

void ESP32_CONNECTED(){
    // Configuración del pin de aviso de nuevos parámetros
    pinMode(ESP32_CONNECTED_PIN, OUTPUT);
    digitalWrite(ESP32_CONNECTED_PIN, HIGH);

    Serial.println("Inicializado pin control de nuevos parámetros");
}

bool ACK_RECEIVED_READ(){
  return digitalRead(ACK_PARAM_PIN);
}


/*
  BLE CONFIGURACION DE PARAMETROS
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UUID del servicio BLE
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX
#define SERVICE_UUID_TX     "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"   // TX
BLECharacteristic *pCharacteristicTX;  // referencia BLE TX
BLECharacteristic *pCharacteristicRX;  // referencia BLE RX

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
   
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
     
      Serial.print("Datos recibidos: ");
      Serial.println(value);

      // Separar los datos por comas
      int Comma1 = value.indexOf(',');
      int Comma2 = value.indexOf(',', Comma1 + 1);
      int Comma3 = value.indexOf(',', Comma2 + 1);
      int Comma4 = value.indexOf(',', Comma3 + 1);
      int Comma5 = value.indexOf(',', Comma4 + 1);
      int Comma6 = value.indexOf(',', Comma5 + 1);
      int Comma7 = value.indexOf(',', Comma6 + 1);
      sent = false;

      if (Comma1 != -1 && Comma2 != -1 && Comma3 != -1 && Comma4 != -1 && Comma5 != -1 && Comma6 != -1 && Comma7 != -1 ) {
        // Convertir cada substring a número
        number_of_stimulations = (uint8_t)value.substring(0, Comma1).toInt();
        resting_time = (uint32_t)value.substring(Comma1 + 1, Comma2).toInt();
        stimulation_time = (uint16_t)value.substring(Comma2 + 1, Comma3).toInt();

        stimulation_on_time = value.substring(Comma3 + 1, Comma4).toInt();
        stimulation_off_time = value.substring(Comma4 + 1, Comma5).toInt();

        positive_current_magnitude = value.substring(Comma5 + 1, Comma6).toInt();
        negative_current_magnitude = value.substring(Comma6 + 1, Comma7).toInt();
        
        step_DAC = (uint16_t)value.substring(Comma7 + 1).toInt();

        number_of_stimulations_done = 0;
        Serial.print("number_of_stimulations: ");
        Serial.println(number_of_stimulations);
        Serial.print("resting_time: ");
        Serial.println(resting_time);
        Serial.print("stimulation_time: ");
        Serial.println(stimulation_time);
        Serial.print("stimulation_on_time: ");
        Serial.println(stimulation_on_time);
        Serial.print("stimulation_off_time: ");
        Serial.println(stimulation_off_time);
        Serial.print("positive_current_magnitude: ");
        Serial.println(positive_current_magnitude);
        Serial.print("negative_current_magnitude: ");
        Serial.println(negative_current_magnitude);
        Serial.print("step_DAC: ");
        Serial.println(step_DAC);

      } else {
        Serial.println("Error: datos no tienen el formato correcto.");
      }
    } 
    RX_BT = true;
  }
  

};

void BLE_SETUP(){ 
  Serial.println("Iniciando BLE en ESP32-S3...");

  BLEDevice::init("ESP32S3_BLE"); // Nombre visible del dispositivo BLE
  BLEDevice::setMTU(517);
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Característica RX (para recibir datos)
  pCharacteristicRX = pService->createCharacteristic(
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristicRX->setCallbacks(new MyCallbacks());

  // Característica TX (para enviar datos)
  pCharacteristicTX = pService->createCharacteristic(
    SERVICE_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pService->start();

  // Inicia la publicidad (para que otros dispositivos lo vean)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("Esperando conexión BLE...");
}

bool printed = false;

void setup() {
  // setCpuFrequencyMhz(8);  // Fija CPU a 80 MHz
  Serial.begin(115200);
  delay(2000);

  /*
    PROTOCOLO DE NUEVOS PARÁMETROS SETUP
  */
  NEW_PARAMETERS_SETUP();
  ACK_RECEIVE_SETUP();

  /*
    AVISAR DE QUE ESTÁ CONECTADO EL ESP32
  */
  ESP32_CONNECTED();

  /*
    INICIALIZAR EL SPI
  */
  SPI_SETUP();
  
  // /*
  //   BLE rx parameters SETUP
  // */
  // BLE_SETUP();



}

void loop() {
  // ---- SPI ----
  initializeBuffers(tx_buf_ECG, rx_buf, 3);
  received_bytes_ECG = slave.transfer(tx_buf_ECG, rx_buf, 3);

  if (received_bytes_ECG > 0) {
    spi_data_array[spi_data_index++] = rx_buf[0];
    spi_data_array[spi_data_index++] = rx_buf[1];
    spi_data_array[spi_data_index++] = rx_buf[2];

  Serial.println("Received bytes: ");
  Serial.print(rx_buf[0]);
  Serial.print(", ");
  Serial.print(rx_buf[1]);
  Serial.print(", ");
  Serial.println(rx_buf[2]);
   
  }
}
