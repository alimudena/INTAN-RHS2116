#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UUID genérico de servicio UART BLE
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // UUID genérico de característica RX

BLECharacteristic *pCharacteristic;

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.print("Datos recibidos: ");
      Serial.println(value.c_str());

      // Separar los dos datos si vienen separados por coma
      String data = String(value.c_str());
      int commaIndex = data.indexOf(',');
      if (commaIndex != -1) {
        String dato1 = data.substring(0, commaIndex);
        String dato2 = data.substring(commaIndex + 1);
        Serial.print("Dato 1: ");
        Serial.println(dato1);
        Serial.print("Dato 2: ");
        Serial.println(dato2);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando BLE en ESP32-S3...");

  BLEDevice::init("ESP32S3_BLE"); // Nombre visible del dispositivo BLE
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  // Inicia la publicidad (para que otros dispositivos lo vean)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("Esperando conexión BLE...");
}

void loop() {
 
}
