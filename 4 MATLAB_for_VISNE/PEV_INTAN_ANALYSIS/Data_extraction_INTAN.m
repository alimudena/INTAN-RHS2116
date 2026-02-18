clc
clear

Experimental_days = datetime(['14-01-2026'; '29-01-2026'; '01-01-2001'], 'InputFormat', 'dd-MM-yyyy');
Experimental_types = ["Electrical", "Visual", "Control"];


%%Call experiments definition for each rodent used
PEV04_definition;
PEV06_definition;
PEV07_definition;
%% Funcionalidades aplicadas:

% Hace el plot de la señal adquirida en el experimento que se indica. No
% plotea los tiempos de estimulación. Se utiliza cuando se quiere tratar
% únicamente la parte de adquisición
plot_captured_signal(PEV04.DAY(1).Experiment(1), 0, 60);

% Hace el plot del experimento que se está llamando, tanto la parte de
% adquisición como el de estimulación
plot_channels_INTAN(PEV04.DAY(1).Experiment(1), 0, 60); 
plot_channels_INTAN(PEV04.DAY(1).Experiment(2), 0, 120);
plot_channels_INTAN(PEV04.DAY(1).Experiment(3), 0, 120);