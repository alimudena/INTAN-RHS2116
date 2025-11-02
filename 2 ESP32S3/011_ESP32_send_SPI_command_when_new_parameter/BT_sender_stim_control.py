import asyncio
from bleak import BleakClient, BleakScanner, BleakError
import matplotlib.pyplot as plt
from collections import deque
import threading

# ------------------ BLE UUIDs ------------------
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Para enviar comandos
CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Para recibir datos
DEVICE_NAME = "ESP32S3_BLE"

# ------------------ Variables globales ------------------
data_buffer = deque(maxlen=200)  # Almacena las últimas muestras recibidas
new_data_event = threading.Event()  # Señal para actualizar la gráfica

# ------------------ Función de callback BLE ------------------
def notification_handler(sender, data):
    """Muestra y guarda los datos recibidos del ESP32S3."""
    try:
        decoded = data.decode("utf-8").strip()
        print(f"📥 Recibido ({len(decoded)} bytes): {decoded}")
        
        # Parsear datos CSV a enteros (si vienen en formato "12,45,78,...")
        values = [int(v) for v in decoded.split(",") if v.isdigit()]
        if values:
            data_buffer.extend(values)
            new_data_event.set()  # Indica a la gráfica que hay datos nuevos

        # Guardar en CSV (opcional)
        with open("spi_data_log.csv", "a") as f:
            f.write(decoded + "\n")

    except Exception as e:
        print(f"⚠️ Error procesando datos: {e}")

# ------------------ Hilo para mostrar la gráfica ------------------
def start_plot_thread():
    plt.ion()
    fig, ax = plt.subplots()
    line, = ax.plot([], [], lw=2)
    ax.set_title("Datos recibidos por BLE (SPI del ESP32S3)")
    ax.set_xlabel("Muestra")
    ax.set_ylabel("Valor SPI")
    ax.grid(True)

    while True:
        new_data_event.wait()  # Espera hasta que haya datos nuevos
        new_data_event.clear()
        if len(data_buffer) > 0:
            ydata = list(data_buffer)
            xdata = list(range(len(ydata)))
            line.set_xdata(xdata)
            line.set_ydata(ydata)
            ax.relim()
            ax.autoscale_view()
            plt.pause(0.05)  # Actualiza la gráfica en tiempo real

# ------------------ Función principal BLE ------------------
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

    # Inicia notificaciones
    print("📡 Suscribiéndose a notificaciones BLE...")
    await client.start_notify(CHARACTERISTIC_UUID_TX, notification_handler)
    print("✅ Notificaciones activadas. Recibiendo datos continuamente...")

    # Inicia el hilo de la gráfica
    threading.Thread(target=start_plot_thread, daemon=True).start()

    # Bucle infinito para mantener la conexión viva
    try:
        while True:
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        print("🧩 Interrupción manual detectada. Cerrando conexión...")
    finally:
        await client.stop_notify(CHARACTERISTIC_UUID_TX)
        await client.disconnect()
        print("🔚 Conexión cerrada correctamente.")

# ------------------ MAIN ------------------
if __name__ == "__main__":
    asyncio.run(main())
