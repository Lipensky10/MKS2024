

data = csvread("ntc.csv");

U1 = 3.3; %Uin [V]
R5 = 10; %R1 [kohm]

Temperatures = data(:,1);
NTC = data(:,2);
ADC = (NTC./(NTC + R5)).*(1024);

p = polyfit(ADC, Temperatures, 10);
ADC2 = 0:1023;
t2 = round(polyval(p,ADC2),1);

% Plotting the data
figure;
plot(ADC,Temperatures, 'o', 'DisplayName', 'Temperature vs ADC');
xlabel('ADC Value');
ylabel('Temperature (°C)');
title('Temperature vs ADC Value');
grid on;

hold on

plot(ADC2,t2, 'r');

lookup_table = zeros(1024, 1); %Look up tabulka
for i = 0:1023
    lookup_table(i + 1) = polyval(p, i); % Store fitted temperature
end

%Store the array values in a suitable format for C language
dlmwrite('data.dlm', t2 * 10, ','); % Save in tenths of a degree

