import asyncio
from bleak import BleakClient, BleakScanner, BleakError

SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
DEVICE_NAME = "ESP32S3_BLE"

async def main():
    print("🔍 Buscando dispositivos BLE cercanos...")
    devices = await BleakScanner.discover()
    esp32_address = None
    for d in devices:
        if d.name and DEVICE_NAME in d.name:
            esp32_address = d.address
            print(f"✅ Encontrado: {d.name} [{d.address}]")
            break

    if not esp32_address:
        print("❌ No se encontró el ESP32S3_BLE.")
        return

    client = BleakClient(esp32_address)
    connected = False

    # Intentos de conexión
    for attempt in range(1, 4):
        print(f"🔗 Intentando conectar (intento {attempt}/3)...")
        try:
            await client.connect(timeout=10.0)
            if client.is_connected:
                connected = True
                print("✅ Conectado correctamente.")
                break
        except BleakError as e:
            print(f"⚠️ Conexión fallida: {e}")
        await asyncio.sleep(2)

    if not connected:
        print("❌ No se pudo conectar al ESP32S3 después de varios intentos.")
        return

    # Enviar datos
    try:
        while True:
            number_of_stimulations = input("Introduce el valor del number_of_stimulations: ")
            resting_time = input("Introduce el valor del resting_time en segundos: ")
            stimulation_time = input("Introduce el valor del stimulation_time en segundos: ")

            stimulation_on_time = input("Introduce el valor del stimulation_on_time en microsegundos: ")
            stimulation_off_time = input("Introduce el valor del stimulation_off_time en microsegundos: ")


            data_to_send = f"{number_of_stimulations},{resting_time},{stimulation_time},{stimulation_on_time},{stimulation_off_time}"
            await client.write_gatt_char(CHARACTERISTIC_UUID, data_to_send.encode('utf-8'))
            print(f"📤 Enviado: {data_to_send}")

            again = input("¿Enviar otro? (s/n): ").strip().lower()
            if again != "s":
                break
    finally:
        await client.disconnect()
        print("🔚 Conexión cerrada.")

if __name__ == "__main__":
    asyncio.run(main())
