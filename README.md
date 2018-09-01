# ESP32_Bluetooth_Presence_Homematic
Es handelt sich um eine Anwesenheitserkennung per Bluetooth für die Homematic, welche auf eine Systemvariable der CCU zugreift!
Bei dicken Wänden/schlechter Reichweite bietet es sich an, eine weitere Systemvariable hinzuzufügen und so einen weiteren ESP32 einzubinden.

Als Bluetooth-Geräte wurden iBeacon (Long Range) benutzt, es funktioniert aber mit jedem Bluetooth Gerät (mit entsprechender Reichweite)

Da der ESP32 ein Problem mit CPU - IDLE hat wurde ein Watchdog dem Script hinzugefügt, der bei einem IDLE-Trace nach wenigen Sekunden den ESP32 neustartet!

Quellen:
- Grundlage des Sketches: https://github.com/SensorsIot/Bluetooth-BLE-on-Arduino-IDE/blob/master/BLE_Proximity_Sensor/BLE_Proximity_Sensor.ino
- Sprachausgabe Alexa: https://www.intelligentes-haus.de/tutorials/smart-home-tutorials/amazon-alexa-text-to-speech-tts-ubers-smart-home-nutzen/
- IDLE Watchdog: https://medium.com/@supotsaeea/esp32-reboot-system-when-watchdog-timeout-4f3536bf17ef


Der ESP32 hat ein Problem bezüglich CPU-Überlastung (IDLE) weshalb das Script den Chip neustartet.
Dies führt zu einer regelmäßigen Sprachausgabe von XYZ ist zu Hause, weshalb aktuell diese Ausgabe auf "false" steht
