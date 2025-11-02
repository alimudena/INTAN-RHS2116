import pandas as pd
import plotly.express as px

# Lee el CSV (ajustando la codificación)
df = pd.read_csv("datos_ECG_20251015_183310.csv", encoding='utf-8-sig')

# Normaliza los nombres de columnas por si hay caracteres extraños
df.columns = df.columns.str.strip().str.replace('�', 'Í', regex=False)

# Crea la figura
fig = px.line(df, x='Indice', y='ECG', title="Señal ECG Interactiva")

# Muestra en navegador
fig.write_html("grafica_ecg.html", auto_open=True)
