 //******************************
 //* Access values of Humidity, temperature, PM1, PM2.5 and PM10 of air quality through local webserver
 //
 //* Hardware used: ESP8266 board, Plantower air quality sensor (works with both 5003 & 7003), DHT temp/hum sensors
 //
 //* Forked from: https://wiki.dfrobot.com/PM2.5_laser_dust_sensor_SKU_SEN0177
 //******************************

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Arduino.h>
#include "DHT.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define LENG 31   //0x42 + 31 bytes equal to 32 bytes
#define DHTPIN D1 // DHT sensor data pin is connected to ESP8266 pin D1
#define DHTTYPE DHT22 // I'm using DHT22 sensor

unsigned char buf[LENG];
int PM01Value=0;          //define PM1.0 value of the air detector module
int PM2_5Value=0;         //define PM2.5 value of the air detector module
int PM10Value=0;         //define PM10 value of the air detector module

DHT dht(DHTPIN, DHTTYPE);

// For timestamp
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// WiFi config
const char* ssid = "your-wifi-network-name";
const char* password = "your-wifi-network-password";

// Create an instance of the server, specify the port to listen on as an argument
ESP8266WebServer server(80);
String page = "";

void setup()
{
  // For getting timestamp
  timeClient.begin();
  
  // Some basic html to render the page
  page = "<h1>Dashboard</h1><h2>Temperature (Celsius), Humidity (%), Particulate Matter 1, 2.5, 10 (ug/m3): </h2>";
 
  // DHT initialization
  Serial.begin(9600);   //use serial0
  delay(10);
  dht.begin();
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // Sending to the server
  server.on("/", [](){
    server.send(200, "text/html", page);
  });
  server.begin();
  Serial.println("Web server started!");

  Serial.setTimeout(1500);    //set the Timeout to 1500ms, longer than the data transmission periodic time of the sensor

}

void loop()
{
  // For timestamp
  timeClient.update();  

  // Reading hum & temp from DHT
  float  h = dht.readHumidity();
  float  t = dht.readTemperature();

  // Get timestamp
  String formattedTime = timeClient.getFormattedTime();

  // Add data to website
  page = page + "<p>"+ formattedTime + "    -    " + String(t) + "    -    "+ String(h);

  // Print to serial for control
  Serial.println(t);
  Serial.println(h);

  delay(1000);
  server.handleClient();
  
  // Reading data from PMS sensor
  
  if(Serial.find(0x42)){    //start to read when detect 0x42
    Serial.readBytes(buf,LENG);

    if(buf[0] == 0x4d){
      if(checkValue(buf,LENG)){
        PM01Value=transmitPM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value=transmitPM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value=transmitPM10(buf); //count PM10 value of the air detector module
      }
    }

}

  static unsigned long OledTimer=millis();
    if (millis() - OledTimer >=1000)
    {
      OledTimer=millis();

      // Printing PMS values to serial for control
      Serial.print("PM1.0: ");
      Serial.print(PM01Value);
      Serial.println("  ug/m3");

      Serial.print("PM2.5: ");
      Serial.print(PM2_5Value);
      Serial.println("  ug/m3");

      Serial.print("PM1 0: ");
      Serial.print(PM10Value);
      Serial.println("  ug/m3");
      Serial.println();

      // Adding values to website
      page = page + "    -    " + String(PM01Value) +"    -    "+ String(PM2_5Value) + "    -    " + String(PM10Value) +"</p>";
 
      server.handleClient();

    }

}

char checkValue(unsigned char *thebuf, char leng)
{
  char receiveflag=0;
  int receiveSum=0;

  for(int i=0; i<(leng-2); i++){
  receiveSum=receiveSum+thebuf[i];
  }
  receiveSum=receiveSum + 0x42;

  if(receiveSum == ((thebuf[leng-2]<<8)+thebuf[leng-1]))  //check the serial data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
  }

//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module
  return PM10Val;
  }
