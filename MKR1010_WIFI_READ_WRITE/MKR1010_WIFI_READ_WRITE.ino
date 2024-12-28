#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiClient.h>
#include "DHT.h"
// Configurazione Wi-Fi
const char* ssid = "BabolandWifi";       // Nome della rete Wi-Fi
const char* password = ""; // Password della rete Wi-Fi

// Configurazione server
const char* serverIP = "192.168.4.2"; // Indirizzo IP del server
const int serverPort = 8080;           // Porta del server

const int READING_TIME = 30*1000;
const int DATAGRAM_SIZE = 255;
byte DATAGRAM[DATAGRAM_SIZE];

const int BUFFER_SIZE = 2;
byte readBuf[4];




#define DHTPIN 6        // Pin a cui è collegato il sensore
#define DHTTYPE DHT11    // Indicazione del modello del sensore
DHT dht(DHTPIN, DHTTYPE);
 



// Configurazione IP statico
IPAddress localIP(192, 168, 4, 3);   // IP statico desiderato
IPAddress gateway(192, 168, 4, 1);    // Gateway della rete
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress dns(8, 8, 8, 8);            // Indirizzo DNS (es. Google DNS)

// TCP Client
WiFiClient client;
int wifiStatus = WL_IDLE_STATUS;

void setLedWifiOff(){
   digitalWrite(1,HIGH);
   digitalWrite(0,LOW);
}

void setLedWifiOn(){
   digitalWrite(0,HIGH);
   digitalWrite(1,LOW);
}








void setLedSocketOff(){
   digitalWrite(5,HIGH);
   digitalWrite(4,LOW);
}

void setLedSocketOn(){
   digitalWrite(4,HIGH);
   digitalWrite(5,LOW);
}


void connectToWiFi() {
   
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connessione a WiFi...");
    WiFi.begin(ssid, password);
    Serial.print("After begin");
    int attempt = 0;

     // Configura l'IP statico
    WiFi.config(localIP, gateway, subnet, dns);
    while (WiFi.status() != WL_CONNECTED && attempt < 10) {
      delay(1000);
      Serial.print(attempt);
      attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnesso a Wi-Fi");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      setLedWifiOn();
    } else {
      Serial.println("\nFallito, riprovo...");
      setLedWifiOff();
    }
  }
}


/**
 * istanzia connessione al Buffer Spooler
 */
void connectToServer() {
  if (!client.connected()) {
    Serial.print("Connessione al server...");
    if (client.connect(serverIP, serverPort)) {
      Serial.println("Connesso al server!");
      setLedSocketOn();
    } else {
      setLedSocketOff();
      Serial.println("Fallita la connessione al server, ritento...");
      delay(1000);
    }
  }
}

/**
 * Ciclo setup board
 */
void setup() {
  
  
  pinMode(0, OUTPUT);
   pinMode(1, OUTPUT);
   pinMode(2, OUTPUT);
   pinMode(3, OUTPUT);
   pinMode(4, OUTPUT);
   pinMode(5, OUTPUT);
   pinMode(7, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();
  setLedWifiOff();
  digitalWrite(3,HIGH);
  setLedSocketOff();
  
  Serial.begin(9600);
  Serial1.begin(1200, SERIAL_8N2);

  delay(100);
  Serial.println("Serial1 configurata in modalità SERIAL_8N2");

  // Connessione a Wi-Fi
  connectToWiFi();

}

/**
 * invia il datagramma di richiesta passato in input, attende la risposta entro il valore di timeout
 * la risposta viene memorizzata nel byte array readBuf
 * 
 * byte* byteArray  datagramma di richiesta
 * unsigned long timeout valore di timeout di default 1sec
 * return true in caso di ricezione risposta entro timeout
 */
bool sendAndReceiveSerial(byte* byteArray,  unsigned long timeout = 1000) {
     //Pulizia del buffer di input
    while (Serial1.available() > 0) {
        Serial1.read();
    }

    // Invio del byte array sulla seriale
    Serial1.write(byteArray,BUFFER_SIZE);
    delay(100);
    // attesa risposta su seriale con gestione timeout
    unsigned long startTime = millis();
    int index=0;
    while (millis() - startTime < timeout) {//&&index<5
        while (Serial1.available()>0) {
            int x = Serial1.read();
            readBuf[index]=x;            
            index++;
   
        }
        // Interrompe il ciclo se è arrivata una risposta completa (2byte request echo + 2byte risposta)
        if (index==4) {
            break;
        }
    }
    
    return index==4;//torna true se sono arrivati eentro il valore di timeout esattamente 4 byte
}


/**
 * legge le locazioni di memoria 0x00 -> 0xFF salvandole nel byte array  DATAGRAM
 */
bool readStoveStatus=false;
byte DUMMY_REQUEST[BUFFER_SIZE] = {0X00,0x00};
void readStove(){
  //itera sulle locazioni di memoria 0x00 -> 0xFF
  readStoveStatus=true;
  for(short i=0;i<255;i++){
    //aggiorna nella struttura di reqest l'area di memoria da richiedere
    //digitalWrite(2,LOW);
    DUMMY_REQUEST[1]=i;
    if(sendAndReceiveSerial(DUMMY_REQUEST)){

        //TODO controllare il checksum
        Serial.println((int)readBuf[3]);
        //per l'iesima richiesta, salvo il valore ottenuto (messo a disposizione nell'ultimo byte del buffer di lettura)
        DATAGRAM[i]=readBuf[3];
        digitalWrite(3,LOW);
        digitalWrite(2,HIGH);       
    }  
    else {
      Serial.println("err");
      //DATAGRAM[i]=0xFF;
      digitalWrite(2,LOW);
      readStoveStatus=false;
    }
  }
  digitalWrite(2,LOW);
}


  float h;
  float t;

  byte hBytes[4];     // Un float è di 4 byte su Arduino
  byte tBytes[4];
void readSensors(){
  digitalWrite(LED_BUILTIN, HIGH);
  h = dht.readHumidity();
  t = dht.readTemperature();

  hBytes[4];     // Un float è di 4 byte su Arduino
  tBytes[4];
  
  Serial.print("Umidita' (%): ");
  Serial.print(h, 1);
  Serial.print(" -  ");
  Serial.print("Temperatura (C): ");
  Serial.println(t, 1);  

  memcpy(hBytes, &h, sizeof(h));
  memcpy(tBytes, &t, sizeof(t));
   for (int i = 0; i < 4; i++) {
    DATAGRAM[250+i]=hBytes[i];
    DATAGRAM[246+i]=tBytes[i];
  }
 digitalWrite(LED_BUILTIN, LOW);
}


//inizializza il timestamp per il calcolo timeput lettura
unsigned long readStoveTime=millis();

/**
 * ciclo infinito
 */
void loop() {

  // Controlla lo stato Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnesso, riconnessione in corso...");
    connectToWiFi();  
  }


    
    // leggo i dati ogni READING_TIME msec
    if (millis() - readStoveTime > READING_TIME) 
    {
       readStove();
       if(readStoveStatus){
        
        //se riesco a leggere i valori della stufa, leggo il sensore onboard t+h
        //utilizzo gli ultimi 8 byte del datagramma per salvare i due valori float
        readSensors();
        
        
         // imposto il valore dell'ultima lettura
        readStoveTime=millis();
        //invio il datagramma al server
        // Connettersi al server
        connectToServer();
        // Invia il messaggio
        if (client.connected()) {          
          client.write(DATAGRAM,DATAGRAM_SIZE);
          Serial.println("Messaggio inviato con successo!");
          client.stop();
         }
         else 
         {
           Serial.println("Socket non connesso, ritento...");
         } 
         
       }
       else{
         digitalWrite(3,HIGH);
       }
     } 
     
    

    //attende un secondo
    delay(1000);
}
