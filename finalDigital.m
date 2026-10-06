% Limpiar el workspace y la ventana de comandos
clear;
clc;
close all;

senal = 'E:\.Biónica\TT\AdquirirSeñal\emg_VastusLat.csv';
fs = 5000; % Fs configurada en Arduino

data_loaded = readmatrix(senal, 'Delimiter', ',', 'NumHeaderLines', 1);
tiempo = data_loaded(:, 1);  
data = data_loaded(:, 2); 
L = length(data);


%Transformada de Fourier (FFT) 
Y = fft(data); 
P2b = abs(Y / L);           % Espectro de doble lado, magnitud normalizada por L
P1b = P2b(1:L/2+1);         % Tomar solo la primera mitad (positiva)
P1b(2:end-1) = 2 * P1b(2:end-1); % Duplicar la amplitud para el espectro unilateral (excepto DC y Nyquist)
fb = fs * (0:(L/2)) / L;

% Normalización
meansig = mean(data);
nomedia = data - meansig; 
minval = min(nomedia);
sig = nomedia + abs(minval); 
maxval = max(sig);
normsig = sig / maxval; 

%Media movil
windowSize = 400;
b_ma = (1/windowSize) * ones(1, windowSize); 
a_ma = 1;                                     
sigmedia = filter(b_ma, a_ma, normsig);


figure
% Señal Original en el Tiempo
subplot(2,1,1);
plot(tiempo, data);
xlabel('Tiempo (ms)');
ylabel('Amplitud');
title('Señal Original en el Tiempo');
grid on;


% FFT
subplot(2,1,2);
plot(fb, P1b); 
xlabel('Frecuencia (Hz)');
ylabel('Amplitud');
title('Espectro de Frecuencia de la Señal Filtrada');
grid on;

figure

% Señal Normalizada
subplot(2,1,1);
plot(tiempo, normsig);
xlabel('Tiempo (ms)');
ylabel('Amplitud Normalizada');
title('Señal Normalizada');
grid on;
ylim([0 1]); 

% Señal Normalizada con su Envolvente (Media Móvil)
subplot(2,1,2); 
plot(tiempo, normsig, 'b'); 
hold on; 
plot(tiempo, sigmedia, 'm', 'LineWidth', 1.5); 
hold off; 
xlabel('Tiempo (ms)');
ylabel('Amplitud Normalizada');
title('Señal Normalizada y Envolvente (Media Móvil)');
legend('Señal Normalizada', 'Envolvente (Media Móvil)', 'Location', 'best');
grid on;
ylim([0 1]); 