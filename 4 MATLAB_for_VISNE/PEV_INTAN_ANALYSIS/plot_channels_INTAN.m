function [t_ON, t_OFF] = plot_channels_INTAN(EXP, MIN_X, MAX_X)

    t1 = EXP.t1;
    CH1 = EXP.CH1;

    t2 = EXP.t2;
    CH2 = EXP.CH2;

    idx_events_ON  = EXP.stim_ON;
    idx_events_OFF = EXP.stim_OFF;

    figure
    plot(t1, CH1, 'DisplayName', 'CH1')
    hold on
    grid on
    plot(t2, CH2, 'DisplayName', 'CH1')
    ylabel('(\muV)', 'FontSize', 13)
    xlabel('Tiempo (s)', 'FontSize', 13)
    title('EEG with stimulation pattern', 'FontSize', 13)
    
    t_ON  = t1(idx_events_ON);
    t_OFF = t1(idx_events_OFF);
    
    xline(t_ON, 'k', 'LineWidth', 1.2);
    % xline(t_OFF, 'r', 'LineWidth', 1.2);
    
    legend('CH1', 'CH2','Stim', 'FontSize', 13)
    legend
    xlim([MIN_X, MAX_X])
end