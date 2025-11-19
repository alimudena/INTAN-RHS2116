
% === CONFIGURACIÓN ===
rutas = ["M2_AC_10mVpp_80Hz.csv"; "M3_AC_10mVpp_120Hz.csv"; "M4_AC_10mVpp_1kHz.csv"; "M5_AC_10mVpp_3kHz.csv"; "M6_AC_5mVpp_30kHz.csv";"M7_AC_5mVpp_80Hz.csv"; "M8_AC_5mVpp_120Hz.csv"; "M9_AC_5mVpp_1kHz.csv"];
ruta_csv = rutas(9);% cambia al nombre real de tu archivo
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
muestras_tiempo_9 = [];
muestras_valor_9 = [];

for i = 1:height(T)-2
    spi_str = string(T.Var2(i));
    spi_str = erase(spi_str, ["(", ")"]);
    canal = str2double(spi_str);

    if canal ~= 49
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

    muestras_tiempo_9(end+1,1) = tiempo; %#ok<SAGROW>
    muestras_valor_9(end+1,1) = valor;  %#ok<SAGROW>
end

% Crear tabla final
df_muestras = table(muestras_tiempo_9, muestras_valor_9, ...
    'VariableNames', {'tiempo', 'valor'});
