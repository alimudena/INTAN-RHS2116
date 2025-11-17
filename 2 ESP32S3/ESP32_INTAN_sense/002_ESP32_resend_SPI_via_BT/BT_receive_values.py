import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
import csv
from datetime import datetime

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
        # print(f"📥 Recibido: {values}")

        # Procesar el buffer
        i = 0
        while i <= len(received_raw_buffer) - 3:
            if received_raw_buffer[i] == 0x31:
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
    ax.set_title("Datos combinados (high_word << 8 | low_word)")
    ax.set_xlabel("Indice de muestra")
    ax.set_ylabel("Valor combinado (16 bits)")
    ax.grid(True)

    # 🔹 Fijar el límite del eje Y entre 0 y 2^16
    ax.set_ylim(0, 2**16)
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
        with open(filename_csv, mode="w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(["Index", "ECG"])
            for i, value in enumerate(processed_data):
                writer.writerow([i, value])

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

# ------------------ GUI Tkinter ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3 - IKKI Stimulation Control")
    root.geometry("950x800")

    tk.Label(root, text="IKKI - STIMULATION", font=("Arial", 16, "bold")).pack(pady=10)

    # ---------- CONEXIÓN ----------
    frame_conn = ttk.LabelFrame(root, text="Conexión BLE", padding=10)
    frame_conn.pack(padx=10, pady=10, fill="x")

    log_text = tk.Text(frame_conn, height=6, width=100)
    log_text.pack(padx=5, pady=5)

    ttk.Button(
        frame_conn, text="🔗 Connect BLE",
        command=lambda: connect_ble(log_text)
    ).pack(pady=5)

    ttk.Button(root, text="💾 Save data in CSV and HTML", command=lambda: save_to_csv(entry_id)).pack(pady=5)

    # ---------- CONTENEDOR DE PARÁMETROS ----------
    frame_params_container = ttk.Frame(root)
    frame_params_container.pack(padx=10, pady=10, fill="x")

    # ================== PARÁMETROS A ==================
    frame_A = ttk.LabelFrame(frame_params_container, text="Stimulation time parameters", padding=10)
    frame_A.pack(side="left", padx=10, fill="both", expand=True)

    frame_id = ttk.Frame(frame_A)
    frame_id.pack(fill="x", pady=5)
    ttk.Label(frame_id, text="Experiment ID", width=30).pack(side="left")
    entry_id = ttk.Entry(frame_id, width=20)
    entry_id.pack(side="right", padx=5)

    labels_a = [
        "Number of stimulations",
        "Resting time (mins)",
        "Stimulation time (s)",
        "Stimulation on time (μs)",
        "Stimulation off time (ms)"
    ]
    entries_a = []
    for lbl in labels_a:
        row = ttk.Frame(frame_A)
        row.pack(fill="x", pady=2)
        ttk.Label(row, text=lbl, width=30).pack(side="left")
        entry = ttk.Entry(row, width=20)
        entry.pack(side="right", padx=5)
        entries_a.append(entry)

    # ================== PARÁMETROS B ==================
    frame_B = ttk.LabelFrame(frame_params_container, text="Current parameters", padding=10)
    frame_B.pack(side="right", padx=10, fill="both", expand=True)

    # Validación para magnitudes (0–255)
    def validate_magnitude(value):
        if not value:
            return True
        if not value.isdigit():
            return False
        num = int(value)
        return 0 <= num < 256

    vcmd = (root.register(validate_magnitude), "%P")

    # Positive current magnitude
    row_pos_mag = ttk.Frame(frame_B)
    row_pos_mag.pack(fill="x", pady=2)
    ttk.Label(row_pos_mag, text="Positive current magnitude (<256)", width=35).pack(side="left")
    entry_pos_mag = ttk.Entry(row_pos_mag, width=20, validate="key", validatecommand=vcmd)
    entry_pos_mag.pack(side="right", padx=5)

    # Negative current magnitude
    row_neg_mag = ttk.Frame(frame_B)
    row_neg_mag.pack(fill="x", pady=2)
    ttk.Label(row_neg_mag, text="Negative current magnitude (<256)", width=35).pack(side="left")
    entry_neg_mag = ttk.Entry(row_neg_mag, width=20, validate="key", validatecommand=vcmd)
    entry_neg_mag.pack(side="right", padx=5)

    # Current step (único dropdown)
    row_step = ttk.Frame(frame_B)
    row_step.pack(fill="x", pady=2)
    ttk.Label(row_step, text="Current step (nA)", width=35).pack(side="left")
    step_var = tk.StringVar()
    step_dropdown = ttk.Combobox(row_step, textvariable=step_var, state="readonly", width=18)
    step_dropdown["values"] = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000]
    step_dropdown.current(0)
    step_dropdown.pack(side="right", padx=5)

    # ---------- SUBSECCIÓN DE RESULTADOS ----------
    subframe_result = ttk.LabelFrame(frame_B, text="Current calculation", padding=10)
    subframe_result.pack(fill="x", pady=10)

    # Positive current value
    ttk.Label(subframe_result, text="Positive current value (nA):", width=35).grid(row=0, column=0, padx=5, pady=2, sticky="w")
    pos_result_var = tk.StringVar(value="0")
    entry_pos_result = ttk.Entry(subframe_result, textvariable=pos_result_var, width=20, state="readonly")
    entry_pos_result.grid(row=0, column=1, padx=5, pady=2)

    # Negative current value
    ttk.Label(subframe_result, text="Negative current value (nA):", width=35).grid(row=1, column=0, padx=5, pady=2, sticky="w")
    neg_result_var = tk.StringVar(value="0")
    entry_neg_result = ttk.Entry(subframe_result, textvariable=neg_result_var, width=20, state="readonly")
    entry_neg_result.grid(row=1, column=1, padx=5, pady=2)

    # ---------- Cálculo automático ----------
    def update_results(*args):
        try:
            pos_mag = int(entry_pos_mag.get()) if entry_pos_mag.get().isdigit() else 0
            neg_mag = int(entry_neg_mag.get()) if entry_neg_mag.get().isdigit() else 0
            step = int(step_var.get()) if step_var.get().isdigit() else 0
            pos_result_var.set(str(pos_mag * step))
            neg_result_var.set(str(neg_mag * step))
        except Exception:
            pos_result_var.set("Error")
            neg_result_var.set("Error")

    entry_pos_mag.bind("<KeyRelease>", lambda e: update_results())
    entry_neg_mag.bind("<KeyRelease>", lambda e: update_results())
    step_var.trace_add("write", lambda *a: update_results())

    # Entradas B para envío
    entries_b = [entry_pos_mag, entry_neg_mag, step_var]

    # ---------- BOTÓN ÚNICO DE ENVÍO ----------
    ttk.Button(
        root, text="📤 Send new data",
        command=lambda: send_data(entries_a, entries_b, log_text)
    ).pack(pady=10)

    ttk.Button(root, text="📊 Show complete graph", command=show_graph).pack(pady=10)

    root.mainloop()

# ------------------ MAIN ------------------
if __name__ == "__main__":
    threading.Thread(target=async_loop.run_forever, daemon=True).start()
    run_gui()
