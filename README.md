### ARTNET HULA HOOP

*A project developed in collaboration with Fernando Favier.*
<br />

The Artnet Hula Hoop uses NeoPixel strips with 144 LEDs per meter. The LEDs are controlled using an ESP32-C2. All is powered using 3.7V 10440 batteries. When activated, the device searches a designated wifi router. When the connection is established, it then awaits DMX data from a host computer running DMX dedicated software. The protocol managing these communications is **artnet** .
<br />

This project is programmed using Arduino.
## External libraries used:
- [ArtnetWifi v1.5.1](https://github.com/rstephan/ArtnetWifi)
- [Adafruit_NeoPixel v1.11.0](https://github.com/adafruit/Adafruit_NeoPixel)
- [ArduinoOTA v1.0.9](https://github.com/jandrassy/ArduinoOTA) : wireless programming required after circuitry installed within a hula hoop

## Materials List

| HARDWARE                     | NOTES                                   | REFERENCES                                                                                      |
| ---------------------------- | --------------------------------------- | ----------------------------------------------------------------------------------------------- |
| QTPY ESP32-S2                |                                         | [Adafruit](https://learn.adafruit.com/adafruit-qt-py-esp32-s2)                                  |
| XIAO ESP32-C3                | Alternative to QTPY. **not yet tested** | [Seeed Studio](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html)                      |
| UltraFire 10440 3.7V 600 mAh | Same size as AAA battery, Ø10mm         | [batteryupgrade.fr](https://www.batteryupgrade.fr/shopBrowser.php?assortmentProductId=21883872) |

## TODO (and to seriously consider)

- Add a vbus read connection in hardware and software. We need to know when the device is plugged in and charging. This should be followed with a program that indicates the charge.
- Add deepSleep. The device should power down if not in use. A capacitive button may be used to wake it up.
- Add a fuel gauge (ex. MAX17043). For the moment we are using a basic voltage divider to read if the battery is charged or not. This is not very precise.
- Test with different NeoPixel strips. For the moment, we are using a strip that is very high power and fragile (no impermeable protection). There are heat issues using 144 pixels per meter. The device gets buggy after several minutes of use.
- Add a development trigger controlled in the software that may be activated via the controler DMX software. This trigger will ignore demands of deepSleep and/or vbus charging.
- When LEDS are inactive, the chipset inside each NeoPixel still pulls power. This is problematic. DeepSleep will not help preserve battery life if we do not deactivate the NeoPixel chipset. The purpose of DeepSleep is thus defeated. This needs development.
