import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
import csv
from datetime import datetime
import time

from matplotlib.animation import FuncAnimation

# ================= BLE UUIDs =================
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
DEVICE_NAME = "ESP32S3_BLE"

# ================= Globales =================
ble_client = None
is_connected = False

received_raw_buffer = []
processed_ch1 = []
processed_ch2 = []

sample_counter_ch1 = 0
sample_counter_ch2 = 0

stim_events = []
stim_intervals = []
pending_stim = None
current_stim_state = 0
number_stimulations = 0

# ================= Tk safe helper =================
def tk_safe(widget, func, *args):
    widget.after(0, lambda: func(*args))

# ================= BLE callback =================
def notification_handler(sender, data):
    global received_raw_buffer
    global processed_ch1, processed_ch2
    global sample_counter_ch1, sample_counter_ch2
    global stim_events, stim_intervals, pending_stim, current_stim_state
    global number_stimulations

    # -------- STIM text --------
    try:
        text = data.decode("utf-8").strip()
        ts_us = time.time_ns() // 1000

        if text == "STIM_ON" and current_stim_state == 0:
            current_stim_state = 1
            stim_events.append({
                "event": "ON",
                "ts_us": ts_us,
                "ch1_idx": sample_counter_ch1,
                "ch2_idx": sample_counter_ch2
            })
            pending_stim = {
                "on_ts_us": ts_us,
                "on_idx_ch1": sample_counter_ch1,
                "on_idx_ch2": sample_counter_ch2
            }
            number_stimulations += 1
            return

        if text == "STIM_OFF" and current_stim_state == 1:
            current_stim_state = 0
            stim_events.append({
                "event": "OFF",
                "ts_us": ts_us,
                "ch1_idx": sample_counter_ch1,
                "ch2_idx": sample_counter_ch2
            })
            if pending_stim:
                stim_intervals.append({
                    "on_ts_us": pending_stim["on_ts_us"],
                    "off_ts_us": ts_us,
                    "on_idx_ch1": pending_stim["on_idx_ch1"],
                    "off_idx_ch1": sample_counter_ch1,
                    "on_idx_ch2": pending_stim["on_idx_ch2"],
                    "off_idx_ch2": sample_counter_ch2
                })
                pending_stim = None
            return

    except Exception:
        pass

    # -------- Binary ECG --------
    values = list(data)
    received_raw_buffer.extend(values)

    i = 0
    while i <= len(received_raw_buffer) - 3:
        header = received_raw_buffer[i]

        if header == 0x31:
            val = (received_raw_buffer[i+1] << 8) | received_raw_buffer[i+2]
            processed_ch1.append((val, time.time_ns() // 1000))
            sample_counter_ch1 += 1
            i += 3
        elif header == 0x71:
            val = (received_raw_buffer[i+1] << 8) | received_raw_buffer[i+2]
            processed_ch2.append((val, time.time_ns() // 1000))
            sample_counter_ch2 += 1
            i += 3
        else:
            i += 1

    received_raw_buffer = received_raw_buffer[i:]

# ================= BLE async =================
async def connect_ble_async(log_text):
    tk_safe(log_text, log_text.insert, tk.END, "🔍 Scanning BLE...\n")
    devices = await BleakScanner.discover()

    address = None
    for d in devices:
        if d.name and DEVICE_NAME in d.name:
            address = d.address
            break

    if not address:
        tk_safe(log_text, log_text.insert, tk.END, "❌ Device not found\n")
        return

    global ble_client, is_connected
    ble_client = BleakClient(address)

    try:
        await ble_client.connect(timeout=10)
        is_connected = True
        await ble_client.start_notify(CHARACTERISTIC_UUID_TX, notification_handler)
        tk_safe(log_text, log_text.insert, tk.END, "✅ Connected\n")
    except BleakError as e:
        tk_safe(log_text, log_text.insert, tk.END, f"⚠️ {e}\n")

async def send_data_async(data, log_text):
    if not ble_client or not is_connected:
        tk_safe(log_text, log_text.insert, tk.END, "❌ Not connected\n")
        return
    await ble_client.write_gatt_char(CHARACTERISTIC_UUID_RX, data.encode())
    tk_safe(log_text, log_text.insert, tk.END, f"📤 {data}\n")

# ================= Wrappers =================
def run_async_task(coro):
    asyncio.run_coroutine_threadsafe(coro, async_loop)

def connect_ble(log_text):
    run_async_task(connect_ble_async(log_text))

def send_data(entries_a, entries_b, entries_c, log_text):
    values = [e.get() for e in entries_a + entries_b + entries_c]
    if not all(values):
        messagebox.showwarning("Empty", "Fill all values")
        return
    run_async_task(send_data_async(",".join(values), log_text))

# ================= Graphs =================
def show_realtime_graph():
    fig, ax = plt.subplots(figsize=(10, 5))
    window = 2000

    def update(_):
        ax.clear()
        ax.set_title("Real-time signal")
        ax.grid(True)

        if processed_ch1:
            ax.plot([v for v, _ in processed_ch1[-window:]], label="Ch1")
        if processed_ch2:
            ax.plot([v for v, _ in processed_ch2[-window:]], label="Ch2")

        max_len = max(len(processed_ch1), len(processed_ch2), 1)
        for it in stim_intervals:
            if it["off_idx_ch1"] >= max_len - window:
                x0 = max(0, it["on_idx_ch1"] - (max_len - window))
                x1 = min(window, it["off_idx_ch1"] - (max_len - window))
                ax.axvspan(x0, x1, alpha=0.2)

        ax.legend()

    FuncAnimation(fig, update, interval=100)
    plt.show(block=False)

# ================= GUI =================
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer")

    log = tk.Text(root, height=6)
    log.pack()

    ttk.Button(root, text="Connect", command=lambda: connect_ble(log)).pack()
    ttk.Button(root, text="Real-time graph", command=show_realtime_graph).pack()

    root.mainloop()

# ================= Async loop =================
async_loop = asyncio.new_event_loop()

def _start_loop(loop):
    asyncio.set_event_loop(loop)
    loop.run_forever()

if __name__ == "__main__":
    threading.Thread(target=_start_loop, args=(async_loop,), daemon=True).start()
    run_gui()
