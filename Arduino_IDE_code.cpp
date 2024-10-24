#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

//------------------------------------------------------------------------------------------------
// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycby9RpfJnJBHVu3VGgf437m5p0ke5L80z5uViNamGVWXEegEFpnV95XzX8x8mGYLBd8adw";
//------------------------------------------------------------------------------------------------
// Enter network credentials:
const char* ssid     = "********";
const char* password = "*********";
//------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
//------------------------------------------------------------------------------------------------
// Google Sheets setup (do not edit)
const char* host = "script.google.com";
String url = String("/macros/s/") + GScriptId + "/exec";
//------------------------------------------------------------
// Declare variables that will be published to Google Sheets
String student_Rollno;
//------------------------------------------------------------
int blocks[] = {4,5,6,8,9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))
//------------------------------------------------------------
#define RST_PIN  2  // Customize these pins as per your ESP32 setup
#define SS_PIN   4
#define BUZZER   14
//------------------------------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;
//------------------------------------------------------------
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  
/* Create another array to read data from Block */
/* Length of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
//------------------------------------------------------------

/****************************************************************
 * setup Function
****************************************************************/
void setup() {
  //----------------------------------------------------------
  pinMode(BUZZER, OUTPUT);
  Serial.begin(115200);        
  delay(10);
  Serial.println('\n');
  //----------------------------------------------------------
  SPI.begin();
  //----------------------------------------------------------
  // Initialize LCD screen
  lcd.init();
  // Turn on the backlight
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("WiFi...");
  //----------------------------------------------------------
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  //----------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("NEC website...");
  delay(5000);
}

/****************************************************************
 * loop Function
****************************************************************/
void loop() {
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Scan your ID");
  lcd.setCursor(0,1);
  lcd.print("Card or Tag...");
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));  
  //----------------------------------------------------------------
  String values = "", data;

  // Creating payload
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    //*************************************************
    if(i == 0){
      data = String((char*)readBlockData);
      data.trim();
      student_Rollno = data;
      values = "\"" + data + ",";
    }
    //*************************************************
    else if(i == total_blocks-1){
      data = String((char*)readBlockData);
      data.trim();
      values += data + "\"}";
    }
    //*************************************************
    else{
      data = String((char*)readBlockData);
      data.trim();
      values += data + ",";
    }
  }
  //----------------------------------------------------------------
  // Create json object string to send to Google Sheets
  payload = payload_base + values;
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Storing Data");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("Please Wait...");
  //----------------------------------------------------------------
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  
  if(WiFi.status() == WL_CONNECTED) { 
    HTTPClient http;
    http.begin("https://" + String(host) + url); 
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(payload);
    if(httpCode > 0) {
      if(httpCode > 0) {
        lcd.clear();
        lcd.setCursor(0,0); //col=0 row=0
        lcd.print("Roll no: "+student_Rollno);
        lcd.setCursor(0,1); //col=0 row=0
        lcd.print("Present");
        digitalWrite(BUZZER, HIGH);
        delay(2000);
        digitalWrite(BUZZER, LOW);
        delay(2000);
      }
    } else {
      Serial.println("Error on HTTP request");
      lcd.clear();
      lcd.setCursor(0,0); //col=0 row=0
      lcd.print("Failed.");
      lcd.setCursor(0,1); //col=0 row=0
      lcd.print("Try Again");
    }
    http.end(); // Free resources
  }
  //----------------------------------------------------------------
  // A delay of several seconds is required before publishing again    
  delay(5000);
}

/****************************************************************
 * ReadDataFromBlock() function
 ****************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  //----------------------------------------------------------------------------
  /* Prepare the key for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //----------------------------------------------------------------------------
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  } else {
    Serial.println("Authentication success");
  }
  //----------------------------------------------------------------------------
  /* Reading data from the Block */
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
  //----------------------------------------------------------------------------
} 
