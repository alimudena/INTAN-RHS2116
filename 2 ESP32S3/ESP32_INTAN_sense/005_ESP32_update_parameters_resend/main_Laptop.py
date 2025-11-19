# main.py
import asyncio
import threading
import csv
import webbrowser
from datetime import datetime
from pathlib import Path

from kivy.clock import Clock, mainthread
from kivy.lang import Builder
from kivy.metrics import dp
from kivy.uix.boxlayout import BoxLayout
from kivy.core.window import Window

from kivymd.app import MDApp
from kivymd.uix.dialog import MDDialog
from kivymd.uix.button import MDRectangleFlatButton
from kivymd.uix.menu import MDDropdownMenu
from kivymd.uix.snackbar import Snackbar

# BLE
from bleak import BleakScanner, BleakClient, BleakError

# Matplotlib for "complete graph"
import matplotlib
matplotlib.use('Agg')  # render to image buffer (we will use FigureCanvasKivyAgg)
from matplotlib.figure import Figure
from kivy.garden.matplotlib import FigureCanvasKivyAgg

# KivyGraph for realtime
from kivy_garden.graph import Graph, MeshLinePlot

# Plotly for interactive HTML export
import plotly.express as px
import pandas as pd

# ---------------- BLE UUIDs ----------------
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
CHAR_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
CHAR_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
DEVICE_NAME = "ESP32S3_BLE"

# ---------------- Global state ----------------
processed_data = []     # valores reales (float)
raw_buffer = []         # buffer de bytes crudos
ble_client = None
is_connected = False

# Async loop for BLE operations (separate thread)
async_loop = asyncio.new_event_loop()

# ---------------- KV UI ----------------
KV = """
<MainScreen>:
    orientation: "vertical"
    padding: dp(12)
    spacing: dp(8)

    MDLabel:
        id: title
        text: "IKKI - SENSE (Android)"
        halign: "center"
        font_style: "H5"

    MDLabel:
        id: status_lbl
        text: "Estado: Desconectado"
        halign: "center"

    MDBoxLayout:
        size_hint_y: None
        height: dp(44)
        spacing: dp(8)

        MDRaisedButton:
            text: "🔗 Connect BLE"
            on_release: app.on_connect_pressed()

        MDRaisedButton:
            text: "🧹 Clear memory"
            on_release: app.clear_memory()

    MDBoxLayout:
        size_hint_y: None
        height: dp(44)
        spacing: dp(8)

        MDTextField:
            id: entry_id
            hint_text: "Experiment ID"
            size_hint_x: .6

        MDRaisedButton:
            text: "💾 Save CSV & HTML"
            on_release: app.on_save_pressed()

    MDSeparator:

    MDLabel:
        text: "Parámetros de estimulación"
        halign: "left"

    GridLayout:
        cols: 2
        row_default_height: dp(40)
        row_force_default: True
        spacing: dp(6)
        size_hint_y: None
        height: dp(40*8)

        MDTextField:
            id: sampling_rate
            hint_text: "ADC sampling rate (kHz)"
        MDTextField:
            id: dsp_cutoff
            hint_text: "DSP cutoff freq (Hz)"

        MDTextField:
            id: channels
            hint_text: "Channels (max 15)"
        MDTextField:
            id: dsp_enabled
            hint_text: "DSP enabled (0/1)"

        MDTextField:
            id: initial_channel
            hint_text: "Initial channel"
        MDTextField:
            id: fc_high_mag
            hint_text: "fc high magnitude"

        MDTextField:
            id: fc_high_unit
            hint_text: "fc high unit (kHz/Hz)"
        MDTextField:
            id: amplifier_cutoff
            hint_text: "Amplifier cutoff (A/B)"

    MDRaisedButton:
        text: "📤 Send parameters"
        on_release: app.on_send_pressed()

    MDSeparator:
    MDLabel:
        text: "Gráficas"
        halign: "left"

    BoxLayout:
        orientation: "horizontal"
        size_hint_y: None
        height: dp(240)
        spacing: dp(8)

        BoxLayout:
            id: plot_container
            orientation: "vertical"
            size_hint_x: .6

        BoxLayout:
            orientation: "vertical"
            size_hint_x: .4

            MDLabel:
                text: "Realtime (KivyGraph)"
                halign: "center"

            BoxLayout:
                id: realtime_container
                size_hint_y: None
                height: dp(180)

            MDLabel:
                text: ""
                halign: "center"

    BoxLayout:
        size_hint_y: None
        height: dp(48)
        spacing: dp(8)

        MDRaisedButton:
            text: "📊 Show complete graph"
            on_release: app.show_complete_graph()

        MDRaisedButton:
            text: "📈 Toggle realtime"
            on_release: app.toggle_realtime()

    Widget:
        size_hint_y: None
        height: dp(8)
"""

# ---------------- Notification handler ----------------
def process_incoming_bytes(data_bytes):
    """
    Procesa bytes crudos recibidos del ESP:
    Busca secuencias [0x31, high, low] y calcula real_value = 0.195*(combined-32768)
    """
    global raw_buffer, processed_data
    try:
        values = list(data_bytes)
        raw_buffer.extend(values)

        i = 0
        while i <= len(raw_buffer) - 3:
            if raw_buffer[i] == 0x31:
                high_word = raw_buffer[i + 1]
                low_word = raw_buffer[i + 2]
                combined = (high_word << 8) | low_word
                real_value = 0.195 * (combined - 32768)
                processed_data.append(real_value)
                i += 3
            else:
                i += 1

        # mantener solo el resto
        if i > 0:
            raw_buffer = raw_buffer[i:]
    except Exception as e:
        print("Error procesando bytes:", e)


# ---------------- App class ----------------
class MainScreen(BoxLayout):
    pass


class BLEApp(MDApp):
    def build(self):
        self.title = "IKKI Sense - Android"
        Window.softinput_mode = "below_target"
        self.root_widget = Builder.load_string(KV)
        self.main = MainScreen()
        self.root_widget.add_widget(self.main)
        self.realtime_enabled = False
        self.realtime_plot = None
        self.realtime_graph = None
        self.realtime_interval_event = None

        # Matplotlib figure placeholder
        self.mat_fig = Figure(figsize=(6, 3))
        self.mat_ax = self.mat_fig.add_subplot(111)
        self.canvas_widget = None

        # Start asyncio loop in background thread
        threading.Thread(target=async_loop.run_forever, daemon=True).start()

        return self.root_widget

    # ---------------- BLE connect ----------------
    def on_connect_pressed(self, *args):
        self.set_status("Buscando dispositivos BLE...")
        self.run_async_task(self.connect_ble_async())

    async def connect_ble_async(self):
        global ble_client, is_connected
        try:
            devices = await BleakScanner.discover()
        except Exception as e:
            self.show_dialog("Error", f"Error scanning BLE: {e}")
            return

        esp_addr = None
        for d in devices:
            if d.name and DEVICE_NAME in d.name:
                esp_addr = d.address
                break

        if not esp_addr:
            self.show_dialog("Resultado", "ESP32S3_BLE no encontrado.")
            self.set_status("Desconectado")
            return

        ble_client = BleakClient(esp_addr, loop=async_loop)
        try:
            await ble_client.connect(timeout=10.0)
            if ble_client.is_connected:
                is_connected = True
                # start notify
                await ble_client.start_notify(CHAR_TX, self._notification_callback)
                self.set_status("Conectado: " + (esp_addr or "ESP32"))
                self.show_snackbar("Conectado correctamente.")
            else:
                self.set_status("Desconectado")
                self.show_dialog("Error", "No se pudo conectar.")
        except BleakError as e:
            self.set_status("Desconectado")
            self.show_dialog("BleakError", str(e))
        except Exception as e:
            self.set_status("Desconectado")
            self.show_dialog("Error", str(e))

    def _notification_callback(self, sender, data):
        # Esta función se ejecuta en hilo del loop BLE; pasamos al procesamiento
        process_incoming_bytes(data)

    # ---------------- Sending data ----------------
    def on_send_pressed(self, *args):
        # Recopilar entradas de parámetros
        vals = []
        ids = [
            "sampling_rate", "dsp_cutoff", "channels", "dsp_enabled",
            "initial_channel", "fc_high_mag", "fc_high_unit", "amplifier_cutoff"
        ]
        for _id in ids:
            w = self.root_widget.ids.get(_id)
            if w:
                vals.append(w.text.strip())
            else:
                vals.append("")

        # Si hay al menos una no vacía, enviamos como CSV simple
        payload = ",".join(vals)
        if not payload:
            self.show_dialog("Aviso", "Completa al menos un parámetro para enviar.")
            return

        # Ejecutar envió async
        self.run_async_task(self.send_data_async(payload))

    async def send_data_async(self, payload):
        global ble_client, is_connected
        if not ble_client or not ble_client.is_connected:
            self.show_dialog("BLE", "No hay conexión BLE.")
            return
        try:
            await ble_client.write_gatt_char(CHAR_RX, payload.encode("utf-8"))
            self.show_snackbar("Datos enviados: " + payload[:40])
        except Exception as e:
            self.show_dialog("Error al enviar", str(e))

    # ---------------- Show complete graph (Matplotlib) ----------------
    def show_complete_graph(self, *args):
        if not processed_data:
            self.show_dialog("Sin datos", "No hay datos recibidos todavía.")
            return

        # limpiar axes
        self.mat_ax.clear()
        self.mat_ax.plot(processed_data, lw=1)
        self.mat_ax.set_title("Valor AC recibido")
        self.mat_ax.set_xlabel("Índice de muestra")
        self.mat_ax.set_ylabel("uV")
        self.mat_ax.grid(True)
        # reajustar
        self.mat_fig.tight_layout()

        # Si ya existe canvas, reemplazar
        if self.canvas_widget:
            self.root_widget.ids.plot_container.remove_widget(self.canvas_widget)

        self.canvas_widget = FigureCanvasKivyAgg(self.mat_fig)
        self.root_widget.ids.plot_container.add_widget(self.canvas_widget)

    # ---------------- Real-time (KivyGraph) ----------------
    def toggle_realtime(self, *args):
        if self.realtime_enabled:
            self.stop_realtime()
        else:
            self.start_realtime()

    def start_realtime(self):
        if not self.realtime_graph:
            # crear graph
            g = Graph(xlabel='Index', ylabel='uV', y_ticks_major=1000,
                      x_ticks_major=100, y_grid_label=True, x_grid_label=True,
                      xmin=0, xmax=1000, ymin=-7000, ymax=7000, padding=5)
            plot = MeshLinePlot()
            plot.points = []
            g.add_plot(plot)
            self.realtime_graph = g
            self.realtime_plot = plot
            self.root_widget.ids.realtime_container.add_widget(g)

        self.realtime_enabled = True
        self.realtime_interval_event = Clock.schedule_interval(self._update_realtime_plot, 0.1)  # 10 Hz
        self.show_snackbar("Realtime started")

    def stop_realtime(self):
        if self.realtime_interval_event:
            self.realtime_interval_event.cancel()
            self.realtime_interval_event = None
        self.realtime_enabled = False
        self.show_snackbar("Realtime stopped")

    def _update_realtime_plot(self, dt):
        if not processed_data:
            return
        window_seconds = 1.0
        sampling_rate_est = 1300  # como en tu app; ajustar si es necesario
        window_size = int(window_seconds * sampling_rate_est)
        data = processed_data[-window_size:]
        if not data:
            return
        # generar puntos (x,y) con x desde 0..len-1
        pts = [(i, float(v)) for i, v in enumerate(data)]
        # limitar x axis
        self.realtime_plot.points = pts
        # ajustar gráficas xmin/xmax
        g = self.realtime_graph
        g.xmin = 0
        g.xmax = max(1, len(data))

    # ---------------- Save CSV & HTML (Plotly) ----------------
    def on_save_pressed(self, *args):
        entry_widget = self.root_widget.ids.entry_id
        exp_id = entry_widget.text.strip() or "experiment"
        if not processed_data:
            self.show_dialog("Sin datos", "No hay datos para guardar.")
            return
        self.save_csv_and_html(exp_id)

    def save_csv_and_html(self, experiment_id):
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename_csv = f"{experiment_id}_ECG_{timestamp}.csv"
        filename_html = f"{experiment_id}_ECG_{timestamp}.html"

        try:
            # CSV con punto decimal convertido a coma
            # Guardamos en directorio de usuario (cwd)
            p_csv = Path.cwd() / filename_csv
            with open(p_csv, mode="w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f, delimiter=';')
                writer.writerow(["Index", "ECG"])
                for i, v in enumerate(processed_data):
                    v_eu = f"{v:.6f}".replace(".", ",")
                    writer.writerow([i, v_eu])

            # DataFrame y Plotly
            df = pd.DataFrame({"Index": list(range(len(processed_data))), "ECG": processed_data})
            fig = px.line(df, x="Index", y="ECG", title=f"ECG signal - {experiment_id}")
            fig.update_layout(xaxis_title="Index", yaxis_title="Amplitude", template="plotly_white")
            p_html = Path.cwd() / filename_html
            fig.write_html(str(p_html), auto_open=False)

            # Abrir HTML en navegador (si el sistema lo soporta)
            try:
                webbrowser.open(str(p_html))
            except Exception:
                pass

            self.show_dialog("Guardado", f"CSV: {p_csv.name}\nHTML: {p_html.name}")
        except Exception as e:
            self.show_dialog("Error guardando", str(e))

    # ---------------- Utilities ----------------
    def clear_memory(self, *args):
        global processed_data, raw_buffer
        processed_data.clear()
        raw_buffer.clear()
        self.show_snackbar("Memoria limpia (datos procesados + buffer).")

    @mainthread
    def set_status(self, txt):
        self.root_widget.ids.status_lbl.text = "Estado: " + txt

    def show_dialog(self, title, text):
        MDDialog(title=title, text=str(text)).open()

    def show_snackbar(self, text):
        Snackbar(text=str(text)).open()

    # Async scheduling helpers
    def run_async_task(self, coro):
        """
        schedule coroutine on the background async_loop
        """
        return asyncio.run_coroutine_threadsafe(coro, async_loop)

    # Close on stop (try to disconnect)
    def on_stop(self):
        global ble_client
        try:
            if ble_client and ble_client.is_connected:
                fut = asyncio.run_coroutine_threadsafe(ble_client.disconnect(), async_loop)
                fut.result(timeout=5)
        except Exception:
            pass

# --------------- Entrypoint ---------------
if __name__ == "__main__":
    BLEApp().run()
