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
