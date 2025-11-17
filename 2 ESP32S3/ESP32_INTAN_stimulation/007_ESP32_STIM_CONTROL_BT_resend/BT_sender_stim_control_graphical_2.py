import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt

# ------------------ BLE UUIDs ------------------
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Enviar
CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Recibir
DEVICE_NAME = "ESP32S3_BLE"

# ------------------ Variables globales ------------------
ble_client = None
is_connected = False
received_raw_buffer = []   # bytes crudos recibidos
processed_data = []        # valores combinados high_word + low_word
async_loop = asyncio.new_event_loop()

# ------------------ BLE callback ------------------
def notification_handler(sender, data):
    """
    Recibe bytes crudos del ESP32-S3 y procesa grupos del tipo:
    [0x30, high_word, low_word]
    """
    global received_raw_buffer, processed_data
    try:
        values = list(data)
        received_raw_buffer.extend(values)
        print(f"📥 Recibido: {values}")

        # Procesar el buffer
        i = 0
        while i <= len(received_raw_buffer) - 3:
            if received_raw_buffer[i] == 0x30:
                high_word = received_raw_buffer[i + 1]
                low_word = received_raw_buffer[i + 2]
                combined_value = (high_word << 8) | low_word
                processed_data.append(combined_value)
                i += 3
            else:
                # Si no hay marca 0x30, descartar byte y continuar
                i += 1

        # Mantener el buffer limpio (solo lo necesario)
        if i > 0:
            received_raw_buffer = received_raw_buffer[i:]

    except Exception as e:
        print(f"⚠️ Error procesando datos: {e}")


# ------------------ Funciones BLE ------------------
async def connect_ble_async(log_text):
    global ble_client, is_connected

    log_text.insert(tk.END, "🔍 Buscando dispositivos BLE...\n")
    log_text.see(tk.END)
    devices = await BleakScanner.discover()

    esp32_address = None
    for d in devices:
        if d.name and DEVICE_NAME in d.name:
            esp32_address = d.address
            log_text.insert(tk.END, f"✅ Encontrado {d.name} [{d.address}]\n")
            log_text.see(tk.END)
            break

    if not esp32_address:
        log_text.insert(tk.END, "❌ ESP32S3_BLE no encontrado.\n")
        log_text.see(tk.END)
        return

    ble_client = BleakClient(esp32_address)
    try:
        await ble_client.connect(timeout=10.0)
        if ble_client.is_connected:
            is_connected = True
            log_text.insert(tk.END, "✅ Conectado correctamente.\n")
            log_text.see(tk.END)
            try:
                await ble_client.exchange_mtu(517)
                log_text.insert(tk.END, "✅ MTU negociado a 517 bytes\n")
            except:
                log_text.insert(tk.END, "⚠️ No se pudo negociar MTU grande\n")
            log_text.see(tk.END)

            await ble_client.start_notify(CHARACTERISTIC_UUID_TX, notification_handler)
        else:
            log_text.insert(tk.END, "❌ No se pudo conectar.\n")
            log_text.see(tk.END)
    except BleakError as e:
        log_text.insert(tk.END, f"⚠️ Error al conectar: {e}\n")
        log_text.see(tk.END)

async def send_data_async(data, log_text):
    global ble_client, is_connected
    if not is_connected or not ble_client:
        log_text.insert(tk.END, "❌ No hay conexión BLE.\n")
        log_text.see(tk.END)
        return
    try:
        await ble_client.write_gatt_char(CHARACTERISTIC_UUID_RX, data.encode("utf-8"))
        log_text.insert(tk.END, f"📤 Enviado: {data}\n")
        log_text.see(tk.END)
    except Exception as e:
        log_text.insert(tk.END, f"⚠️ Error al enviar: {e}\n")
        log_text.see(tk.END)

# ------------------ Wrappers seguros para Tkinter ------------------
def run_async_task(coro):
    asyncio.run_coroutine_threadsafe(coro, async_loop)

def connect_ble(log_text):
    run_async_task(connect_ble_async(log_text))

def send_data(entries, log_text):
    values = [entry.get().strip() for entry in entries]
    if not all(values):
        messagebox.showwarning("Campos vacíos", "Por favor, completa todos los valores antes de enviar.")
        return
    data = ",".join(values)
    run_async_task(send_data_async(data, log_text))

# ------------------ Gráfica ------------------
def show_graph():
    if not processed_data:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.plot(processed_data, color="blue", lw=1)
    ax.set_title("Datos combinados (high_word << 8 | low_word)")
    ax.set_xlabel("Índice de muestra")
    ax.set_ylabel("Valor combinado (16 bits)")
    ax.grid(True)
    plt.show()

# ------------------ GUI Tkinter ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3")
    root.geometry("700x600")

    tk.Label(root, text="Control y visualización BLE - ESP32S3", font=("Arial", 16, "bold")).pack(pady=10)

    frame_conn = ttk.LabelFrame(root, text="Conexión BLE", padding=10)
    frame_conn.pack(padx=10, pady=10, fill="x")

    log_text = tk.Text(frame_conn, height=6, width=80)
    log_text.pack(padx=5, pady=5)

    ttk.Button(
        frame_conn, text="🔗 Conectar BLE", 
        command=lambda: connect_ble(log_text)
    ).pack(pady=5)

    frame_send = ttk.LabelFrame(root, text="Parámetros de estimulación", padding=10)
    frame_send.pack(padx=10, pady=10, fill="x")

    labels = [
        "number_of_stimulations",
        "resting_time (s)",
        "stimulation_time (s)",
        "stimulation_on_time (μs)",
        "stimulation_off_time (μs)"
    ]
    entries = []
    for lbl in labels:
        row = ttk.Frame(frame_send)
        row.pack(fill="x", pady=2)
        ttk.Label(row, text=lbl, width=30).pack(side="left")
        entry = ttk.Entry(row, width=20)
        entry.pack(side="right", padx=5)
        entries.append(entry)

    ttk.Button(
        frame_send, text="📤 Enviar datos",
        command=lambda: send_data(entries, log_text)
    ).pack(pady=10)

    ttk.Button(root, text="📊 Mostrar gráfico completo", command=show_graph).pack(pady=10)

    root.mainloop()

# ------------------ MAIN ------------------
if __name__ == "__main__":
    threading.Thread(target=async_loop.run_forever, daemon=True).start()
    run_gui()
