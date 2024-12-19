#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiClient.h>
// Configurazione Wi-Fi
const char* ssid = "SSID";       // Nome della rete Wi-Fi
const char* password = "PWD"; // Password della rete Wi-Fi

// Configurazione server
const char* serverIP = "192.168.4.2"; // Indirizzo IP del server
const int serverPort = 8080;           // Porta del server

const int READING_TIME = 30*1000;
const int DATAGRAM_SIZE = 255;
byte DATAGRAM[DATAGRAM_SIZE];

const int BUFFER_SIZE = 2;
byte readBuf[4];


// TCP Client
WiFiClient client;
int wifiStatus = WL_IDLE_STATUS;

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connessione a WiFi...");
    WiFi.begin(ssid, password);
    Serial.print("After begin");
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 10) {
      delay(1000);
      Serial.print(attempt);
      attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnesso a Wi-Fi");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFallito, riprovo...");
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
    } else {
      Serial.println("Fallita la connessione al server, ritento...");
      delay(1000);
    }
  }
}

/**
 * Ciclo setup board
 */
void setup() {
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
void readStove(){

byte DUMMY_REQUEST[BUFFER_SIZE] = {0X00,0x00};
  //itera sulle locazioni di memoria 0x00 -> 0xFF
  for(short i=0;i<255;i++){
    //aggiorna nella struttura di reqest l'area di memoria da richiedere
    DUMMY_REQUEST[1]=i;
    if(sendAndReceiveSerial(DUMMY_REQUEST)){
        //TODO controllare il checksum
        Serial.println((int)readBuf[3]);
        //per l'iesima richiesta, salvo il valore ottenuto (messo a disposizione nell'ultimo byte del buffer di lettura)
        DATAGRAM[i]=readBuf[3];
    }  
    else {
      Serial.println("err");
      DATAGRAM[i]=0xFF;
    }
  }
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
     } else {
        Serial.println("Socket non connesso, ritento...");
      }      
    }

    //attende un secondo
    delay(1000);
}
