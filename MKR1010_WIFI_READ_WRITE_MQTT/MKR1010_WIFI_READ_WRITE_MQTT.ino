// *** VERSIONE COMPLETA DEL CODICE ADATTATA PER PUBBLICARE MQTT E LEGGERE STUFA ***
// Pubblica il datagramma DATAGRAM (255 byte) sul topic "klover"
// usando l'account MQTT "arduinomkr1000"

#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "DHT.h"

// -----------------------------------------------------------------------------
// CONFIGURAZIONE WI-FI
// -----------------------------------------------------------------------------
const char* ssid = "BabolandWifi";
const char* password = "";

// IP statico
IPAddress localIP(192, 168, 4, 3);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

// -----------------------------------------------------------------------------
// CONFIGURAZIONE MQTT (Mosquitto)
// -----------------------------------------------------------------------------
const char* mqtt_server   = "192.168.4.2";
const int   mqtt_port     = 1883;
const char* mqtt_user     = "arduinomkr1000";
const char* mqtt_password = "arduinomkr1000";
const char* mqtt_topic    = "klover";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// -----------------------------------------------------------------------------
// CONFIGURAZIONE LETTURA STUFA + SENSORI
// -----------------------------------------------------------------------------
const int READING_TIME = 30 * 1000;
const int DATAGRAM_SIZE = 255;
byte DATAGRAM[DATAGRAM_SIZE];
const int BUFFER_SIZE = 2;
byte readBuf[4];

#define DHTPIN 6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------
#define LED_WIFI_ON   0
#define LED_WIFI_OFF  1
#define LED_SERIAL_OK 2
#define LED_SERIAL_ERR 3
#define LED_MQTT_ON   4
#define LED_MQTT_OFF  5

int wifiStatus = WL_IDLE_STATUS;

// -----------------------------------------------------------------------------
// FUNZIONI LED
// -----------------------------------------------------------------------------
void setLedWifiOff() { digitalWrite(LED_WIFI_OFF, HIGH); digitalWrite(LED_WIFI_ON, LOW); }
void setLedWifiOn()  { digitalWrite(LED_WIFI_ON, HIGH); digitalWrite(LED_WIFI_OFF, LOW); }
void setLedMQTTOff() { digitalWrite(LED_MQTT_OFF, HIGH); digitalWrite(LED_MQTT_ON, LOW); }
void setLedMQTTOn()  { digitalWrite(LED_MQTT_ON, HIGH); digitalWrite(LED_MQTT_OFF, LOW); }

// -----------------------------------------------------------------------------
// CONNESSIONE WIFI
// -----------------------------------------------------------------------------
void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connessione a WiFi...");
    WiFi.begin(ssid, password);
    WiFi.config(localIP, gateway, subnet, dns);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 10) { delay(1000); attempt++; }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connesso!");
      Serial.print("IP: "); Serial.println(WiFi.localIP());
      setLedWifiOn();
    } else {
      Serial.println("Fallito, ritento...");
      setLedWifiOff();
    }
  }
}

// -----------------------------------------------------------------------------
// CONNESSIONE MQTT
// -----------------------------------------------------------------------------
void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connessione a MQTT...");
    if (mqttClient.connect("arduino_mkr1000_client", mqtt_user, mqtt_password)) { setLedMQTTOn(); Serial.println("connesso!"); }
    else { setLedMQTTOff(); Serial.print("Errore MQTT rc="); Serial.println(mqttClient.state()); delay(2000); }
  }
}

// -----------------------------------------------------------------------------
// SERIAL: invia datagramma e legge 4 byte
// -----------------------------------------------------------------------------
bool sendAndReceiveSerial(byte* byteArray, unsigned long timeout = 1000) {
  while (Serial1.available() > 0) Serial1.read();
  Serial1.write(byteArray, BUFFER_SIZE);
  delay(100);
  unsigned long startTime = millis();
  int index = 0;
  while (millis() - startTime < timeout) {
    while (Serial1.available() > 0) { readBuf[index++] = Serial1.read(); }
    if (index == 4) break;
  }
  return index == 4;
}

// -----------------------------------------------------------------------------
// LETTURA COMPLETA STUFA 0x00 → 0xFF
// -----------------------------------------------------------------------------
bool readStoveStatus = false;
byte DUMMY_REQUEST[BUFFER_SIZE] = {0x00, 0x00};
void readStove() {
  readStoveStatus = true;
  for (short i = 0; i < 255; i++) {
    DUMMY_REQUEST[1] = i;
    if (sendAndReceiveSerial(DUMMY_REQUEST)) {
      DATAGRAM[i] = readBuf[3];
      digitalWrite(LED_SERIAL_OK, HIGH); digitalWrite(LED_SERIAL_ERR, LOW);
    } else {
      digitalWrite(LED_SERIAL_OK, LOW); digitalWrite(LED_SERIAL_ERR, HIGH);
      readStoveStatus = false;
    }
  }
  digitalWrite(LED_SERIAL_OK, LOW);
}

// -----------------------------------------------------------------------------
// LETTURA SENSORI DHT
// -----------------------------------------------------------------------------
float h, t;
byte hBytes[4], tBytes[4];
void readSensors() {
  h = dht.readHumidity(); t = dht.readTemperature();
  memcpy(hBytes, &h, sizeof(h)); memcpy(tBytes, &t, sizeof(t));
  for (int i = 0; i < 4; i++) { DATAGRAM[246+i]=tBytes[i]; DATAGRAM[250+i]=hBytes[i]; }
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
unsigned long readStoveTime = millis();
void setup() {
  pinMode(LED_WIFI_ON, OUTPUT); pinMode(LED_WIFI_OFF, OUTPUT);
  pinMode(LED_SERIAL_OK, OUTPUT); pinMode(LED_SERIAL_ERR, OUTPUT);
  pinMode(LED_MQTT_ON, OUTPUT); pinMode(LED_MQTT_OFF, OUTPUT);
  dht.begin(); setLedWifiOff(); setLedMQTTOff();
  Serial.begin(9600); Serial1.begin(1200, SERIAL_8N2);
  connectToWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  connectToMQTT();
}

// -----------------------------------------------------------------------------
// LOOP PRINCIPALE
// -----------------------------------------------------------------------------
void loop() {
  // Mantieni connessione WiFi
  if (WiFi.status() != WL_CONNECTED) { setLedWifiOff(); connectToWiFi(); }
  else { setLedWifiOn(); }

  // Mantieni connessione MQTT
  if (!mqttClient.connected()) { setLedMQTTOff(); connectToMQTT(); }
  else { setLedMQTTOn(); }
  mqttClient.loop();

  // Lettura stufa e sensori ogni READING_TIME
  if (millis() - readStoveTime > READING_TIME) {
    readStove();
    if (readStoveStatus) {
      readSensors();
      readStoveTime = millis();
      bool ok = mqttClient.publish(mqtt_topic, DATAGRAM, DATAGRAM_SIZE);
      if (ok) Serial.println("Datagramma MQTT pubblicato!");
      else Serial.println("Errore pubblicazione MQTT!");
    }
  }
  delay(1000);
}
