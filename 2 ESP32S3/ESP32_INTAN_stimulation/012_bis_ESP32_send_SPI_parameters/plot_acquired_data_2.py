import pandas as pd
import plotly.express as px

df = pd.read_csv("datos_ECG_20251015_183310.csv", encoding='utf-8-sig')

fig = px.line(df, x='Indice', y='ECG', title="Señal ECG Interactiva")
fig.update_layout(xaxis_title="Índice", yaxis_title="Amplitud")
fig.show()
