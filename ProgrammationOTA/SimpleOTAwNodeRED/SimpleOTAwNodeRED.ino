/*
* Code par Julfi fondateur de Weldybox
* Version : 12/01/2019
* 
* Code d'interaction avec wemos d1 mini.
* Intéraction MQTT (publish subscribe) Broker MQTT
* Programmation OTA sur commande avec WebServeur
* Intéraction MQTT et OTA contrôlable depuis Node-RED
* 
* Ce code fournis tous les X temps.
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/*
 * On définit le SSID et la clé WPA.
 */

char tempX[5];
bool automatic = false;
    
float tempMAX; //Définition la variable qui détermine la valeur max avant trigger

const char* ssid = "Livebox-8E6A";
const char* password = "";
const char* mqtt_server = "192.168.1.222";

/*
 * Définition des différents objets
 */
Adafruit_BME280 bme; //Objet bme pour gérer la librairie BME
ESP8266WebServer server; //Création de l'objet sever qui sera notre serveur Web pour OTA
WiFiClient espClient; //Création du client WIFI
PubSubClient client(espClient); //Création de l'objet client pour gérer les messages MQTT

/*
 * Création des variables de gestions des delay
 */
unsigned long current = 0; //Nombre de secondes écoulés depuis le début
unsigned long previousLED = 0; //Dernier indicateur de temps concernant le changemenet d'état de la LED
unsigned long previousBME = 0; //Dernier indicateur de temps concernant le changement d'état du BME
int intervalLED = 5000; //Interval de changement d'état de la LED
const long intervalBME = 900000; //Interval de changement d'état du BME

bool prog = true; //Définir la variable programmation true
uint16_t time_elapsed = 0; //Définir le temps écoulé pour la programmation OTA
int LEDv = D1; 
int LEDr = D2;
char buf[20]; //définition du buffer

void setup() {
  /*
   * Début du Wire qui permet d'utiliser la connexion I2C avec le BME
   */
  Wire.begin(2, 12);
  Wire.setClock(100000);

  Serial.begin(115200);
  pinMode(LEDv, OUTPUT);
  pinMode(LEDr, OUTPUT);
  digitalWrite(LEDr, LOW);

  /*
   * Initialisation du Wifi
   */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

 /*
  * Initialisation du broker MQTT
  */
  client.setServer(mqtt_server, 2222);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("BMESensor")) { //On se connecte au broker avec l'identifiant BMEsensor
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }

    /*
     * Initialisation du lien I2C avec le BME
     */
    bool status;
    status = bme.begin(0x76);//L'adresse du couple de connecteur I2C
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
    
    /*
     * Initialisation d'OTA
     */
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  }
  
  /*
   * Si l'event /prog est detecté, on lance la programmation OTA
   */
  server.on("/prog",[](){
  server.send(200,"text/plain", "programming...");
  prog = true; //On définit la variable programmation à vrai
  time_elapsed = 0; //Le temps écoulé à 0
  OTAprog(); //On lance la fonction de programmation OTA
  });
  
  server.begin(); //On lance le serveur pour écouter l'arrivé d'éventuelle requête HTTP
  
  Subinit(); //Subinit définit tous les topic dans lesquels l'ESP doit s'abonner

  client.publish("/BME/prog/ack", "ok");
 
}

void Subinit(){
  client.subscribe("/BME");
  client.subscribe("/BME/vmax");
}

/*
 * Fonction reconnexion au broker MQTT
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("BMESensor")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(10);
    }
  }
}

/*
 * Fonction callback qui écoute l'arrivée d'éventuelles messages MQTT
 */
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i]; //définition du message recus au topic X
}
 
  if (strcmp(topic,"/BME")==0){ //si le message reçus sur le topic /BME est ok
    if (msg == "on"){
      for(int i = 0; i<10; i++){
        digitalWrite(LEDv, !digitalRead(LEDv)); //On fait clignoter les LED
        delay(100);
      }
      automatic = true;
      //Serial.println(automatic);
    }else{
      automatic = false;
      //Serial.println(automatic);
    }
    Serial.println(msg);
  }
  else if(strcmp(topic,"/BME/vmax")==0){ //Si un message est reçus sur le topic /BME/Vmax
    tempMAX = msg.toFloat(); //On change la valeure de la variable Vmax
    delay(10);
    Serial.println(tempMAX);
  }
}

/*
 * Fonction client qui rassemble l'ensemble des fonctions d'écoutes et de managment.
 */
void loop() {
  if (!client.connected()) {
    reconnect(); //Si le client n'est pas connecté alors on se reconnect
  }
  server.handleClient(); //On écoute l'arrivée de nouveaux messages du Webserver
  client.loop(); //On écoute l'arrivée de message MQTT
  ManageLED(); //On regarde s'il faut clignoter les LED
  ManageBME(); //On regarde s'il faut envoyer un message MQTT vers le broker
  Chauffage();
}

void Chauffage (){
  if (automatic){
    if (bme.readTemperature() < tempMAX){
      digitalWrite(LEDr, HIGH);
      delay(10);
    }else{
      digitalWrite(LEDr, LOW);
      delay(10);
    }
  }else{
    digitalWrite(LEDr, LOW);  
    delay(10);
  }
}

/*
 * Fonction qui regarde s'il faut envoyer une donnée au broker
 */
void ManageBME (){
  current = millis();
  if (current - previousBME >= intervalBME){ //Si l'interval de temps correspond au temps entre previous et current
    previousBME = current;
    float hum = bme.readHumidity();
    float temp = bme.readTemperature();
    char* msgTemp = dtostrf(temp, 4, 2, tempX);
    client.publish("/BME/temp", msgTemp); //On convertit puis on envoie les données au broker
    delay(10);

    Serial.println("msg BME envoyé");
    Serial.print("température = ");
    Serial.println(temp);
    Serial.println();
    
    //char tempX[5];
    char* msgHum = dtostrf(hum, 4, 2, tempX);
    client.publish("/BME/hum", msgHum);

    Serial.println("msg BME envoyé");
    Serial.print("humidity = ");
    Serial.println(hum);
    Serial.println();
    delay(10);
  }
}

/*
 * Fonction qui détermine s'il faut faire clignoter la LED
 */
void ManageLED (){
  current = millis();
  if (current - previousLED >= intervalLED){ //Si le code est lancé depuis 1 seconde
    previousLED = current;
    digitalWrite(LEDv, !digitalRead(LEDv)); //Alors on fait clignoter la LED
    delay(10);
  }
}

/*
 * Fonction qui envoie un accusé de reception une fois l'interval du programmation OTA est terminé
 */
void SendDataACK () {
  if (!client.connected()) {
    reconnect();
  }
  client.publish("/BME/prog/ack", "ok");
  delay(100);
  Subinit();
}

/*
 * Fonction qui met le programme en pause pendant 15 secondes et écoute l'arrivé de donnée.
 */
void OTAprog(){
  if(prog)
  {
    uint16_t time_start = millis();
    while(time_elapsed < 15000) //Tant qu'il n'y a pas eu 15 secondes d'écoulé
    {
      digitalWrite(LEDv, HIGH); //La LED reste allumé
      ArduinoOTA.handle(); //On écoute l'arrivé d'éventuel de code
      time_elapsed = millis()-time_start;
      delay(10);
    }
    delay(10);
    SendDataACK();
    digitalWrite(LEDv, LOW); //On éteint la LED
    prog = false; //On définie la variable de programmation à false
  } 
}
