
/*

  SPI

*/

#include <ESP32SPISlave.h>
#include "helper.h"


ESP32SPISlave slave;



static constexpr size_t SPI_data_send = 6;

static constexpr size_t BUFFER_SIZE = 64;
static constexpr size_t QUEUE_SIZE = 8;

uint8_t tx_buf[BUFFER_SIZE] = {0xAA};
uint8_t rx_buf[BUFFER_SIZE] = {0x00};


uint8_t spi_rx_buf[SPI_data_send] = {0};
uint8_t spi_tx_buf[SPI_data_send] = {0};

/*

  STIMULATION CONTROL

*/


const int WANT_TO_SEND_PIN = 11; // Pin para pedir mandar los parámetros por SPI
const int ACK_ALLOW_SEND_PIN = 12;  // Pin para recibir el ACK del enío por SPI
const int ESP32_CONNECTED_PIN = 13;  // Pin para avisar de si está conectado el ESP32 o no 



// Parámetros de tiempos de estimulación
uint8_t resting_time = 15; //15 seconds
uint8_t stimulation_time = 5; //5 seconds
uint8_t number_of_stimulations = 0; //number of trains of pulses to be sent
uint8_t number_of_stimulations_done = 0;


uint16_t stimulation_on_time = 0; // milisegundos
uint8_t stimulation_off_time = 0; // milisegundos

bool received_ack = false;
bool sent = false;
bool parameters_updated = false;
size_t received_bytes;
size_t received_bytes_counter;


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

/*

  ALMACENAMIENTO DE DATOS A TRAVÉS DE SPI

*/
static constexpr size_t SPI_DATA_ARRAY_SIZE = 30;  // número de muestras a almacenar
uint8_t spi_data_array[SPI_DATA_ARRAY_SIZE];
size_t spi_data_index = 0; // posición actual del array



void ON_SEND_PIN(){
  digitalWrite(WANT_TO_SEND_PIN, HIGH);
}

void OFF_SEND_PIN(){
  digitalWrite(WANT_TO_SEND_PIN, LOW);
}


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

      if (Comma1 != -1 && Comma2 != -1 && Comma3 != -1 && Comma4 != -1 ) {
        // Convertir cada substring a número
        number_of_stimulations = (uint8_t)value.substring(0, Comma1).toInt();
        resting_time = (uint32_t)value.substring(Comma1 + 1, Comma2).toInt();
        stimulation_time = (uint16_t)value.substring(Comma2 + 1, Comma3).toInt();

        stimulation_on_time = value.substring(Comma3 + 1, Comma4).toInt();
        stimulation_off_time = value.substring(Comma4 + 1).toInt();

        number_of_stimulations_done = 0;

        Serial.print("number_of_stimulations: ");
        Serial.println(number_of_stimulations);
        Serial.print("resting_time: ");
        Serial.println(resting_time);
        Serial.print("stimulation_time: ");
        Serial.println(stimulation_time);

        Serial.print("stimulation_on_time [us]: ");
        Serial.println(stimulation_on_time, 3);
        Serial.print("stimulation_off_time [us]: ");
        Serial.println(stimulation_off_time, 3);


      } else {
        Serial.println("Error: datos no tienen el formato correcto.");
      }
      parameters_updated = true;      
    } 
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

void NEW_PARAMETERS_SETUP(){
    // Configuración del pin aviso de que existen nuevos parámetros
    pinMode(WANT_TO_SEND_PIN, OUTPUT);
    digitalWrite(WANT_TO_SEND_PIN, LOW);

    Serial.println("Inicializado pin para avisar de que existen nuevos parámetros.");

    // Configuración del pin de recepción del ACK por la existencia de nuevos parámetros
    pinMode(ACK_ALLOW_SEND_PIN, INPUT);

    Serial.println("Inicializado pin para recibir el ACK por parte del maestro");


    // Configuración del esclavo SPI
    slave.setDataMode(SPI_MODE0);
    slave.setQueueSize(SPI_data_send);
    slave.begin(HSPI, 16, 17, 18, 15);//uint8_t spi_bus, int sck, int miso, int mosi, int ss

    Serial.println("Inicializado SPI Slave.");
}

void ESP32_CONNECTED(){
    // Configuración del pin de aviso de nuevos parámetros
    pinMode(ESP32_CONNECTED_PIN, OUTPUT);
    digitalWrite(ESP32_CONNECTED_PIN, HIGH);

    Serial.println("Inicializado pin control de nuevos parámetros");
}

bool ACK_RECEIVED(){
  return digitalRead(ACK_ALLOW_SEND_PIN);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  /*
    NEW PARAMETERS SETUP
  */
  NEW_PARAMETERS_SETUP();

  /*
    AVISAR DE QUE ESTÁ CONECTADO EL ESP32
  */
  ESP32_CONNECTED();
  
  /*
    BLE rx parameters SETUP
  */
  BLE_SETUP();

}

void loop() {
  // ---- SPI ----
  initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
  received_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

  if (received_bytes > 0) {
    spi_data_array[spi_data_index++] = rx_buf[0];
    spi_data_array[spi_data_index++] = rx_buf[1];
    spi_data_array[spi_data_index++] = rx_buf[2];

    // Si el array se llena → enviar por BLE
    if (spi_data_index >= SPI_DATA_ARRAY_SIZE) {
      String dataString = "";
      for (size_t i = 0; i < SPI_DATA_ARRAY_SIZE; i++) {
        dataString += String(spi_data_array[i]);
        if (i < SPI_DATA_ARRAY_SIZE - 1) dataString += ",";
      }

      // Serial.println("Array SPI lleno. Enviando datos por BLE...");
      if (pCharacteristicTX != nullptr) {
        pCharacteristicTX->setValue(spi_data_array, SPI_DATA_ARRAY_SIZE);
        pCharacteristicTX->notify();
      }

      // Reiniciar el índice del buffer
      spi_data_index = 0;
    }
  }

  //   // Pequeña pausa opcional para evitar saturar el loop
  //   delay(1);
  if (parameters_updated){
    uint8_t high_byte_stimulation_on = (stimulation_on_time >> 8) & 0xFF;
    uint8_t low_byte_stimulation_on  = stimulation_on_time & 0xFF;

    spi_tx_buf[0] = number_of_stimulations;  // number_of_stimulations
    spi_tx_buf[1] = resting_time;  // Código de comando
    spi_tx_buf[2] = stimulation_time;  // Código de comando
    spi_tx_buf[3] = high_byte_stimulation_on;  // Código de comando
    spi_tx_buf[4] = low_byte_stimulation_on;  // Código de comando
    spi_tx_buf[5] = stimulation_off_time;  // Código de comando

    ON_SEND_PIN(); 
    received_bytes = slave.transfer(spi_tx_buf, spi_rx_buf, SPI_data_send);
  }
  received_ack = ACK_RECEIVED();
  if(received_ack & !sent){
    OFF_SEND_PIN();
    sent = true;
    Serial.println("received ACK");
    // Comprobación del resultado
    Serial.print("Transacción SPI completada. Bytes recibidos: ");
    Serial.println(received_bytes);

    Serial.print("Transacción número: ");
    Serial.println(received_bytes_counter++);


    Serial.print("Datos recibidos del maestro: ");
    for (size_t i = 0; i < received_bytes; ++i) {
      Serial.printf("%02X ", spi_rx_buf[i]);
    }
    Serial.println();
    Serial.print("Datos enviados al maestro: ");
    for (size_t i = 0; i < received_bytes; ++i) {
      Serial.printf("%02X ", spi_tx_buf[i]);
    }
    Serial.println();
    parameters_updated = false;
    
  }else if(!received_ack){ // este reloj va mucho más rápido, necesitamos tener un control sobre el flag del maestro para poder enviar otros parámetros nuevos
    sent = false;
  }
  

 
}
