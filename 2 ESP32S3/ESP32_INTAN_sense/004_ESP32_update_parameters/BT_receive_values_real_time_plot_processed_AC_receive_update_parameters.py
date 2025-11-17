import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
import csv
from datetime import datetime

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


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

        i = 0
        while i <= len(received_raw_buffer) - 3:
            if received_raw_buffer[i] == 0x31:
                high_word = received_raw_buffer[i + 1]
                low_word = received_raw_buffer[i + 2]
                combined_value = (high_word << 8) | low_word
                real_value = 0.195 * (combined_value - 32768)

                processed_data.append(real_value)
                i += 3
            else:
                i += 1

        # Mantener el buffer limpio (solo lo necesario)
        if i > 0:
            received_raw_buffer = received_raw_buffer[i:]

    except Exception as e:
        print(f"⚠️ Error procesando datos: {e}")

# ------------------ Funciones BLE ------------------
async def connect_ble_async(log_text):
    global ble_client, is_connected

    log_text.insert(tk.END, "🔍 Looking for BLE devices...\n")
    log_text.see(tk.END)
    devices = await BleakScanner.discover()

    esp32_address = None
    for d in devices:
        if d.name and DEVICE_NAME in d.name:
            esp32_address = d.address
            log_text.insert(tk.END, f"✅ Found {d.name} [{d.address}]\n")
            log_text.see(tk.END)
            break

    if not esp32_address:
        log_text.insert(tk.END, "❌ ESP32S3_BLE not found.\n")
        log_text.see(tk.END)
        return

    ble_client = BleakClient(esp32_address)
    try:
        await ble_client.connect(timeout=10.0)
        if ble_client.is_connected:
            is_connected = True
            log_text.insert(tk.END, "✅ Connected correctly.\n")
            log_text.see(tk.END)

            await ble_client.start_notify(CHARACTERISTIC_UUID_TX, notification_handler)
        else:
            log_text.insert(tk.END, "❌ Could not connect.\n")
            log_text.see(tk.END)
    except BleakError as e:
        log_text.insert(tk.END, f"⚠️ Error when connecting: {e}\n")
        log_text.see(tk.END)

async def send_data_async(data, log_text):
    global ble_client, is_connected
    if not is_connected or not ble_client:
        log_text.insert(tk.END, "❌ No BLE connection.\n")
        log_text.see(tk.END)
        return
    try:
        await ble_client.write_gatt_char(CHARACTERISTIC_UUID_RX, data.encode("utf-8"))
        log_text.insert(tk.END, f"📤 Sent: {data}\n")
        log_text.see(tk.END)
    except Exception as e:
        log_text.insert(tk.END, f"⚠️ Error when sending: {e}\n")
        log_text.see(tk.END)

# ------------------ Wrappers seguros para Tkinter ------------------
def run_async_task(coro):
    asyncio.run_coroutine_threadsafe(coro, async_loop)

def connect_ble(log_text):
    run_async_task(connect_ble_async(log_text))

def send_data(entries_a, entries_b, log_text):
    # Combina todos los valores de los parámetros A y B
    values_a = [entry.get().strip() for entry in entries_a]
    values_b = [entry.get().strip() for entry in entries_b]

    all_values = values_a + values_b

    # Verifica que no haya campos vacíos
    if not all(all_values):
        messagebox.showwarning("Empty values", "Please, fill in values before sending.")
        return

    data = ",".join(all_values)
    run_async_task(send_data_async(data, log_text))

# ------------------ Gráfica ------------------
def show_graph():
    if not processed_data:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.plot(processed_data, color="blue", lw=1)
    ax.set_title("Valor AC recibido")
    ax.set_xlabel("Indice de muestra")
    ax.set_ylabel("uV")
    ax.grid(True)

    # 🔹 Fijar el límite del eje Y entre 0 y 2^16
    plt.show()
    
def show_realtime_graph():
    if not processed_data:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return

    fig, ax = plt.subplots(figsize=(7, 4))
    line, = ax.plot([], [], color="blue", lw=1)
    ax.set_title("Señal en tiempo real")
    ax.set_xlabel("Índice relativo")
    ax.set_ylabel("uV")
    ax.set_ylim(-7000, 7000)
    ax.grid(True)

    window_duration = 1  # segundos
    sampling_rate = 1300  # estimación (ajústala según tu frecuencia real de envío)
    window_size = window_duration * sampling_rate

    def update(frame):
        if not processed_data:
            return line,

        # Tomar las últimas muestras dentro de la ventana temporal
        data = processed_data[-window_size:]
        x = list(range(len(data)))
        line.set_data(x, data)
        ax.set_xlim(0, len(data) if len(data) > 0 else 1)
        return line,

    ani = FuncAnimation(fig, update, interval=100)  # actualiza cada 100 ms (~10 fps)
    plt.show()

def save_to_csv(entry_id):
    if not processed_data:
        messagebox.showinfo("No data", "No data received to be shown.")
        return

    import plotly.express as px
    import pandas as pd
    import webbrowser

    # Leer ID del experimento
    experiment_id = entry_id.get().strip()
    if not experiment_id:
        experiment_id = "experiment"

    # Crear nombres de archivo con fecha, hora y ID
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename_csv = f"{experiment_id}_ECG_{timestamp}.csv"
    filename_html = f"{experiment_id}_ECG_{timestamp}.html"

    try:
        # --- Guardar CSV ---
        with open(filename_csv, mode="w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file, delimiter=';')

            # Cabecera
            writer.writerow(["Index", "ECG"])

            # Filas formateadas a coma europea
            for i, value in enumerate(processed_data):
                value_eu = f"{value:.6f}".replace(".", ",")
                writer.writerow([i, value_eu])


        # --- Crear DataFrame ---
        df = pd.DataFrame({
            "Index": list(range(len(processed_data))),
            "ECG": processed_data
        })

        # --- Crear gráfica interactiva ---
        fig = px.line(df, x="Index", y="ECG", title=f"ECG signal - {experiment_id}")
        fig.update_layout(
            xaxis_title="Index",
            yaxis_title="Amplitude",
            template="plotly_white"
        )

        # --- Guardar HTML y abrir en navegador ---
        fig.write_html(filename_html, auto_open=True)

        # --- Mostrar mensaje ---
        messagebox.showinfo(
            "Éxito",
            f"✅ Datos guardados correctamente.\n\n"
            f"📁 CSV: {filename_csv}\n"
            f"🌐 HTML: {filename_html}\n\n"
            "El gráfico interactivo se ha abierto en tu navegador."
        )

    except Exception as e:
        messagebox.showerror("Error", f"No se pudo guardar el archivo:\n{e}")



def clear_memory(log_text=None):
    global processed_data, received_raw_buffer

    processed_data.clear()
    received_raw_buffer.clear()

    if log_text:
        log_text.insert(tk.END, "🧹 Memory cleared (processed data + buffer).\n")
        log_text.see(tk.END)

# ------------------ GUI Tkinter ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3 - IKKI Sense Control")
    root.geometry("450x680")   # ventana más compacta

    tk.Label(root, text="IKKI - SENSE", font=("Arial", 16, "bold")).pack(pady=5)

    # ---------- CONEXIÓN ----------
    frame_conn = ttk.LabelFrame(root, text="Conexión BLE", padding=5)
    frame_conn.pack(padx=8, pady=8, fill="x")

    log_text = tk.Text(frame_conn, height=5, width=70)
    log_text.pack(padx=3, pady=3)

    ttk.Button(frame_conn, text="🔗 Connect BLE",
               command=lambda: connect_ble(log_text)).pack(pady=4)

    ttk.Button(root, text="💾 Save data in CSV and HTML",
               command=lambda: save_to_csv(entry_id)).pack(pady=3)

    ttk.Button(root, text="🧹 Clear memory",
               command=lambda: clear_memory(log_text)).pack(pady=3)

    # ---------- PARÁMETROS ----------
    frame_params_container = ttk.Frame(root)
    frame_params_container.pack(padx=5, pady=5, fill="both")

    frame_A = ttk.LabelFrame(frame_params_container, text="Stimulation parameters", padding=5)
    frame_A.pack(side="left", padx=5, fill="both", expand=True)

    # Usamos GRID para compactar la UI
    entries_a = []
    row_i = 0

    def add_row(label_text, widget):
        nonlocal row_i
        ttk.Label(frame_A, text=label_text).grid(row=row_i, column=0, sticky="w", padx=3, pady=1)
        widget.grid(row=row_i, column=1, sticky="e", padx=3, pady=1)
        entries_a.append(widget)
        row_i += 1

    # ID
    ttk.Label(frame_A, text="Experiment ID").grid(row=row_i, column=0, sticky="w", padx=3, pady=1)
    entry_id = ttk.Entry(frame_A, width=18)
    entry_id.grid(row=row_i, column=1, sticky="e", padx=3, pady=1)
    row_i += 1

    # Campos compactados
    add_row("ADC sampling rate (kHz)", ttk.Entry(frame_A, width=18))
    add_row("DSP cutoff frequency (Hz)", ttk.Entry(frame_A, width=18))
    add_row("Channels (max 15)", ttk.Entry(frame_A, width=18))
    add_row("DSP enabled", ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly"))
    add_row("Initial channel", ttk.Entry(frame_A, width=18))
    add_row("fc high magnitude", ttk.Entry(frame_A, width=18))
    add_row("fc high unit", ttk.Combobox(frame_A, values=["kHz", "Hz"], width=16, state="readonly"))

    allowed_fc_low = [
        "1000","500","250","200","100","75","50","30","25","20","15",
        "10","7.5","5","3","2.5","2","1.5","1","0.75","0.5","0.3",
        "0.25","0.1"
    ]

    add_row("fc low A", ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly"))
    add_row("fc low B", ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly"))
    add_row("Amplifier cutoff", ttk.Combobox(frame_A, values=["A", "B"], width=16, state="readonly"))

    # ---- Botones inferiores ----
    ttk.Button(root, text="📤 Send new data",
               command=lambda: send_data(entries_a, [], log_text)).pack(pady=7)

    ttk.Button(root, text="📊 Show complete graph",
               command=show_graph).pack(pady=5)

    ttk.Button(root, text="📈 Show real-time graph (3s)",
               command=show_realtime_graph).pack(pady=5)

    root.mainloop()

# ------------------ MAIN ------------------
if __name__ == "__main__":
    threading.Thread(target=async_loop.run_forever, daemon=True).start()
    run_gui()
