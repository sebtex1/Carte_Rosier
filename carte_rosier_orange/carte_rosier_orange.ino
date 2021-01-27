#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
// Définir par son réseau
const char *ssid     = "Petibonum09";
const char *password = "Piedecochon-09";
// Server port
WiFiServer server(80);
// Défini le NTP client pour get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String currentDate;
String formattedTime;

#include <dht.h>
dht DHT;
// PIN du PCS humidité et temperature
#define DHT11_PIN D1
String messageDHT11;

// PINS du PCS humidité du sol
int pinAnalogKY71=A0;
int pinDigitalKY71=D0;
int valAnalogKY71;
int mapAnalogKY71;
int valDigitalKY71;
String messageKY71;

// PIN du PCS lumière 
#define VIN 3.3
#define R 10000
int pinDigitalHALJIA = D2;
int valueLux; // Valeur digital du PCS lux
String etatLux;
String messageLux;

// PIN du PCS niveau d'eau
int pinDigitalWater=D3;
int waterLevel;

// PIN pompe
int boutonPompe = D4;

void setup() {
  pinMode(boutonPompe, OUTPUT);
  digitalWrite(boutonPompe, LOW);
  
  Serial.begin(9600);
  Serial.println();

  // Connection au Wi-Fi
  Serial.print("Connexion à ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Adresse IP sur le wifi
  Serial.print("Connecté, adresse IP:");
  Serial.println(WiFi.localIP());

  // Démarrage du server
  server.begin();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
  
  Serial.println("Initialisation DHT11");

  Serial.println("Initialisation KY71");
  pinMode(pinAnalogKY71, INPUT);
  pinMode(pinDigitalKY71, INPUT);

  Serial.println("Initialisation HALJIA");
  pinMode(pinDigitalHALJIA, INPUT);

  Serial.println("Initialisation CQRobot");
  pinMode(pinDigitalWater, INPUT);


}

void loop() {
  WiFiClient client = server.available();
  
  // Ne rien faire si pas de client
  if(!client) {
    return;
  }

  // Attendre le client
  while(!client.available()) {
    timeClient.update();

    unsigned long epochTime = timeClient.getEpochTime();  
    formattedTime = timeClient.getFormattedTime();
    
    //Get a time structure
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;
    //Print complete date:
    currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
    
    // Données DHT11
    int chk = DHT.read11(DHT11_PIN);
    // Empêche les valeurs d'être à 0
    while (DHT.temperature == 0 or DHT.humidity == 0) {
      int chk = DHT.read11(DHT11_PIN);
    }    
  
    // Données KY71
    valAnalogKY71=analogRead(pinAnalogKY71);
    valDigitalKY71=digitalRead(pinDigitalKY71);
    // La valeur analog peut permettre d'être précis sur le taux d'humidité
    // Si la valeur digital est set à 0 alors le sol est suffisamment humide
    mapAnalogKY71 = map(valAnalogKY71, 0, 1023, 0, 100);
    if (valDigitalKY71 == 0) {
      messageKY71="Plante hydratee ";
    } else {
      messageKY71="Plante en manque d'eau ";
    }
    //Serial.println(messageKY71);

    if (mapAnalogKY71 < 45) {
      Serial.println("début de pompage");
      digitalWrite(boutonPompe, HIGH);
      delay(1000);
      digitalWrite(boutonPompe, LOW);
      Serial.println("fin de pompage");
    }
    
    // Données HALJIA
    valueLux = digitalRead(pinDigitalHALJIA);
    if (valueLux == 0) {
      etatLux = "Lumiere allumee";
      messageLux = "Eteindre la lumiere";
    } else {
      etatLux = "Lumiere eteinte";
      messageLux = "Allumer la lumiere";
    }
  
    // Données CQRobot
    waterLevel=digitalRead(pinDigitalWater);
    Serial.println(waterLevel);
    if (waterLevel == 0) {
      Serial.println("Le bac n'a plus d'eau");
    } else {
      Serial.println("Le bac contient de l'eau");
    }
    
    // Démarcation dans le COM
    Serial.println("Appel");
    //Serial.println("----------------------------------");
   }

  // Nouveau client avec requête
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Affichage
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");

  client.println("<!DOCTYPE HTML>");
  client.println("<html>"); 
  client.println("<h1>Rosier orange :</h1>");
  client.print(messageKY71);
  client.print(valAnalogKY71);
  client.print("/1024");
  client.print(" (");
  client.print(mapAnalogKY71);
  client.print("%)");
  client.println("<br><br>");
  client.print("Date: ");
  client.print(currentDate);
  client.println("<br><br>");
  client.print("Heure: ");
  client.print(formattedTime); 
  client.println("<br><br>");
  client.print("Temperature: ");
  client.println(DHT.temperature);
  client.println("<br><br>");
  client.print("Humidite: ");
  client.println(DHT.humidity);
  client.println("<br><br>");
  client.print("Lumiere: ");
  client.print(etatLux);
  client.println("<br><br>");
  client.println("!!! à venir !!!");
  client.println("<br><br>");
  client.println("Changer la luminosité: ");
  client.println("<a href=\"/LED=ON\">" + messageLux + "</a><br>");
  client.println("</html>");

  Serial.println("");
}
