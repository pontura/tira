# Pinout - LED Game Console

## Master: ESP32 Dev Module

| GPIO | Componente        | Detalle                  |
|------|-------------------|--------------------------|
| 25   | Tira WS2812B      | Data in                                         |
| 14   | Buzzer pasivo     | Signal                                          |
| 21   | Display SH1106    | SDA — WIRE_SDA nativo                           |
| 22   | Display SH1106    | SCL — WIRE_SCL nativo                           |
| 13   | HW-504 SW         | Joystick menú — INPUT_PULLUP (no usado para selección)        |
| 34   | HW-504 VRy        | Eje Y joystick menú — ADC, input only (scroll arriba/abajo)   |
| 35   | HW-504 VRx        | Eje X joystick menú — ADC, input only (derecha = entrar)      |

### HW-504 Joystick local (menú)

| Pin HW-504 | Conectar a  | Detalle                                        |
|------------|-------------|------------------------------------------------|
| +5V        | 3.3V ESP32  | ⚠️ Usar 3.3V, no 5V (ADC del ESP32 max 3.3V) |
| GND        | GND ESP32   |                                                |
| VRy        | GPIO 34     | Scroll vertical del menú                       |
| VRx        | GPIO 35     | Mover derecha → entrar al juego seleccionado   |
| SW         | GPIO 13     | No usado para selección (reservado)            |

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
| 3    | MPU-6050       | SDA (I2C)                      |
| 4    | MPU-6050       | SCL (I2C)                      |
| 7    | Buzzer pasivo  | Signal                         |
| 8    | Botón disparar | INPUT_PULLUP, activo LOW       |
| 10   | Botón color    | INPUT_PULLUP, activo LOW       |
| —    | ESP-NOW        | WiFi integrado, sin pin extra  |

### ESP-NOW — MAC del master

Master y joysticks en modo `WIFI_STA`, canal 1 fijo. El joystick apunta al **STA MAC** del master:

| Interface | MAC               |
|-----------|-------------------|
| **STA MAC** (usar en joystick) | **70:4B:CA:21:EA:18** |

> Confirmar en serial del master al arrancar: `Master MAC: XX:XX:XX:XX:XX:XX`
> Si el master se re-flashea con una placa diferente, hay que actualizar `masterMAC[]` en el joystick.

### ⚠️ Advertencia RF — Geekble Mini ESP32-C3

El Geekble Mini tiene plano de masa pequeño y antena de traza muy sensible:
- **No conectar cables a pines no usados** (actúan como antenas de ruido y degradan el TX WiFi)
- En particular GPIO20 y GPIO21 (UART TX/RX): si no se usan, dejarlos sin conectar
- Mantener cables alejados del extremo del board opuesto al USB-C (zona de antena)

### MPU-6050

> **Nota para el PCB:** montar el chip rotado 90° respecto al eje largo del control, de forma que el eje X del MPU quede horizontal (izquierda-derecha) cuando el control se sostiene vertical. Así el acelerómetro detecta el tilt correctamente sin necesidad de giroscopio ni drift. El MPU se usa con calibración de fábrica (sin offset manual).

| Pin MPU | Conectar a         | Detalle                              |
|---------|--------------------|--------------------------------------|
| VCC     | 3.3V ESP32-C3      |                                      |
| GND     | GND ESP32-C3       |                                      |
| SDA     | GPIO 3             |                                      |
| SCL     | GPIO 4             |                                      |
| INT     | —                  | No usado                             |
| AD0     | GND                | Dirección I2C = 0x68 (por defecto)   |
| INT     | —                  | No usado                             |

## Alimentación joysticks

| Componente       | Detalle                                         |
|------------------|-------------------------------------------------|
| Batería          | 18650 → TP4056 BAT+ / BAT-                     |
| TP4056 → ESP32   | OUT+ → pin 5V / OUT- → GND                     |
| Carga            | USB-C del TP4056                                |
| Encendido        | Interruptor en cable OUT+ entre TP4056 y ESP32 |