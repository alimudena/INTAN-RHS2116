import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
import csv
from datetime import datetime
import time
import math


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

# Cada canal almacena tuplas (value_uV, timestamp_us)
processed_ch1 = []   # Canal 1 (0x31)
processed_ch2 = []   # Canal 2 (0x71)

sample_counter_ch1 = 0
sample_counter_ch2 = 0

number_stimulations = 0

realtime_anim = None

# === CAMBIO: eventos de estimulación ricos en info ===
# Lista de dicts: {"event":"ON"/"OFF", "ts_us":..., "ch1_idx":..., "ch2_idx":...}
stim_events = []

# Intervalos de estimulación con mapeo por canal e timestamps
# Cada elemento: {"on_idx_ch1":..., "off_idx_ch1":..., "on_idx_ch2":..., "off_idx_ch2":..., "on_ts_us":..., "off_ts_us":...}
stim_intervals = []
# Si hay un ON pendiente (aún no se recibió OFF), lo guardamos aquí:
pending_stim = None  # dict temporal con on info

current_stim_state = 0       # 0=OFF, 1=ON

start_time = time.time()

APP_START_NS = time.perf_counter_ns()


def imprimir_con_tiempo(values):
    now_ns = time.perf_counter_ns()
    elapsed_ns = now_ns - APP_START_NS
    elapsed_us = elapsed_ns / 1_000  # microsegundos
    # Formato con 0 decimales (entero) o con decimales
    # print(f"{elapsed_us:.0f}")  # entero en μs
    # print(f"{values}  |  desde inicio: {elapsed_us:.0f} μs")  # entero en μs
    # Alternativa con 3 decimales en milisegundos:
    # print(f"{values}  |  desde inicio: {elapsed_us/1000:.3f} ms")


# ------------------ BLE callback ------------------
def notification_handler(sender, data):
    """
    Recibe paquetes BLE. Diferencia si vienen como texto ("STIM_ON"/"STIM_OFF")
    o como bytes para los canales. Guarda timestamps en µs y mapea eventos a contadores de muestra.
    """
    global received_raw_buffer
    global processed_ch1, processed_ch2
    global sample_counter_ch1, sample_counter_ch2
    global stim_events, stim_intervals, pending_stim, current_stim_state
    global number_stimulations

    # -------- Eventos de estimulación (texto) --------
    try:
        text = data.decode("utf-8").strip()
        ts_us = time.time_ns() // 1000  # timestamp en microsegundos

        if text == "STIM_ON":
            # Si ya está ON, ignoramos
            if current_stim_state == 0:
                current_stim_state = 1
                # Registramos evento con contadores actuales
                ev = {
                    "event": "ON",
                    "ts_us": ts_us,
                    "ch1_idx": sample_counter_ch1,
                    "ch2_idx": sample_counter_ch2
                }
                stim_events.append(ev)
                # Creamos pending_stim
                pending_stim = {
                    "on_ts_us": ts_us,
                    "on_idx_ch1": sample_counter_ch1,
                    "on_idx_ch2": sample_counter_ch2
                }
                number_stimulations = number_stimulations +1
                print(f"⚡ STIM ON — ts={ts_us} us, ch1_idx={sample_counter_ch1}, ch2_idx={sample_counter_ch2}")
                print(f"Number of stimulations={number_stimulations}")

            return

        if text == "STIM_OFF":
            if current_stim_state == 1:
                current_stim_state = 0
                ev = {
                    "event": "OFF",
                    "ts_us": ts_us,
                    "ch1_idx": sample_counter_ch1,
                    "ch2_idx": sample_counter_ch2
                }
                stim_events.append(ev)

                # Cerramos el pending_stim y añadimos a stim_intervals
                if pending_stim is not None:
                    interval = {
                        "on_ts_us": pending_stim["on_ts_us"],
                        "off_ts_us": ts_us,
                        "on_idx_ch1": pending_stim["on_idx_ch1"],
                        "off_idx_ch1": sample_counter_ch1,
                        "on_idx_ch2": pending_stim["on_idx_ch2"],
                        "off_idx_ch2": sample_counter_ch2
                    }
                    stim_intervals.append(interval)
                    pending_stim = None

                print(f"⚡ STIM OFF — ts={ts_us} us, ch1_idx={sample_counter_ch1}, ch2_idx={sample_counter_ch2}")
            return

    except Exception:
        # No era texto o fallo al decodificar -> tratamos como datos binarios
        pass

    # -------- Decodificación ECG (binario) --------
    try:
        values = list(data)
        received_raw_buffer.extend(values)
        imprimir_con_tiempo(values)
        # print(values)

        i = 0
        while i <= len(received_raw_buffer) - 3:
            header = received_raw_buffer[i]

            # ---- Canal 1 (0x31) ----
            if header == 0x31:
                high_word = received_raw_buffer[i+1]
                low_word  = received_raw_buffer[i+2]
                combined_value = (high_word << 8) | low_word
                real_value = 0.195 * (combined_value - 32768)
                # real_value = combined_value
                timestamp_us = time.time_ns() // 1000
                processed_ch1.append((real_value, timestamp_us))
                sample_counter_ch1 += 1
                i += 3
                continue

            # ---- Canal 2 (0x71) ----
            elif header == 0x71:
                high_word = received_raw_buffer[i+1]
                low_word  = received_raw_buffer[i+2]
                combined_value = (high_word << 8) | low_word
                real_value = 0.195 * (combined_value - 32768)
                # real_value = combined_value
                timestamp_us = time.time_ns() // 1000
                processed_ch2.append((real_value, timestamp_us))
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


def send_data(entries_a, entries_b, entries_c, bipolar_cb, log_text):
    # Combina todos los valores de los parámetros A y B
    values_a = [entry.get().strip() for entry in entries_a]
    values_b = [entry.get().strip() for entry in entries_b]
    values_b.append(bipolar_cb.get())
    values_c = [entry.get().strip() for entry in entries_c]


    all_values = values_a + values_b + values_c

    # Verifica que no haya campos vacíos
    if not all(all_values):
        messagebox.showwarning("Empty values", "Please, fill in values before sending.")
        return

    data = ",".join(all_values)
    run_async_task(send_data_async(data, log_text))

# ------------------ Gráfica (sin cambios funcionales importantes) ------------------
def show_graph():
    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return

    fig, ax = plt.subplots(figsize=(10, 5))

    if processed_ch1:
        # ploteamos valores (ignoramos timestamp para este plot)
        ax.plot([v for v, t in processed_ch1], label="Ch 1", lw=1)

    if processed_ch2:
        ax.plot([v for v, t in processed_ch2], label="Ch 2", lw=1)

    ax.set_title("Received signals — Ch 1 and Ch 2")
    ax.set_xlabel("Index")
    ax.set_ylabel("uV")
    ax.set_ylim(-10000, 10000)  
    ax.grid(True)
    ax.legend()

    # Dibujar marcadores de estimulación: usamos índices por canal si es posible
    for interval in stim_intervals:
        # Dibujamos rectángulos combinando los índices máximos para visualización simplificada
        on_idx = min(interval.get("on_idx_ch1", 0), interval.get("on_idx_ch2", 0))
        off_idx = max(interval.get("off_idx_ch1", on_idx), interval.get("off_idx_ch2", on_idx))
        ax.axvline(on_idx, color="green", linestyle="--", linewidth=1)
        ax.axvline(off_idx, color="red", linestyle="--", linewidth=1)


    plt.show()

def show_realtime_graph():
    global realtime_anim

    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    window_samples = 3800

    def update(_):
        ax.clear()
        ax.set_title("Real time signal")
        ax.set_xlabel("Index")
        ax.set_ylabel("uV")
        ax.grid(True)
        # ax.set_ylim(0, 2**16)
        ax.set_ylim(-10000, 10000)  

        if processed_ch1:
            y1 = [v for v, _ in processed_ch1[-window_samples:]]
            ax.plot(y1, label="Ch 1", lw=1)

        if processed_ch2:
            y2 = [v for v, _ in processed_ch2[-window_samples:]]
            ax.plot(y2, label="Ch 2", lw=1)

        max_len = max(len(processed_ch1), len(processed_ch2), 1)

        for interval in stim_intervals:
            on_idx = interval["on_idx_ch1"]
            off_idx = interval["off_idx_ch1"]

            if off_idx >= max_len - window_samples:
                x0 = max(0, on_idx - (max_len - window_samples))
                x1 = min(window_samples, off_idx - (max_len - window_samples))
                ax.axvspan(x0, x1, color="green", alpha=0.15)

        ax.legend(loc="upper right")

    realtime_anim = FuncAnimation(
        fig,
        update,
        interval=100,
        cache_frame_data=False
    )

    plt.show(block=False)

    
# def show_realtime_graph():
#     if not processed_ch1 and not processed_ch2:
#         messagebox.showinfo("Sin datos", "No hay datos recibidos todavía.")
#         return

#     fig, ax = plt.subplots(figsize=(10, 5))

#     line1, = ax.plot([], [], label="Ch 1", lw=1)
#     line2, = ax.plot([], [], label="Ch 2", lw=1)

#     ax.set_title("Real time signal")
#     ax.set_xlabel("Index")
#     ax.set_ylabel("uV")
#     # ax.set_ylim(-7000, 7000)
#     ax.grid(True)
#     ax.legend()

#     window_samples = 2000

#     def update(frame):
#         if processed_ch1:
#             data1 = [v for v, t in processed_ch1[-window_samples:]]
#             line1.set_data(range(len(data1)), data1)

#         if processed_ch2:
#             data2 = [v for v, t in processed_ch2[-window_samples:]]
#             line2.set_data(range(len(data2)), data2)

#         max_len = max(len(processed_ch1), len(processed_ch2), 1)
#         ax.set_xlim(0, min(window_samples, max_len))

#         # Marcas de estimulación dentro del rango: convertimos índice absoluto a ventana
#         for interval in stim_intervals:
#             on_idx = min(interval.get("on_idx_ch1", 0), interval.get("on_idx_ch2", 0))
#             off_idx = max(interval.get("off_idx_ch1", on_idx), interval.get("off_idx_ch2", on_idx))
#             # si el intervalo está dentro de los últimos max_len muestras
#             if off_idx >= max_len - window_samples:
#                 x0 = max(0, on_idx - (max_len - window_samples))
#                 x1 = min(window_samples, off_idx - (max_len - window_samples))
#                 ax.axvspan(x0, x1, color="green", alpha=0.12)

#         return line1, line2

#     anim = FuncAnimation(fig, update, interval=100, cache_frame_data=False)
#     plt.show()

# ------------------ Export / Reconstrucción ------------------
def save_to_csv(entry_id):
    """
    Exporta:
     - CSV unificado (por timeline): Index, Time_us, Ch1_val, Ch1_ts_us, Ch2_val, Ch2_ts_us, StimState
     - CSV con eventos de STIM y su mapeo a índices por canal
    """
    global processed_ch1, processed_ch2, stim_intervals, stim_events, pending_stim, current_stim_state

    if not processed_ch1 and not processed_ch2:
        messagebox.showinfo("No data", "No data received to be shown.")
        return

    experiment_id = entry_id.get().strip() or "experiment"
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename_csv = f"{experiment_id}_ECG_timeline_{timestamp}.csv"
    filename_stim = f"{experiment_id}_STIM_EVENTS_{timestamp}.csv"

    try:
        # --- Preparamos las listas por canal: (ts_us, value) ---
        ch1 = [(ts, val) for (val, ts) in [(v, t) for v, t in processed_ch1]]
        # pero processed_ch1 es (value, ts); transformo a (ts, val)
        ch1 = [(t, v) for (v, t) in processed_ch1]
        ch2 = [(t, v) for (v, t) in processed_ch2]

        # --- Creamos timeline unificado (ordenado por timestamp) ---
        # Tomamos todos los timestamps y ordenamos únicos:
        all_ts = sorted(set([t for t, v in ch1] + [t for t, v in ch2]))
        if len(all_ts) == 0:
            raise RuntimeError("No timestamps to export.")

        # Indices para buscar el último valor <= ts (vamos a iterar secuencialmente)
        idx1 = 0
        idx2 = 0
        last_val1 = None
        last_ts1 = None
        last_val2 = None
        last_ts2 = None

        # Preparamos intervalos por timestamps para determinar StimState
        stim_ranges = []
        for interval in stim_intervals:
            on = interval.get("on_ts_us")
            off = interval.get("off_ts_us")
            if on is not None and off is not None:
                stim_ranges.append((on, off))
        # Si hay un pending_stim (ON sin OFF) lo añadimos hasta último ts
        if pending_stim is not None:
            stim_ranges.append((pending_stim["on_ts_us"], all_ts[-1]+1))

        # Función para comprobar si un timestamp está dentro de algún stim_range
        def is_stim_at(ts):
            for a,b in stim_ranges:
                if a <= ts < b:
                    return 1
            return 0

        # --- Escribimos CSV unificado ---
        with open(filename_csv, mode="w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file, delimiter=';')
            writer.writerow(["Index", "Time_us", "Ch1_val_uV", "Ch1_ts_us", "Ch2_val_uV", "Ch2_ts_us", "StimState"])

            idx = 0
            # Usamos dos punteros para avanzar por ch1, ch2
            # ch1 and ch2 are sorted by ts already by construction
            for ts in all_ts:
                # Avanzamos idx1 hasta que ch1[idx1].ts > ts
                while idx1 < len(ch1) and ch1[idx1][0] <= ts:
                    last_ts1, last_val1 = ch1[idx1]
                    idx1 += 1
                while idx2 < len(ch2) and ch2[idx2][0] <= ts:
                    last_ts2, last_val2 = ch2[idx2]
                    idx2 += 1

                s = is_stim_at(ts)

                writer.writerow([
                    idx,
                    ts,
                    f"{last_val1:.6f}".replace(".", ",") if last_val1 is not None else "",
                    last_ts1 if last_ts1 is not None else "",
                    f"{last_val2:.6f}".replace(".", ",") if last_val2 is not None else "",
                    last_ts2 if last_ts2 is not None else "",
                    s
                ])
                idx += 1

        # --- Guardamos archivo de eventos STIM con mapeo a índices por canal ---
        with open(filename_stim, mode="w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f, delimiter=';')
            writer.writerow(["Event", "ts_us", "ch1_idx", "ch2_idx"])
            for ev in stim_events:
                writer.writerow([ev.get("event"), ev.get("ts_us"), ev.get("ch1_idx"), ev.get("ch2_idx")])

        messagebox.showinfo(
            "Éxito",
            f"✅ Datos guardados correctamente.\n\n"
            f"📁 CSV timeline: {filename_csv}\n"
            f"📁 STIM events: {filename_stim}\n\n"
            "Puedes reconstruir la señal usando los timestamps y los mapeos de índices."
        )

    except Exception as e:
        messagebox.showerror("Error", f"No se pudo guardar el archivo:\n{e}")

# ------------------ Clear memory ------------------
def clear_memory(log_text=None):
    global processed_ch1, processed_ch2, received_raw_buffer
    global sample_counter_ch1, sample_counter_ch2
    global stim_intervals, pending_stim, stim_events, current_stim_state
    global number_stimulations

    processed_ch1.clear()
    processed_ch2.clear()
    received_raw_buffer.clear()
    sample_counter_ch1 = 0
    sample_counter_ch2 = 0

    stim_intervals.clear()
    stim_events.clear()
    pending_stim = None
    current_stim_state = 0
    number_stimulations = 0

    if log_text:
        log_text.insert(tk.END, "🧹 Memory cleared (data + buffer + intervals).\n")
        log_text.see(tk.END)

def timestamp_to_index(ts_event, processed):
    """
    Convierte un timestamp de estimulación al índice real de la señal procesada.
    processed = [(value, timestamp), ...]
    """
    if not processed:
        return None

    # Búsqueda binaria por timestamp
    lo, hi = 0, len(processed) - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if processed[mid][1] < ts_event:
            lo = mid + 1
        else:
            hi = mid

    return lo


# ------------------ GUI Tkinter ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3 - Sense and Stim Control")
    root.geometry("1200x800")

    tk.Label(root, text="SENSE & STIM", font=("Arial", 16, "bold")).pack(pady=5)

    # ---------- CONEXIÓN ----------
    frame_conn = ttk.LabelFrame(root, text="BLE connection", padding=5)
    frame_conn.pack(padx=8, pady=8, fill="x")

    log_text = tk.Text(frame_conn, height=5, width=70)
    log_text.pack(padx=3, pady=3)

    
    frame_actions = ttk.LabelFrame(root, text="Actions", padding=10)
    frame_actions.pack(padx=10, pady=10, fill="x")

    ttk.Button(
    frame_actions,
    text="🔗 Connect BLE",
    command=lambda: connect_ble(log_text)
    ).grid(row=0, column=0, padx=5, pady=5)
    ttk.Button(
    frame_actions,
    text="📤 Send new data",
    command=lambda: send_data(entries_a, entries_b, entries_c, bipolar_cb, log_text)
    ).grid(row=0, column=1, padx=5, pady=5)

    ttk.Button(
        frame_actions,
        text="💾 Save data in CSV timeline",
        command=lambda: save_to_csv(entry_id)
    ).grid(row=0, column=2, padx=5, pady=5)

    ttk.Button(
        frame_actions,
        text="🧹 Clear memory",
        command=lambda: clear_memory(log_text)
    ).grid(row=0, column=3, padx=5, pady=5)

    ttk.Button(
        frame_actions,
        text="📊 Show complete graph",
        command=show_graph
    ).grid(row=1, column=0, padx=5, pady=5)

    ttk.Button(
        frame_actions,
        text="📈 Show real-time graph (3s)",
        command=show_realtime_graph
    ).grid(row=1, column=1, padx=5, pady=5)



    # ---------- PARÁMETROS (compactados; se mantienen igual que en tu código) ----------
    frame_params_container = ttk.Frame(root)
    frame_params_container.pack(padx=10, pady=10, fill="both")

    frame_A = ttk.LabelFrame(frame_params_container, text="Sense parameters", padding=5)
    frame_A.pack(side="left", padx=2, fill="both", expand=True)

    entries_a = []
    row_i = 0

    def add_row(label_text, widget):
        nonlocal row_i
        ttk.Label(frame_A, text=label_text).grid(row=row_i, column=0, sticky="w", padx=3, pady=1)
        widget.grid(row=row_i, column=1, sticky="e", padx=3, pady=1)
        entries_a.append(widget)
        row_i += 1

    ttk.Label(frame_A, text="Experiment ID").grid(row=row_i, column=0, sticky="w", padx=3, pady=1)
    entry_id = ttk.Entry(frame_A, width=18)
    entry_id.grid(row=row_i, column=1, sticky="e", padx=3, pady=1)
    row_i += 1

    add_row("ADC sampling rate (kHz)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1.3")
    add_row("DSP cutoff frequency (Hz)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "40")
    add_row("Channels (max 15)", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1")
    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("DSP enabled", cb)
    cb.set("0")
    add_row("Initial channel", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(1, "1")
    add_row("fc high magnitude", ttk.Entry(frame_A, width=18))
    entries_a[-1].insert(0, "1")
    cb = ttk.Combobox(frame_A, values=["kHz", "Hz"], width=16, state="readonly")
    add_row("fc high unit", cb)
    cb.set("kHz")
    allowed_fc_low = [
        "1000","500","250","200","100","75","50","30","25","20","15",
        "10","7.5","5","3","2.5","2","1.5","1","0.75","0.5","0.3",
        "0.25","0.1"
    ]
    cb = ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly")
    add_row("fc low A", cb)
    cb.set("0.1")
    cb = ttk.Combobox(frame_A, values=allowed_fc_low, width=16, state="readonly")
    add_row("fc low B", cb)
    cb.set("0.1")
    cb = ttk.Combobox(frame_A, values=["A", "B"], width=16, state="readonly")
    add_row("Amplifier cutoff", cb)
    cb.set("A")
    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("C2 enabled", cb)
    cb.set("0")
    cb = ttk.Combobox(frame_A, values=["0", "1"], width=16, state="readonly")
    add_row("Absolute value mode", cb)
    cb.set("0")

    frame_B = ttk.LabelFrame(frame_params_container, text="Stimulation time parameters", padding=10)

    # ---- Bipolar stimulation ----
    row = ttk.Frame(frame_B)
    row.pack(fill="x", pady=2)

    ttk.Label(row, text="Bipolar_stim", width=30).pack(side="left")
    bipolar_cb = ttk.Combobox(row, values=["0", "1"], width=18, state="readonly")
    bipolar_cb.pack(side="right", padx=5)
    bipolar_cb.set("0")



    frame_B.pack(side="left", padx=10, fill="both", expand=True)

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
    entries_b[0].insert(0, "10")
    entries_b[1].insert(0, "1")
    entries_b[2].insert(0, "30")
    entries_b[3].insert(0, "1000")
    entries_b[4].insert(0, "50")

    frame_C = ttk.LabelFrame(frame_params_container, text="Current parameters", padding=10)
    frame_C.pack(side="right", padx=10, fill="both", expand=True)

    def validate_magnitude(value):
        if not value:
            return True
        if not value.isdigit():
            return False
        num = int(value)
        return 0 <= num < 256

    vcmd = (root.register(validate_magnitude), "%P")

    row_pos_mag = ttk.Frame(frame_C)
    row_pos_mag.pack(fill="x", pady=2)
    ttk.Label(row_pos_mag, text="Positive current magnitude (<256)", width=35).pack(side="left")
    entry_pos_mag = ttk.Entry(row_pos_mag, width=20, validate="key", validatecommand=vcmd)
    entry_pos_mag.pack(side="right", padx=5)

    row_neg_mag = ttk.Frame(frame_C)
    row_neg_mag.pack(fill="x", pady=2)
    ttk.Label(row_neg_mag, text="Negative current magnitude (<256)", width=35).pack(side="left")
    entry_neg_mag = ttk.Entry(row_neg_mag, width=20, validate="key", validatecommand=vcmd)
    entry_neg_mag.pack(side="right", padx=5)

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
    step_var.set("100")

    subframe_result = ttk.LabelFrame(frame_C, text="Current calculation", padding=10)
    subframe_result.pack(fill="x", pady=10)

    ttk.Label(subframe_result, text="Positive current value (nA):", width=35).grid(row=0, column=0, padx=5, pady=2, sticky="w")
    pos_result_var = tk.StringVar(value="0")
    entry_pos_result = ttk.Entry(subframe_result, textvariable=pos_result_var, width=20, state="readonly")
    entry_pos_result.grid(row=0, column=1, padx=5, pady=2)

    ttk.Label(subframe_result, text="Negative current value (nA):", width=35).grid(row=1, column=0, padx=5, pady=2, sticky="w")
    neg_result_var = tk.StringVar(value="0")
    entry_neg_result = ttk.Entry(subframe_result, textvariable=neg_result_var, width=20, state="readonly")
    entry_neg_result.grid(row=1, column=1, padx=5, pady=2)

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

    entries_c = [entry_pos_mag, entry_neg_mag, step_var]


    root.mainloop()

# --------- AsyncIO loop en hilo dedicado ---------
async_loop = asyncio.new_event_loop()

def _start_loop(loop):
    asyncio.set_event_loop(loop)
    loop.run_forever()

# -----------------------------------------------
# ------------------ MAIN ------------------
if __name__ == "__main__":
    loop_thread = threading.Thread(target=_start_loop, args=(async_loop,), daemon=True)
    loop_thread.start()
    run_gui()
