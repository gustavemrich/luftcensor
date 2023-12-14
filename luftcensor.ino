
#include <SoftwareSerial.h>    
SoftwareSerial esp(6, 7);   //Pin 6 and 7 act as RX and TX

#define DEBUG true
int number;
String mySSID = "ssid";  // Wi-Fi SSID
String myPWD = "psw"; // Wi-Fi Password
String myAPI = "QPJ3CKTU4HJ3JNTE";   // API key för thingspeak channel
String myHOST = "api.thingspeak.com";
String myPORT = "80"; // Port
String field1 = "field1"; // The fields where I send the data to.
String field2 = "field2"; 
String field3 = "field3"; 
String field4 = "field4"; 

// Initialize libraries
#include <Wire.h>
#include "Adafruit_SGP30.h"

Adafruit_SGP30 sgp;
// Function to get the quality of water vapor in the wet air of unit volume (1 m3)
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void setup()
{
  Serial.begin(115200); // Open serial communactions for the serial window.
  esp.begin(115200);  // Open the serial communications för the esp8266 module.
  Send_AT_Cmd("AT+RST", 1000, DEBUG);     // Reset the wifi module so that no old code is still running on it.         
  Send_AT_Cmd("AT+CWMODE=1", 1000, DEBUG);     // Sets the esp8266 module to station mode so that it can recieve data.
  Send_AT_Cmd("AT+CWJAP=\""+ mySSID +"\",\""+ myPWD +"\"", 1000, DEBUG);   //Connect the esp module with the wifi you provide (the ssid and psw)
  delay(1000); 

  if (! sgp.begin()){ // Check if the sgp censor is connected
    Serial.println("Sensor not found :(");
    while (1);
  }
  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);
}
int counter = 0;

void loop()
  {
  Send_Data(sgp.TVOC, field1); // Using my function to send data to send the air quality measurements to their respective fields.
  Send_Data(sgp.eCO2, field2);
  Send_Data(sgp.rawH2, field3);
  Send_Data(sgp.rawEthanol, field4);

  if (! sgp.IAQmeasure()) { //Checking if the measurements for the sgp module are working.
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
  Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");

  if (! sgp.IAQmeasureRaw()) { //Checking if the measurements for the sgp module are working.
    Serial.println("Raw Measurement failed");
    return;
  }
  Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
  Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");

  counter++;
  if (counter == 30) {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
  }
}


// function to send the AT command to the WiFi module. The function takes in two parameters: the AT command and the response time.
String Send_AT_Cmd(String command, const int timeout)
{
  Serial.print(command);
  Serial.println("     ");
  
  String response = "";
  esp.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (esp.available())
    {
      char c = esp.read();
      response += c;
    }
  }
  return response;
}

// Function to send data to a certain field inside of the thingspeak channel. Passes a data variable (the number) and the name of the field you are gonna send that number to.
void Send_Data(int data, String field) 
{
  String sendData = "GET /update?api_key="+ myAPI +"&"+ field +"="+String(data); //create a variable ‘sendData’ that contains a GET request that we will make to ThingSpeak to update the respective field with the value held in the variable ‘number’ converted to a string.
  Send_AT_Cmd("AT+CIPMUX=1", 1000, DEBUG);     // enable multiple connections (maximum 4)
  Send_AT_Cmd("AT+CIPSTART=0,\"TCP\",\""+ myHOST +"\","+ myPORT, 1000, DEBUG); // set the port connection and address of port.
  Send_AT_Cmd("AT+CIPSEND=0," +String(sendData.length()+4),1000,DEBUG);  //tells the WIFI module how many bytes we want to send to the server.
  esp.find(">"); 
  esp.println(sendData);
  Serial.print("Value to be sent: ");
  Serial.println(data);
  Send_AT_Cmd("AT+CIPCLOSE=0",1000,DEBUG); // Closes the gateway to send data.
  Serial.println("Done!");
  Serial.println("");
  delay(2500); // Delay of 2,5s between each set of data we send.
}
