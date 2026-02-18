function [CH1, CH2, stim_ON, stim_OFF, t1, t2, idx_events_ON, idx_events_OFF] = extract_channel_and_stimulation(datos_bio,datos_STIM)
    arguments (Input)
        datos_bio
        datos_STIM
    end
    
    arguments (Output)
        CH1
        CH2
        stim_ON
        stim_OFF
        t1
        t2
        idx_events_ON
        idx_events_OFF
    end
    
    CH1 = str2double(strrep(datos_bio.Ch1_val_uV, ',', '.'));
    CH2 = str2double(strrep(datos_bio.Ch2_val_uV, ',', '.'));
    
    stim_ON  = datos_STIM.ch1_idx(strcmp(datos_STIM.Event,'ON'));
    stim_OFF = datos_STIM.ch1_idx(strcmp(datos_STIM.Event,'OFF'));


    % Identificar NaNs en CH2
    idxValid = ~isnan(CH2);
    
    % Aplicar máscara a TODO lo que esté indexado por muestras
    CH2 = CH2(idxValid);
    CH1 = CH1(idxValid);
    
    t1 = (max(datos_bio.Time_us)- min(datos_bio.Time_us)) * 1e-6; % segundos
    f1 = length(CH1)/t1;
    % f1 = 10100;
    t1 = (1:1:length(CH1))/f1;
    
    t2 = (max(datos_bio.Time_us)- min(datos_bio.Time_us)) * 1e-6; % segundos
    f2 = length(CH2)/t2;
    % f2 = 10100;
    t2 = (1:1:length(CH1))/f2;
    
    ts_events_OFF = datos_STIM.ts_us(strcmp(datos_STIM.Event,'OFF'));
    ts_events_ON = datos_STIM.ts_us(strcmp(datos_STIM.Event,'ON'));
    ts_samples = datos_bio.Time_us;
    
    
    % Si usas timestamps
    ts_samples = ts_samples(idxValid);
    
    % Mapear ts_us -> índice de muestra más cercana
    idx_events_OFF = interp1(ts_samples, ...
                         1:numel(ts_samples), ...
                         ts_events_OFF, ...
                         'nearest', 'extrap');
    
    idx_events_ON = interp1(ts_samples, ...
                         1:numel(ts_samples), ...
                         ts_events_ON, ...
                         'nearest', 'extrap');

end