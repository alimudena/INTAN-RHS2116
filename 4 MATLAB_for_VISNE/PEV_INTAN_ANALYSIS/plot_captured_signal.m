function [] = plot_captured_signal(EXP, MIN_X, MAX_X)

    t1 = EXP.t1;
    CH1 = EXP.CH1;

    t2 = EXP.t2;
    CH2 = EXP.CH2;

    figure
    plot(t1, CH1, 'DisplayName', 'CH1')
    hold on
    grid on
    plot(t2, CH2, 'DisplayName', 'CH1')
    ylabel('(\muV)', 'FontSize', 13)
    xlabel('Tiempo (s)', 'FontSize', 13)
    title('EEG without stimulation pattern', 'FontSize', 13)
    
    legend('CH1', 'CH2', 'FontSize', 13)
    legend
    xlim([MIN_X, MAX_X])
end