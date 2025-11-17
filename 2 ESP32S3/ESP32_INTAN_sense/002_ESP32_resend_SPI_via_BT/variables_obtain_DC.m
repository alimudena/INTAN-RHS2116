%%

% === CONFIGURACIÓN ===
ruta_csv = 'M2_AC_10mVpp_80H.csv';   % cambia al nombre real de tu archivo
big_endian = true;         % true = [byte alto, byte bajo]; false = little endian

% === LECTURA ROBUSTA DEL CSV ===
opts = detectImportOptions(ruta_csv, 'Delimiter', ';', 'DecimalSeparator', ',');
T = readtable(ruta_csv, opts);

% Si el tiempo se leyó como índice, lo aseguramos como columna
if ~any(strcmp('Time', T.Properties.VariableNames))
    T.Time = (1:height(T))';
end

% Eliminar columna no deseada
if any(strcmp('SPI_1_2_3_4_', T.Properties.VariableNames))
    T = removevars(T, {'SPI_1_2_3_4_'});
elseif any(strcmp('SPI (1,2,3,4)', T.Properties.VariableNames))
    T = removevars(T, {'SPI (1,2,3,4)'});
end

% Asegurar nombres válidos
T.Properties.VariableNames = matlab.lang.makeValidName(T.Properties.VariableNames);

% === PROCESAMIENTO DE DATOS ===
muestras_tiempo = [];
muestras_valor = [];

for i = 1:height(T)-2
    spi_str = string(T.Var2(i));
    spi_str = erase(spi_str, ["(", ")"]);
    canal = str2double(spi_str);

    if canal ~= 48
        continue
    end

    % Extraer bytes alto y bajo
    byte_alto = str2double(erase(string(T.Var2(i+1)), ["(", ")"]));
    byte_bajo = str2double(erase(string(T.Var2(i+2)), ["(", ")"]));

    % Validar datos
    if any(isnan([canal, byte_alto, byte_bajo]))
        continue
    end

    % Combinar bytes (endianness)
    if big_endian
        valor = bitor(bitshift(byte_alto, 8), byte_bajo);
    else
        valor = bitor(bitshift(byte_bajo, 8), byte_alto);
    end

    tiempo = T.Var1(i);

    muestras_tiempo(end+1,1) = tiempo; %#ok<SAGROW>
    muestras_valor(end+1,1) = valor;  %#ok<SAGROW>
end
%%
% Crear tabla final
df_muestras = table(muestras_tiempo, muestras_valor, ...
    'VariableNames', {'tiempo', 'valor'});


%% === FFT Y GRAFICACIÓN ===
t = muestras_tiempo;
val = muestras_valor;

% Calcular la frecuencia de muestreo (Fs)
Ts = mean(diff(t));   % periodo de muestreo (s)
Fs = 1 / Ts;          % frecuencia de muestreo (Hz)

% Número de puntos
N = length(val);

% Calcular FFT
Y = fft(val);

% Espectro de magnitud
P2 = abs(Y / N);
P1 = P2(1:N/2+1);
P1(2:end-1) = 2*P1(2:end-1);

% Eje de frecuencia
f = Fs * (0:(N/2)) / N;

% === GRAFICAR ===
figure;

subplot(2,1,1);
plot(t, val);
xlabel('Tiempo (s)');
ylabel('Amplitud');
title('Señal en el dominio del tiempo');
ylim([0, 2^10])
grid on;

subplot(2,1,2);
plot(f, P1);
xlabel('Frecuencia (Hz)');
ylabel('|Y(f)|');
xlim([0, 100])
ylim([0, 800])
title('Espectro de amplitud (FFT)');
grid on;

disp('✅ Análisis FFT completado correctamente.');