//##############################################################################################################################################################
//### Quellen:                                                                                                                                               ###
//###   - Grundlage des Sketches: https://github.com/SensorsIot/Bluetooth-BLE-on-Arduino-IDE/blob/master/BLE_Proximity_Sensor/BLE_Proximity_Sensor.ino       ###
//###   - Sprachausgabe Alexa: https://www.intelligentes-haus.de/tutorials/smart-home-tutorials/amazon-alexa-text-to-speech-tts-ubers-smart-home-nutzen/     ###
//###   - IDLE Watchdog: https://medium.com/@supotsaeea/esp32-reboot-system-when-watchdog-timeout-4f3536bf17ef                                               ###
//###                                                                                                                                                        ###
//###        Als Bluetooth-Geräte wurden iBeacon (Long Range) benutzt, es funktioniert aber mit jedem Bluetooth Gerät (mit entsprechender Reichweite)        ###
//###                                                                                                                                                        ###
//### Da der ESP32 noch Probleme mit einigen Bibliotheken hat, wurde die IP-Adresse fest per Quellcode gesetzt (WifiManager hat ein Problem im Disconnect!   ###
//###                                                                                                                                                        ###
//###                             Der ESP32 hat ein Problem bezüglich CPU-Überlastung (IDLE) weshalb das Script den Chip neustartet.                         ###
//###                  Dies führt zu einer regelmäßigen Sprachausgabe von XYZ ist zu Hause, weshalb aktuell diese Ausgabe auf "false" steht                  ###
//##############################################################################################################################################################

#include "HTTPClient.h"     //Bibliothek zum Versenden der HTTP-Befehle für Alexa und Homematic-CCU
#include "BLEDevice.h"      //Bibliothek zur Bluetooth-Suche und Erkennung des jeweiligen Gerätes
#include "WiFi.h"           //Bibliothek für die Integration in das WLAN-Netz
#include "esp_system.h"     //Bibliothek zur Generierung eines Watchdog, welcher bei Absturz des Chips einen Neustart anregt

//#########################################################################################################
//###                         In diesem Bereich müssen die Daten angepasst werden                       ###
//#########################################################################################################

//Netzwerk-Einstellungen 
const char* ssid     = "WLAN";          //Name Ihres WLAN-Netzes
const char* password = "PASS";  //Passwort Ihres WLAN-Netzes
IPAddress local_IP(xxx, xxx, xxx, xxx);   //IP-Adresse, welche der ESP32 im WLAN-Netz bekommen soll
IPAddress gateway(xxx, xxx, xxx, xxx);      //Subnetzmaske Ihres WLAN-Netzes
IPAddress subnet(xxx, xxx, xxx, xxx);     //Gateway Ihres WLAN-Netzes

//IP-Adresse der Homematic CCU
String IPCCU = "xxx.xxx.xxx.xxx";

//IP-Adresse des Webservers, auf dem das Alexa-Script läuft
String AlexaWEB = "xxx.xxx.xxx.xxx";

//Alexa Sprachausgabe aktivieren/deaktivieren ### true = aktiv; false = nicht aktiv
bool AlexaActive = false;

//Name des Alexa-Gerätes, welches die Sprachausgabe ausführen soll (zu finden auf der Webseite mit dem Alexa-Skript)
String AlexaDEV = "NameDerAlexa";

//Systemvariable der Homematic Anwesenheitserkennung
String Systemvariable = "Systemvariable";



//Namen der Personen, bei denen die Anwesenheit geprüft wird
//Im Sketch wird die Anwesenheit für 5 Personen geprüft - sollten es mehr sein, so müssen die Variablen hinzugefügt werden
String P1 = "Teilnehmer 1";
String P2 = "Teilnehmer 2";
String P3 = "Teilnehmer 3";
String P4 = "Teilnehmer 4";
String P5 = "Teilnehmer 5";

//MAC-Adressen der Geräte, welche man per Anwesenheitserkennung finden soll
//Im Sketch wird die Anwesenheit für 5 Personen geprüft - sollten es mehr sein, so müssen die MAC-Adressen hinzugefügt werden
String knownAddresses[] = { // MAC Adressen [NUM_BEACON]
"d2:49:8f:c5:59:cc", // iBeacon
"d6:8f:fc:5d:1e:35", // iBeacon
"",
"",
""
};

//#########################################################################################################
//###             Ab hier nichts ändern, es sei denn, es werden mehr als 5 Personen eingebunden         ###
//#########################################################################################################

//Deklaration der Bluetooth-Variablen
static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;

// Rssi - Entfernung wie weit das Gerät erkannt wird. 
const int minRSSI = -120; 

//Status der Homematic Systemvariable als Deklaration
bool VarSTATE = false;

//Variablensteuerung Alexa, damit die Sprachausgabe nur 1x pro Person geschieht während der ESP32 gestartet ist (Loop)
bool Person[] = {false};
//#########################################################################################################
//###                                         Klassen und Funktionen                                    ###
//#########################################################################################################

//Watchdog Reboot wenn sich der ESP32 aufgehangen hat
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule(){
    ets_printf("reboot\n");
    esp_restart_noos();
}


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
}



class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    /**
        Called for each advertising BLE server.
    */

    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("Bluetooth-Gerät mit folgenden Daten gefunden: ");
      Serial.println(advertisedDevice.toString().c_str());
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());

      bool known = false;

      for (int i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
        if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0){
          known = true;
          
          HTTPClient http;  //Deklaration des HTTP-Client über den die HTTP-Befehle an Alexa gesendet wird

          //Im Sketch wird die Anwesenheit für 5 Personen geprüft - sollten es mehr sein, so müssen die Case-Bedingungen hinzugefügt werden
          switch(i)
            {
             case 0:
               // ----- Person 1 erkannt -----
               Serial.println(P1+" ist in der Nähe!");
                  if(!Person[i] && AlexaActive == true){
                    http.begin("http://"+AlexaWEB+"/alexa_tts/alexa.php?device_name="+AlexaDEV+"&text_tts="+P1+"%20ist%20zu%20Hause!");
                  }
                  Person[i] = true;                 
               // --------------------
             break;

             case 1:
               // ----- Person 2 erkannt -----
               Serial.println(P2+" ist in der Nähe!");
                  if(!Person[i] && AlexaActive == true){
                    http.begin("http://"+AlexaWEB+"/alexa_tts/alexa.php?device_name="+AlexaDEV+"&text_tts="+P2+"%20ist%20zu%20Hause!");
                  }
                  Person[i] = true;     
               // --------------------
             break;

             case 2:
               // ----- Person 3 erkannt -----
               Serial.println(P3+" ist in der Nähe!");
                  if(!Person[i] && AlexaActive == true){
                    http.begin("http://"+AlexaWEB+"/alexa_tts/alexa.php?device_name="+AlexaDEV+"&text_tts="+P3+"%20ist%20zu%20Hause!");
                  }
                  Person[i] = true;     
               // --------------------
             break;

             case 3:
               // ----- Person 4 erkannt -----
               Serial.println(P4+" ist in der Nähe!");
                  if(!Person[i] && AlexaActive == true){
                    http.begin("http://"+AlexaWEB+"/alexa_tts/alexa.php?device_name="+AlexaDEV+"&text_tts="+P4+"%20ist%20zu%20Hause!");
                  }
                  Person[i] = true;     
               // --------------------
             break;

             case 4:
               // ----- Person 5 erkannt -----
               Serial.println(P5+" ist in der Nähe!");
                  if(!Person[i] && AlexaActive == true){
                    http.begin("http://"+AlexaWEB+"/alexa_tts/alexa.php?device_name="+AlexaDEV+"&text_tts="+P5+"%20ist%20zu%20Hause!");
                  }
                  Person[i] = true;     
               // --------------------
             break;
          }
                   int httpCode = http.GET(); 
                   http.end(); //Free the resources
        }
      }
      if (known) {

        Serial.print("Gerät gefunden: ");
        Serial.println(advertisedDevice.getRSSI());

        if (advertisedDevice.getRSSI() > minRSSI) deviceFound = true;
        else deviceFound = false;

        Serial.print("MAC Adresse: ");
        Serial.println(pServerAddress->toString().c_str());
        
      }
            
    }
}; // MyAdvertisedDeviceCallbacks


void checkSysVar(){
  HTTPClient http;
    http.begin("http://"+IPCCU+":8181/a.exe?ret=dom.GetObject(ID_SYSTEM_VARIABLES).Get('"+Systemvariable+"').State()");
    int httpCode = http.GET(); 
    String payload = http.getString();
    http.end(); //Free the resources
    if (payload.indexOf("true") >= 0) {
      VarSTATE = true;
    }
    else{
      VarSTATE = false;
    }
    delay(1000);   
};

//#########################################################################################################
//###                                                SETUP                                              ###
//#########################################################################################################

void setup() {

  Serial.begin(115200);
  Serial.println("Starte die Anwesenheitserkennung per ESP32...");

// Watchdog überwachung einstellen

  Serial.println(" - IDLE-Watchdog erstellt");
    timer = timerBegin(0, 80, true); //timer 0, div 80
    timerAttachInterrupt(timer, &resetModule, true);
    timerAlarmWrite(timer, 40000000, false); //set time in us
    timerAlarmEnable(timer); //enable interrupt

/////////////////////////////////////////////////
//             Bluetooth Scanner
/////////////////////////////////////////////////
  
  BLEDevice::init("");

  pClient  = BLEDevice::createClient();
  Serial.println(" - Bluetooth-Client erstellt");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

/////////////////////////////////////////////////
//                WiFi Config
/////////////////////////////////////////////////


  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA konnte nicht konfiguriert werden! Die IP-Einstellungen funktionieren nicht!");
  }

  // Verbindung mit dem WLAN herstellen
  Serial.print("Verbindung wird hergestellt zu: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Ausgabe der lokalen Netzwerkkonfiguration des ESP32

  Serial.println("");
  Serial.println("WLAN-Netz verbunden!");
  Serial.print("IP Adresse: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Adresse: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnetzmaske: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

}

//#########################################################################################################
//###                                           Hauptprogramm                                           ###
//#########################################################################################################

void loop() {

    HTTPClient http;

checkSysVar();
    
/////////////////////////////////////////
//////////// Reboot wenn Watchdog hängt!
/////////////////////////////////////////
 
    timerWrite(timer, 0); //reset timer (feed watchdog)
    long tme = millis();

/////////////////////////////////////////
//////////// Bluetooth Scan
/////////////////////////////////////////

  Serial.println();
  Serial.println("Der Bluetooth-Scan zur Anwesenheitserkennung wird gestartet.....");
  deviceFound = false;
  BLEScanResults scanResults = pBLEScan->start(30);
  if (deviceFound) {
    if (VarSTATE == false){
    Serial.println("Es ist jemand zu Hause!");

        http.begin("http://"+IPCCU+":8181/a.exe?ret=dom.GetObject(ID_SYSTEM_VARIABLES).Get('"+Systemvariable+"').State(true)");
        int httpCode = http.GET(); 
        String payload = http.getString();
        http.end(); //Free the resources
    Serial.println(payload);
    }
    Serial.println("Starte neue Suche!");
  }

  else {
    
    Serial.println("Es ist niemand zu Hause!");

        http.begin("http://"+IPCCU+":8181/a.exe?ret=dom.GetObject(ID_SYSTEM_VARIABLES).Get('"+Systemvariable+"').State(false)");
        int httpCode = http.GET(); 
        http.end(); //Free the resources
        
      for (int i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
        Person[i] = false;        
      }
      
  }

/////////////////////////////////////////
////////////// Ausgabe der Loopzeit (WD)
/////////////////////////////////////////
    Serial.print("Durchlaufzeit dieses Loop = ");
    tme = millis() - tme;
    Serial.println(tme);


} // End of loop
