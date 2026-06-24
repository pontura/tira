# Pinout - LED Game Console

## Master: ESP32 DevKit

| GPIO | Componente        | Detalle                  |
|------|-------------------|--------------------------|
| 5    | Tira WS2812B      | Data in                  |
| 18   | Buzzer pasivo     | Signal                   |
| 21   | Display SH1106    | SDA (I2C)                |
| 22   | Display SH1106    | SCL (I2C)                |

## Alimentación

| Componente     | VCC       | GND                        |
|----------------|-----------|----------------------------|
| Tira WS2812B   | 5V externo (fuente mínimo 5A) | GND fuente + GND ESP32 |
| Display SH1106 | 3.3V ESP32 | GND ESP32                 |
| ESP32 DevKit   | USB / VIN | —                          |

---
*Se irá actualizando a medida que se agreguen componentes.*

## Joysticks: ESP32-C3 SuperMini (x2)

| GPIO | Componente     | Detalle                        |
|------|----------------|--------------------------------|
| 7    | Botón disparar | INPUT_PULLUP, activo LOW       |
| 9    | Botón color    | INPUT_PULLUP, activo LOW       |
| 4    | Buzzer pasivo  | Signal                         |
| —    | ESP-NOW        | WiFi integrado, sin pin extra  |

## Alimentación joysticks

| Componente       | Detalle                                         |
|------------------|-------------------------------------------------|
| Batería          | 18650 → TP4056 BAT+ / BAT-                     |
| TP4056 → ESP32   | OUT+ → pin 5V / OUT- → GND                     |
| Carga            | USB-C del TP4056                                |
| Encendido        | Interruptor en cable OUT+ entre TP4056 y ESP32 |