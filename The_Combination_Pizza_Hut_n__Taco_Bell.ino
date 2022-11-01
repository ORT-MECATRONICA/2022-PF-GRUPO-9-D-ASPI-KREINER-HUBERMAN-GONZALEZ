//Libraries
#include <SPI.h>//https://www.arduino.cc/en/reference/SPI
#include <MFRC522.h>//https://github.com/miguelbalboa/rfid
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>

unsigned long long valor1;
int valor1a;
int valor2;
int valor3;
int valor4;
unsigned long valorFinal;
int suma1;

boolean signIn = false;

int signInNumber = 1;
int signOffNumber = 1;

// Replace with your network credentials
const char* ssid     = "ORT-IoT";
const char* password = "OrtIOTnew22$";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

//Constants

#define SS_PIN 5
#define RST_PIN 2
//Parameters
const int ipaddress[4] = {103, 97, 67, 25};
//Variables
byte nuidPICC[4] = {0, 0, 0, 0};
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyA8t-9m2npYJA8ZtiXjo_t4jKDWPRtR9HQ"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://sistema-de-fichado-default-rtdb.firebaseio.com/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
String stringValue;
bool signupOK = false;

void setup() {
  //Init Serial USB
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-10800);

  Serial.println(F("Initialize System"));
  //init rfid D8,D5,D6,D7
  SPI.begin();
  rfid.PCD_Init();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
void loop() {

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  delay(1000);

  readRFID();

}
void readRFID(void ) { /* function readRFID */
  ////Read RFID card
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  // Verify if the NUID has been readed
  if (  !rfid.PICC_ReadCardSerial())
    return;
  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];

    equation();
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    String adress = "/Users/" + String(valorFinal);

    if (Firebase.RTDB.getInt(&fbdo, adress )) {
      if (fbdo.dataType() == "string") {
        stringValue = fbdo.stringData();

        String accessGranted = "Welcome, " + String(stringValue);
        String exitGranted = "Goodbye, " + String(stringValue);
        //String dateRecord = "/" + String(dayStamp) + "/" + String(stringValue);
        String signInTimeRecord = "/" + String(dayStamp) + "/" + String(stringValue) + " sign in " + signInNumber;
        String signOffTimeRecord = "/" + String(dayStamp) + "/" + String(stringValue) + " sign off" + signOffNumber;

        /*if (Firebase.RTDB.setString(&fbdo, userDateRecord, dayStamp)) {
          }

          if (Firebase.RTDB.setString(&fbdo, userTimeRecord, timeStamp)) {
          }*/

        if (signIn == false) {
          if (Firebase.RTDB.setString(&fbdo, signInTimeRecord, timeStamp)) {
            Serial.println(accessGranted);
            signIn = true;
            signInNumber = signInNumber + 1;
          }
        }
        
        else {
          if (signIn == true) {
            if (Firebase.RTDB.setString(&fbdo, signOffTimeRecord, timeStamp)) {
              Serial.println(exitGranted);
              signIn = false;
              signOffNumber = signOffNumber + 1;              
            }
          }
        }

        Serial.print(F("RFID In dec: "));
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        // Halt PICC
        rfid.PICC_HaltA();
        // Stop encryption on PCD
        rfid.PCD_StopCrypto1();

      }
    }

    /*if (Firebase.RTDB.getInt(&fbdo, "/Users/" + String(valorFinal)) ) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();
        Serial.println(intValue);
      }
      }*/
    else {
      //Serial.println(fbdo.errorReason());
      Serial.println("Unidentified user detected");
    }
  }

}
/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void equation(void)
{
  valor3 = nuidPICC[3];

  if (valor3 < 100)
  {
    valor2 = nuidPICC[2] * 100;
  }
  else
  {
    valor2 = nuidPICC[2] * 1000;
  }

  suma1 = valor3 + valor2;

  if (suma1 < 10000)
  {
    valor1 = nuidPICC[1] * 10000;
  }
  else
  {
    if (suma1 < 100000 && suma1 >= 10000)
    {
      valor1 = nuidPICC[1] * 100000;
    }
    else
    {
      valor1 = nuidPICC[1] * 1000000;
    }
  }

  valorFinal = suma1 + valor1;

}
