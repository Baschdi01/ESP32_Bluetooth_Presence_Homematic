# ESP32_Bluetooth_Presence_Homematic
Es handelt sich um eine Anwesenheitserkennung per Bluetooth für die Homematic.

Es wurden zur Erkennung iBeacons verwendet.

Da der ESP32 ein Problem mit CPU - IDLE hat wurde ein Watchdog dem Script hinzugefügt, der bei einem IDLE-Trace nach wenigen Sekunden den ESP32 neustartet!
