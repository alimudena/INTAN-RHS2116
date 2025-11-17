import pandas as pd
import matplotlib.pyplot as plt

# === 1. Leer el archivo CSV ===
# Cambia "datos.csv" por el nombre de tu archivo real
archivo_csv = "datos_ECG_20251015_183310.csv"

# Algunas veces los CSV tienen caracteres raros, por eso usamos encoding='utf-8-sig'
df = pd.read_csv(archivo_csv, encoding='utf-8-sig')

# === 2. Mostrar primeras filas para verificar ===
print(df.head())

# === 3. Graficar ===
plt.figure(figsize=(10, 5))
plt.plot(df['Indice'], df['ECG'], label='Señal ECG', color='blue')

plt.title("Señal ECG")
plt.xlabel("Índice")
plt.ylabel("Amplitud")
plt.legend()
plt.grid(True)

# === 4. Activar modo interactivo ===
# Esto abre una ventana con botones de zoom, mover, guardar, etc.
plt.tight_layout()
plt.show()
