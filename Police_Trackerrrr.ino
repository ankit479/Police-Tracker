#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* GScriptId =
"AKfycbzGTsD6tSBgQD2JttASHRQF_5U1TkY1IrvkrnDUA7Lf2d3suceOnbgp-MoKsT7oJWct";

String gate_number = "Company";

const char* ssid     = "Motorola";
const char* password = "vivi@123";

String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;
String student_id;

int blocks[] = {4,5,6,8,9};
#define total_blocks (sizeof(blocks) / sizeof(blocks[0]))

#define RST_PIN  0
#define SS_PIN   2
#define buzzer   15
#define led      16

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

int blockNum = 2;
byte bufferLen = 18;
byte readBlockData[18];

void setup() {
    Serial.begin(9600);
    delay(10);
    Serial.println('\n');
    SPI.begin();
    pinMode(buzzer, OUTPUT);
    pinMode(led, OUTPUT);

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting to");
    lcd.setCursor(0,1);
    lcd.print("WiFi...");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to ");
    Serial.print(ssid); Serial.println("...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println('\n');
    Serial.println("WiFi Connected!");
    Serial.println(WiFi.localIP());

    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting to");
    lcd.setCursor(0,1);
    lcd.print("Google ");

    delay(50);
    Serial.print("Connecting to ");
    Serial.println(host);

    bool flag = false;
    for(int i=0; i<5; i++){
        int retval = client->connect(host, httpsPort);
        if (retval == 1){
            flag = true;
            String msg = "Connected. OK";
            Serial.println(msg);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(msg);
            delay(20);
            break;
        }
        else
            Serial.println("Connection failed. Retrying...");
    }
    if (!flag) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Connection fail");
        Serial.print("Could not connect to server: ");
        Serial.println(host);
        delay(50);
        delete client;
        client = nullptr;
    }
}

void loop() {
    static bool flag = false;
    if (!flag){
        client = new HTTPSRedirect(httpsPort);
        client->setInsecure();
        flag = true;
        client->setPrintResponseBody(true);
        client->setContentTypeHeader("application/json");
    }

    if (client != nullptr){
        if (!client->connected()){
            int retval = client->connect(host, httpsPort);
            if (retval != 1){
                Serial.println("Disconnected. Retrying...");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Disconnected.");
                lcd.setCursor(0,1);
                lcd.print("Retrying...");
                return;
            }
        }
        else{
            Serial.println("Error creating client object!");
            Serial.println("else");

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Scan your Tag");

            mfrc522.PCD_Init();
            if (!mfrc522.PICC_IsNewCardPresent()) {return;}
            if (!mfrc522.PICC_ReadCardSerial()) {return;}

            Serial.println(F("Reading last data from RFID..."));
            String values = "", data;
            for (byte i = 0; i < total_blocks; i++) {
                ReadDataFromBlock(blocks[i], readBlockData);
                if(i == 0){
                    data = String((char*)readBlockData);
                    data.trim();
                    student_id = data;
                    values += "\n" + data + ",";
                } else {
                    data = String((char*)readBlockData);
                    data.trim();
                    values += data + ",";
                }
            }
            values += gate_number + "\"}";

            payload = payload_base + values;

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Publishing Data");
            lcd.setCursor(0,1);
            lcd.print("Please Wait...");
            digitalWrite(buzzer, HIGH);
            digitalWrite(led, HIGH);
            delay(400);
            digitalWrite(buzzer, LOW);
            digitalWrite(led, LOW);

            Serial.println("Publishing data...");
            Serial.println(payload);

            if(client->POST(url, host, payload)){
                Serial.println("[OK] Data published.");
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Student ID: "+student_id);
                lcd.setCursor(0,1);
                lcd.print("Thanks");
                digitalWrite(buzzer, HIGH);
                digitalWrite(led, HIGH);
                delay(10);
                digitalWrite(buzzer, LOW);
                digitalWrite(led, LOW);
                delay(10);
            }
            else{
              Serial.println("Error while connecting");
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("Failed.");
              lcd.setCursor(0,1);
              lcd.print("Try Again");
            }
            Serial.println("[TEST] delay(5000)");
            delay(50);
        }
    }
}






          void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed for Read: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    } else {
        Serial.println("Authentication success");
    }
    status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Reading failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    } else {
        readBlockData[16] = ' ';
        readBlockData[17] = ' ';
        Serial.println("Block was read successfully");
    }
}
