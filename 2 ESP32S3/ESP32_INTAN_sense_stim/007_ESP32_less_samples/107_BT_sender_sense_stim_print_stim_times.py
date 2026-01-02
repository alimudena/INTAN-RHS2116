import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
import csv
from datetime import datetime
import time

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
received_raw_buffer = []

processed_ch1 = []   # Canal 1 (id 0xA0)
processed_ch2 = []   # Canal 2 (id 0x00)

sample_counter_ch1 = 0
sample_counter_ch2 = 0


stim_events_us = []      # Timestamps en microsegundos
stim_state = 0           # 0=OFF, 1=ON
stim_marks_idx = []      # índices de muestra donde ocurre cambio ON/OFF



# Intervalos de estimulación por índice (cerrados-abiertos): [on_idx, off_idx)
stim_intervals = []          # lista de tuplas (on_idx, off_idx)
pending_on_idx = None        # índice del comienzo actual (si estimulación está activa)

# (Opcional) Timestamps para hover/trazabilidad
stim_intervals_ts = []       # lista de tuplas (on_ts, off_ts) emparejadas
pending_on_ts = None         # timestamp del ON actual (si activo)

current_stim_state = 0       # 0=OFF, 1=ON
start_time = time.time()


# ------------------ BLE callback ------------------
def notification_handler(sender, data):
    global received_raw_buffer
    global processed_ch1, processed_ch2
    global sample_counter_ch1, sample_counter_ch2
    global stim_events_us, stim_state, stim_marks_idx

    # -------- Eventos de estimulación --------
    try:
        text = data.decode("utf-8").strip()

        # Timestamp en microsegundos
        ts_us = time.time_ns() // 1000  

        if text == "STIM_ON":
            if stim_state == 0:
                stim_state = 1
                stim_events_us.append(("ON", ts_us))
                stim_marks_idx.append(("ON", max(len(processed_ch1), len(processed_ch2))))
                print(f"⚡ STIM ON — {ts_us} µs")
            return

        if text == "STIM_OFF":
            if stim_state == 1:
                stim_state = 0
                stim_events_us.append(("OFF", ts_us))
                stim_marks_idx.append(("OFF", max(len(processed_ch1), len(processed_ch2))))
                print(f"⚡ STIM OFF — {ts_us} µs")
            return

    except:
        pass   # No es texto → seguimos con datos de ECG

    # -------- Decodificación ECG --------
    try:
        values = list(data)
        received_raw_buffer.extend(values)

        i = 0
        while i <= len(received_raw_buffer) - 3:
            header = received_raw_buffer[i]

            # ---- Canal 1 (0x31) ----
            if header == 0x31:
                high_word = received_raw_buffer[i+1]
                low_word  = received_raw_buffer[i+2]
                combined_value = (high_word << 8) | low_word
                real_value = 0.195 * (combined_value - 32768)
                processed_ch1.append(real_value)
                sample_counter_ch1 += 1
                i += 3
                continue

            # ---- Canal 2 (0x71) ----
            elif header == 0x71:
                high_word = received_raw_buffer[i+1]
                low_word  = received_raw_buffer[i+2]
                combined_value = (high_word << 8) | low_word
                real_value = 0.195 * (combined_value - 32768)
                processed_ch2.append(real_value)
                sample_counter_ch2 += 1
                i += 3
                continue

            else:
                i += 1

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


def send_data(entries_a, entries_b, entries_c, log_text):
    # Combina todos los valores de los parámetros A y B
    values_a = [entry.get().strip() for entry in entries_a]
    values_b = [entry.get().strip() for entry in entries_b]
    values_c = [entry.get().strip() for entry in entries_c]


    all_values = values_a + values_b + values_c

    # Verifica que no haya campos vacíos
    if not all(all_values):
        messagebox.showwarning("Empty values", "Please, fill in values before sending.")
        return

    data = ",".join(all_values)
    run_async_task(send_data_async(data, log_text))

# ------------------ Gráfica ------------------
def show_graph():
    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return

    fig, ax = plt.subplots(figsize=(10, 5))

    if processed_ch1:
        ax.plot(processed_ch1, label="Canal 1 (0xA0)", lw=1)

    if processed_ch2:
        ax.plot(processed_ch2, label="Canal 2 (0x00)", lw=1)

    ax.set_title("Señales recibidas — Canal 1 y Canal 2")
    ax.set_xlabel("Índice de muestra")
    ax.set_ylabel("uV")
    ax.grid(True)
    ax.legend()
    
    # Dibujar marcadores de estimulación
    for event, idx in stim_marks_idx:
        if event == "ON":
            ax.axvline(idx, color="green", linestyle="--", alpha=0.5, label="STIM ON")
        else:
            ax.axvline(idx, color="red", linestyle="--", alpha=0.5, label="STIM OFF")

    plt.show()

from matplotlib.animation import FuncAnimation


def show_realtime_graph():
    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return

    fig, ax = plt.subplots(figsize=(10, 5))

    line1, = ax.plot([], [], label="Canal 1", lw=1)
    line2, = ax.plot([], [], label="Canal 2", lw=1)

    ax.set_title("Señales en tiempo real")
    ax.set_xlabel("Índice")
    ax.set_ylabel("uV")
    ax.set_ylim(-7000, 7000)
    ax.grid(True)
    ax.legend()

    window_samples = 2000

    def update(frame):
        if processed_ch1:
            data1 = processed_ch1[-window_samples:]
            line1.set_data(range(len(data1)), data1)

        if processed_ch2:
            data2 = processed_ch2[-window_samples:]
            line2.set_data(range(len(data2)), data2)

        max_len = max(len(processed_ch1), len(processed_ch2), 1)
        ax.set_xlim(0, min(window_samples, max_len))

        # Marcas de estimulación dentro del rango
        for event, idx in stim_marks_idx:
            if idx >= max_len - window_samples:
                x = idx - (max_len - window_samples)
                color = "green" if event == "ON" else "red"
                ax.axvline(x, color=color, linestyle="--", alpha=0.5)


        return line1, line2

    anim = FuncAnimation(fig, update, interval=100, cache_frame_data=False)  # 👈 aquí

    plt.show()


def save_to_csv(entry_id):
    global processed_ch1, processed_ch2
    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("No data", "No data received to be shown.")
        return

    import plotly.express as px
    import plotly.graph_objects as go
    import pandas as pd

    experiment_id = entry_id.get().strip() or "experiment"
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename_csv = f"{experiment_id}_ECG_{timestamp}.csv"
    filename_html = f"{experiment_id}_ECG_{timestamp}.html"

    try:
        N = max(len(processed_ch1), len(processed_ch2))

        # Estado por muestra: 0 u 1
        stim_state = [0] * N
        for (on_idx, off_idx) in stim_intervals:
            a = max(0, on_idx)
            b = min(N, off_idx)
            for i in range(a, b):
                stim_state[i] = 1
        # Intervalo en curso
        if current_stim_state == 1 and pending_on_idx is not None:
            a = max(0, pending_on_idx)
            for i in range(a, N):
                stim_state[i] = 1

        # --- Guardar CSV ---
        with open(filename_csv, mode="w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file, delimiter=';')
            writer.writerow(["Index", "Ch1(uV)", "Ch2(uV)", "StimState"])

            max_len = max(len(processed_ch1), len(processed_ch2))

            for i in range(max_len):
                v1 = processed_ch1[i] if i < len(processed_ch1) else ""
                v2 = processed_ch2[i] if i < len(processed_ch2) else ""
                s  = stim_state[i] if i < len(stim_state) else 0

                writer.writerow([
                    i,
                    f"{v1:.6f}".replace(".", ",") if v1 != "" else "",
                    f"{v2:.6f}".replace(".", ",") if v2 != "" else "",
                    s
                ])

        # --- HTML interactivo ---

        df = pd.DataFrame({
            "Index": list(range(N)),
            "Ch1": [processed_ch1[i] if i < len(processed_ch1) else None for i in range(N)],
            "Ch2": [processed_ch2[i] if i < len(processed_ch2) else None for i in range(N)],
            "StimState": stim_state
        })

        # Crear figura desde cero (dos canales)
        fig = go.Figure()

        fig.add_scatter(x=df["Index"], y=df["Ch1"],
                        mode="lines", name="Canal 1")

        fig.add_scatter(x=df["Index"], y=df["Ch2"],
                        mode="lines", name="Canal 2")

        fig.update_layout(
            title=f"ECG signals - {experiment_id}",
            xaxis_title="Índice",
            yaxis_title="Amplitud (uV)",
            template="plotly_white"
        )

        # Añadir rectángulos verticales para intervalos de estimulación
        for k, (on_idx, off_idx) in enumerate(stim_intervals):
            fig.add_vrect(
                x0=on_idx, x1=off_idx,
                fillcolor="green", opacity=0.15, line_width=0,
                annotation_text=f"STIM #{k+1}",
                annotation_position="top left"
            )

        # Intervalo actual si sigue ON
        if current_stim_state == 1 and pending_on_idx is not None:
            fig.add_vrect(
                x0=pending_on_idx, x1=N,
                fillcolor="green", opacity=0.10, line_width=0,
                annotation_text="STIM actual",
                annotation_position="top left"
            )

        # Marcas ON/OFF con timestamps (solo si coinciden longitudes)
        for (on_idx, off_idx), (on_ts, off_ts) in zip(stim_intervals, stim_intervals_ts):
            fig.add_scatter(
                x=[on_idx, off_idx],
                y=[
                    df["Ch1"][on_idx] if on_idx < len(df["Ch1"]) else None,
                    df["Ch1"][off_idx - 1] if off_idx - 1 < len(df["Ch1"]) else None
                ],
                mode="markers",
                marker=dict(color="green", size=6),
                name="STIM on/off",
                hovertext=[f"ON t={on_ts:.3f}s", f"OFF t={off_ts:.3f}s"],
                hoverinfo="text"
            )

        fig.write_html(filename_html, auto_open=True)

                # ---- Save stim timestamps ----
        filename_stim = f"{experiment_id}_STIM_EVENTS_{timestamp}.csv"
        with open(filename_stim, "w", newline="") as f:
            writer = csv.writer(f, delimiter=";")
            writer.writerow(["Event", "Timestamp_us"])
            for event, ts in stim_events_us:
                writer.writerow([event, ts])


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
    global processed_data, received_raw_buffer, sample_counter
    global stim_intervals, pending_on_idx, stim_intervals_ts, pending_on_ts
    global current_stim_state
    global processed_ch1, processed_ch2


    processed_ch1.clear()
    processed_ch2.clear()
    sample_counter_ch1 = 0
    sample_counter_ch2 = 0


    stim_intervals.clear()
    stim_intervals_ts.clear()
    pending_on_idx = None
    pending_on_ts = None
    current_stim_state = 0

    if log_text:
        log_text.insert(tk.END, "🧹 Memory cleared (data + buffer + intervals).\n")
        log_text.see(tk.END)



# ------------------ GUI Tkinter ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3 - IKKI Sense Control")
    root.geometry("1200x800")   # ventana más compacta

    tk.Label(root, text="IKKI - SENSE & STIM", font=("Arial", 16, "bold")).pack(pady=5)

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
    frame_params_container.pack(padx=10, pady=10, fill="both")
    
    # ================== PARÁMETROS A ==================
    frame_A = ttk.LabelFrame(frame_params_container, text="Sense parameters", padding=5)
    frame_A.pack(side="left", padx=2, fill="both", expand=True)

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
    # entry_id[-1].insert(0, "R1")
    entry_id.grid(row=row_i, column=1, sticky="e", padx=3, pady=1)
    row_i += 1

    allowed_fc_low = [
        "1000","500","250","200","100","75","50","30","25","20","15",
        "10","7.5","5","3","2.5","2","1.5","1","0.75","0.5","0.3",
        "0.25","0.1"
    ]


    # Campos compactados
    add_row("ADC sampling rate (kHz)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1.3")

    add_row("DSP cutoff frequency (Hz)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "40")

    add_row("Channels (max 15)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1")

    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("DSP enabled", cb)
    cb.set("0")   # ✔

    add_row("Initial channel", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(1, "1")

    add_row("fc high magnitude", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1")

    cb = ttk.Combobox(frame_A, values=["kHz", "Hz"], width=16, state="readonly")
    add_row("fc high unit", cb)
    cb.set("kHz")   # ✔

    cb = ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly")
    add_row("fc low A", cb)
    cb.set("10")

    cb = ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly")
    add_row("fc low B", cb)
    cb.set("10")

    cb = ttk.Combobox(frame_A, values=["A", "B"], width=16, state="readonly")
    add_row("Amplifier cutoff", cb)
    cb.set("A")

    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("C2 enabled", cb)
    cb.set("0")

    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("Absolute value mode", cb)
    cb.set("0")

    



    # ================== PARÁMETROS B ==================
    frame_B = ttk.LabelFrame(frame_params_container, text="Stimulation time parameters", padding=10)
    frame_B.pack(side="left", padx=10, fill="both", expand=True)

    frame_id = ttk.Frame(frame_B)
    frame_id.pack(fill="x", pady=5)

    labels_a = [
        "Number of stimulations",
        "Resting time (ms)",
        "Stimulation time (ms)",
        "Stimulation on time (μs)",
        "Stimulation off time (ms)"
    ]
    entries_b = []
    for lbl in labels_a:
        row = ttk.Frame(frame_B)
        row.pack(fill="x", pady=2)
        ttk.Label(row, text=lbl, width=30).pack(side="left")
        entry = ttk.Entry(row, width=20)
        entry.pack(side="right", padx=5)
        entries_b.append(entry)
    # Number of stimulations
    entries_b[0].insert(0, "10")

    # Resting time (ms)
    entries_b[1].insert(0, "1")

    # Stimulation time (s)
    entries_b[2].insert(0, "30")

    # Stimulation ON time (µs)
    entries_b[3].insert(0, "1000")

    # Stimulation OFF time (ms)
    entries_b[4].insert(0, "50")


    # ================== PARÁMETROS C ==================
    frame_C = ttk.LabelFrame(frame_params_container, text="Current parameters", padding=10)
    frame_C.pack(side="right", padx=10, fill="both", expand=True)

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
    row_pos_mag = ttk.Frame(frame_C)
    row_pos_mag.pack(fill="x", pady=2)
    ttk.Label(row_pos_mag, text="Positive current magnitude (<256)", width=35).pack(side="left")
    entry_pos_mag = ttk.Entry(row_pos_mag, width=20, validate="key", validatecommand=vcmd)
    entry_pos_mag.pack(side="right", padx=5)

    # Negative current magnitude
    row_neg_mag = ttk.Frame(frame_C)
    row_neg_mag.pack(fill="x", pady=2)
    ttk.Label(row_neg_mag, text="Negative current magnitude (<256)", width=35).pack(side="left")
    entry_neg_mag = ttk.Entry(row_neg_mag, width=20, validate="key", validatecommand=vcmd)
    entry_neg_mag.pack(side="right", padx=5)

    # Current step (único dropdown)
    row_step = ttk.Frame(frame_C)
    row_step.pack(fill="x", pady=2)
    

    ttk.Label(row_step, text="Current step (nA)", width=35).pack(side="left")
    step_var = tk.StringVar()
    step_dropdown = ttk.Combobox(row_step, textvariable=step_var, state="readonly", width=18)
    step_dropdown["values"] = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000]
    step_dropdown.current(0)
    step_dropdown.pack(side="right", padx=5)

    entry_pos_mag.insert(0, "10")
    entry_neg_mag.insert(0, "10")
    step_var.set("100")    # ✔ por defecto (nA)

    # ---------- SUBSECCIÓN DE RESULTADOS ----------
    subframe_result = ttk.LabelFrame(frame_C, text="Current calculation", padding=10)
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
    entries_c = [entry_pos_mag, entry_neg_mag, step_var]

    

    # ---- Botones inferiores ----
    ttk.Button(root, text="📤 Send new data",
               command=lambda: send_data(entries_a, entries_b, entries_c, log_text)
               ).pack(pady=7)

    ttk.Button(root, text="📊 Show complete graph",
               command=show_graph).pack(pady=5)

    ttk.Button(root, text="📈 Show real-time graph (3s)",
               command=show_realtime_graph).pack(pady=5)

    root.mainloop()
# --------- AsyncIO loop en hilo dedicado ---------
async_loop = asyncio.new_event_loop()

def _start_loop(loop):
    # Asegura que este 'loop' sea el event loop activo en este hilo
    asyncio.set_event_loop(loop)
    loop.run_forever()


# -----------------------------------------------
# ------------------ MAIN ------------------
if __name__ == "__main__":
    # Arranca el event loop en un hilo daemon
    loop_thread = threading.Thread(target=_start_loop, args=(async_loop,), daemon=True)
    loop_thread.start()

    run_gui()
