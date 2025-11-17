import asyncio
import threading
from bleak import BleakClient, BleakScanner, BleakError
import tkinter as tk
from tkinter import ttk, messagebox
from datetime import datetime
import csv
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
processed_data = []
async_loop = asyncio.new_event_loop()

# ------------------ BLE callback ------------------
def notification_handler(sender, data):
    global received_raw_buffer, processed_data
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

    if i > 0:
        received_raw_buffer = received_raw_buffer[i:]

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

def run_async_task(coro):
    asyncio.run_coroutine_threadsafe(coro, async_loop)

def connect_ble(log_text):
    run_async_task(connect_ble_async(log_text))

def send_data(entries, log_text):
    values = []
    for entry in entries:
        val = entry.get().strip()
        if not val:
            messagebox.showwarning("Empty values", "Please fill all fields.")
            return
        values.append(val)

    data = ",".join(values)
    run_async_task(send_data_async(data, log_text))

# ------------------ GUI ------------------
def run_gui():
    root = tk.Tk()
    root.title("BLE Viewer ESP32S3")
    root.geometry("450x680")

    tk.Label(root, text="IKKI - SENSE", font=("Arial", 16, "bold")).pack(pady=5)

    frame_conn = ttk.LabelFrame(root, text="Conexión BLE", padding=5)
    frame_conn.pack(padx=8, pady=8, fill="x")

    log_text = tk.Text(frame_conn, height=5, width=70)
    log_text.pack(padx=3, pady=3)
    ttk.Button(frame_conn, text="🔗 Connect BLE", command=lambda: connect_ble(log_text)).pack(pady=4)

    frame_params = ttk.LabelFrame(root, text="Stimulation parameters", padding=5)
    frame_params.pack(padx=8, pady=8, fill="both", expand=True)

    entries = []
    labels = ["ADC sampling rate (kHz)", "DSP cutoff frequency (Hz)", "Channels (max 15)", 
              "DSP enabled", "Initial channel", "fc high magnitude", "fc high unit", 
              "fc low A", "fc low B", "step DAC"]

    for i, label in enumerate(labels):
        ttk.Label(frame_params, text=label).grid(row=i, column=0, sticky="w", padx=3, pady=1)
        if "enabled" in label:
            widget = ttk.Combobox(frame_params, values=["0","1"], state="readonly", width=16)
        elif "unit" in label:
            widget = ttk.Combobox(frame_params, values=["kHz","Hz"], state="readonly", width=16)
        else:
            widget = ttk.Entry(frame_params, width=18)
        widget.grid(row=i, column=1, sticky="e", padx=3, pady=1)
        entries.append(widget)

    ttk.Button(root, text="📤 Send new data", command=lambda: send_data(entries, log_text)).pack(pady=7)
    root.mainloop()

if __name__ == "__main__":
    threading.Thread(target=async_loop.run_forever, daemon=True).start()
    run_gui()
