# ESP32 Potentiometer Web Server

This project starts an ESP32 access point and serves a web page that shows the current potentiometer reading and a knee joint animation based on the sensor value.

## Wiring

- Potentiometer `VCC` -> `3V3`
- Potentiometer `GND` -> `GND`
- Potentiometer signal -> `GPIO34`

## Important

On a standard ESP32, `GPIO23` is **not** an analog input pin, so `analogRead(23)` will not work properly. Move the potentiometer signal wire to an ADC pin such as `34`, `35`, `32`, or `33`.

## Upload

1. Run `pio run -t upload`.
2. Open Serial Monitor at `115200` baud if you want the boot log.
3. Join the open Wi-Fi network `ESP32-Pot-Reader`.
4. Open `http://192.168.4.1` on your phone.
