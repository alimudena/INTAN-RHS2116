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

/*

  SPI

*/

#include <ESP32SPISlave.h>
#include "helper.h"


ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 64;
static constexpr size_t QUEUE_SIZE = 8;

uint8_t tx_buf[BUFFER_SIZE] = {0xAA};
uint8_t rx_buf[BUFFER_SIZE] = {0x00};


/*

  STIMULATION CONTROL

*/


const int STIM_CONTROL = 1; // Pin para habilitar o no la estimulación
const int TIME_CONTROL = 2;  // Pin avisar del cambio de estimulación
const int NEW_PARAM_CONTROL = 8;  // Pin avisar de que hay nuevos parámetros 



// para contar el tiempo que ha pasado desde la anterior estimulación o pulso de estimulación
unsigned long previous_STIM_milis = 0;
unsigned long previous_TIME_micros = 0;


// para poner a vcc o 0 V el pin
bool pinState_STIM = LOW;
bool pinState_TIME = LOW;
bool pinState_NEW_PARAM = LOW;

// Parámetros de tiempos de estimulación
uint32_t resting_time = 15; //15 seconds
uint16_t stimulation_time = 5; //5 seconds
uint8_t number_of_stimulations = 0; //number of trains of pulses to be sent
uint8_t number_of_stimulations_done = 0;


float stimulation_on_time = 0.5; // milisegundos
float stimulation_off_time = 50.0; // milisegundos

uint16_t step_DAC = 5000; //uA
uint8_t positive_magnitude = 0.5; // current value = step_DAC*positive_magnitude
uint8_t negative_magnitude = 50.0; // milisegundos

uint8_t NUMBER_PARAMETERS_UPDATED = 8;

bool stimulation_on = false;

bool new_parameters = false;


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

    if (Comma1 != -1 && Comma2 != -1 && Comma3 != -1 && Comma4 != -1 && Comma4 != -1 && Comma5 != -1 ) {
      // Convertir cada substring a número
      number_of_stimulations = (uint8_t)value.substring(0, Comma1).toInt();
      resting_time = (uint32_t)value.substring(Comma1 + 1, Comma2).toInt();
      stimulation_time = (uint16_t)value.substring(Comma2 + 1, Comma3).toInt();

      stimulation_on_time = value.substring(Comma3 + 1, Comma4).toFloat();
      stimulation_off_time = value.substring(Comma4 + 1, Comma5).toFloat();

      step_DAC = (uint8_t)value.substring(Comma5 + 1, Comma6).toInt();
      positive_magnitude = (uint32_t)value.substring(Comma6 + 1, Comma7).toInt();
      negative_magnitude = (uint16_t)value.substring(Comma7 + 1).toInt();

      number_of_stimulations_done = 0;
      new_parameters = true;
      
    } else {
      Serial.println("Error: datos no tienen el formato correcto.");
    }
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

void STIM_CONTROL_SETUP(){
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

void NEW_PARAMETERS_SETUP(){
    // Configuración del pin de aviso de nuevos parámetros
    pinMode(NEW_PARAM_CONTROL, OUTPUT);
    digitalWrite(NEW_PARAM_CONTROL, LOW);

    Serial.println("Inicializado pin control de nuevos parámetros");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  /*
    STIMULATION CONTROL SETUP
  */
  STIM_CONTROL_SETUP();

  /*
    NEW PARAMETERS WARNING SETUP
  */
  NEW_PARAMETERS_SETUP();
  
  /*
    BLE rx parameters SETUP
  */
  BLE_SETUP();

}

void loop() {

  if (new_parameters){
    digitalWrite(NEW_PARAM_CONTROL, HIGH); // AVISAR DE QUE HAY NUEVOS PARÁMETROS ENVIADOS POR BLUETOOTH
    uint8_t tx_buf_new_parameters[BUFFER_SIZE] = {0};
    size_t index = 0;

    memcpy(&tx_buf_new_parameters[index], &number_of_stimulations, sizeof(number_of_stimulations));
    index += sizeof(number_of_stimulations);

    memcpy(&tx_buf_new_parameters[index], &resting_time, sizeof(resting_time));
    index += sizeof(resting_time);

    memcpy(&tx_buf_new_parameters[index], &stimulation_time, sizeof(stimulation_time));
    index += sizeof(stimulation_time);

    memcpy(&tx_buf_new_parameters[index], &stimulation_on_time, sizeof(stimulation_on_time));
    index += sizeof(stimulation_on_time);

    memcpy(&tx_buf_new_parameters[index], &stimulation_off_time, sizeof(stimulation_off_time));
    index += sizeof(stimulation_off_time);

    memcpy(&tx_buf_new_parameters[index], &step_DAC, sizeof(step_DAC));
    index += sizeof(step_DAC);

    memcpy(&tx_buf_new_parameters[index], &positive_magnitude, sizeof(positive_magnitude));
    index += sizeof(positive_magnitude);

    memcpy(&tx_buf_new_parameters[index], &negative_magnitude, sizeof(negative_magnitude));
    index += sizeof(negative_magnitude);

    Serial.print("sending values: ");
    Serial.println(tx_buf_new_parameters[0]);
    
    initializeBuffers(tx_buf_new_parameters, rx_buf, BUFFER_SIZE);
    const size_t received_bytes = slave.transfer(tx_buf_new_parameters, rx_buf, BUFFER_SIZE);

    digitalWrite(NEW_PARAM_CONTROL, LOW); // AVISAR DE QUE HAY NUEVOS PARÁMETROS ENVIADOS POR BLUETOOTH
    new_parameters = false;
  }
  // ---- SPI for ECG----
  initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE);
  const size_t received_bytes = slave.transfer(tx_buf, rx_buf, BUFFER_SIZE);

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

    // ---- CONTROL DE LA ESTIMULACIÓN ----
    unsigned long currentMillis = millis();
    unsigned long currentMicros = millis();


    if(number_of_stimulations_done < number_of_stimulations){
        // Encendido del STIM ENABLE tras resting_time segundos
        if (pinState_STIM == LOW && currentMillis - previous_STIM_milis >= resting_time*1000) {
            digitalWrite(STIM_CONTROL, HIGH);
            pinState_STIM = HIGH;
            previous_STIM_milis = currentMillis;
            // Serial.println("→ STIM puesto en HIGH");
            
        }

        // Apagado del STIM ENABLE tras stimulation_time segundos
        if (pinState_STIM == HIGH && currentMillis - previous_STIM_milis >= stimulation_time*1000) {
            digitalWrite(STIM_CONTROL, LOW);
            pinState_STIM = LOW;
            previous_STIM_milis = currentMillis;
            // Serial.println("→ STIM vuelto a LOW");
            Serial.print("Estimulación número: ");
            Serial.println(number_of_stimulations_done);
            number_of_stimulations_done++;

        }


        if(!stimulation_on){// El pulso está en positivo o en negativo en la estimulación
          // Envio de un pulso para encender la estimulación
          if (pinState_STIM == HIGH && pinState_TIME == LOW && currentMicros - previous_TIME_micros >= stimulation_on_time) {
              digitalWrite(TIME_CONTROL, HIGH);
              pinState_TIME = HIGH;
              previous_TIME_micros = currentMicros;
              // Serial.println("→ TIME puesto en HIGH");
              
          }

          // Si el pin está en HIGH, esperamos 3 milisegundos para apagarlo
          if (pinState_STIM == HIGH && pinState_TIME == HIGH && currentMicros - previous_TIME_micros >= 3) {
              digitalWrite(TIME_CONTROL, LOW);
              pinState_TIME = LOW;
              previous_TIME_micros = currentMicros;
              // Serial.println("→ TIME vuelto a LOW");
              stimulation_on = true;
          }


        }else{// El pulso está pausado en la estimulación
          // Envio de un pulso para apagar la estimulación
          if (pinState_STIM == HIGH && pinState_TIME == LOW && currentMicros - previous_TIME_micros >= stimulation_off_time) {
              digitalWrite(TIME_CONTROL, HIGH);
              pinState_TIME = HIGH;
              previous_TIME_micros = currentMicros;
              // Serial.println("→ TIME puesto en HIGH");
              
          }

          // Si el pin está en HIGH, esperamos 3 milisegundos para apagarlo
          if (pinState_STIM == HIGH && pinState_TIME == HIGH && currentMicros - previous_TIME_micros >= 3) {
              digitalWrite(TIME_CONTROL, LOW);
              pinState_TIME = LOW;
              previous_TIME_micros = currentMicros;
              // Serial.println("→ TIME vuelto a LOW");
              stimulation_on = false;
          }
        }

    }else if(number_of_stimulations_done == number_of_stimulations){
      Serial.println("Máximo estimulaciones alcanzado.");
      number_of_stimulations_done++;
      stimulation_on = false;
    }


    // Pequeña pausa opcional para evitar saturar el loop
    delay(1);
 
}
