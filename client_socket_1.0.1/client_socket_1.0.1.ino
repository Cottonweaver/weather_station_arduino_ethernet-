#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_Sensor.h> 
#include <DHT.h>
#include <Wire.h>
#include "err.h"

//--------------------------------------------------------------------
//declare / define var and objects

#define DHTTYPE DHT22

#define DELAY_TIME 3000

EthernetClient client;

//für connection wichtige ips und subnetmaske 
IPAddress ServerIp(192, 168, 178, 21);
IPAddress ip(192, 168, 178, 11);
IPAddress gateway(192, 168,178,1);
IPAddress subnet(255, 255, 255, 0);

volatile long port = 1234;

//code defined mac adress
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5};

static const int DHT_PIN_1 =7;
static const int DHT_PIN_2 =6;

static const int lightSens = A5;
static const int moistureSens = A4;


DHT dht_1 = DHT(DHT_PIN_1, DHTTYPE);
DHT dht_2 = DHT(DHT_PIN_2, DHTTYPE);

//structure zum speichern von Messwerten
struct values{
  float temperature1 =0;
  float humidity1 =0;
  float temperature2 =0;
  float humidity2 =0;
  float lux;
  int moisture;
};

//input und output string
String input;
String output;
char trennzeichen = ';';

//create values object
values v;


float volt;
float ampere;
float microampere;

//--------------------------------------------------------------------
//setup

void setup() 
{
  //begin Serial for debugging and wait till Serial is open
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  
  dht_1.begin();
  dht_2.begin();
  
  //init sensor pins and SPI bus for RTC=real time clock
  pinMode(lightSens, INPUT);
  pinMode(moistureSens, INPUT);
  
  //begin Ethernet shield
  Ethernet.begin(mac, ip, gateway, subnet);

  //check if Ethernet shield is available
  if (Ethernet.hardwareStatus() == EthernetNoHardware) 
  {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware.");
    while (true) 
    {
      delay(1);
    }
  }

  //check if ethernet cable is plugged in
  if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
  }
  
  //try connecting to server
  client.connect(ServerIp, port);

  delay(1000);
}

//--------------------------------------------------------------------
//Main loop

void loop() 
{
  readSensDat();
  //check if client is connected to server if so send values to server and to the Serial output
  if(!client.connected())
  {
    client.stop();
    client.connect(ServerIp, port);
    client.setTimeout(1500);
    Serial.println(dumpToString());
    delay(DELAY_TIME);
  }

  //if not connected to server just print values to Serial output
  else
  {
    input=read_();

    if(input == "GET_SEN")
    {
      delay(DELAY_TIME);
      sendSensDat();
      Serial.println(dumpToString());
    }
  }
}


//--------------------------------------------------------------------
//selfmade functions

int readSensDat()
{
  int analog_light;
  float temp_lux;
  //read analog voltage values and convertig it into lux value
  analog_light = analogRead(lightSens);
  
  temp_lux = analog_light * 5.0f / 1024.0f; //wandle analogen wert in anliegende Spannung um
  temp_lux = temp_lux / 10000.0f; //wandle anliegende Spannung über intern verbauten wiederstand in ampere um
  temp_lux = temp_lux * 1000000; // wandle in milli Ampere um 
  temp_lux = temp_lux * 2; // wandle milli Ampere anhand vom Datasheet in einheit lux um y=ampere x=lumen Formel -> y=1/2(x)
  v.lux = temp_lux;
  
  
  v.moisture = analogRead(moistureSens);
    
  //put DHT11 values into structure
  //dht11.read(DHT_PIN_1, &v.temperature1, &v.humidity1, NULL);
  //dht11.read(DHT_PIN_2, &v.temperature2, &v.humidity2, NULL);

  v.temperature1 = dht_1.readTemperature();
  v.humidity1 = dht_1.readHumidity();
  v.temperature2 = dht_2.readTemperature();
  v.humidity2 = dht_2.readHumidity();
}

//blocking read function that can react on an disconnect
//return
//received string or ERR if disconnecting to the server while listening
String read_()
{
  char c;
  String input="";
  int ba;

  while(!client.available())
  {
    if(!client.connected())
    {
      return(ERR_N_C);
    }
  }

  ba = client.available();
  
    for(int i = 0; i < ba; i++)
    {
        c = client.read();
        input += c;
    }
  return input;
}

//create string out of values structure
String dumpToString()
{
  String out = "";
  out += v.lux;
  out += trennzeichen;
  out += v.moisture;
  out += trennzeichen;
  out += v.temperature1;
  out += trennzeichen;
  out += v.humidity1;
  out += trennzeichen;
  out += v.temperature2;
  out += trennzeichen;
  out += v.humidity2;
  
  return out;
}

//send whole value structure to server with simple value -> OK structure (see simple example communication in the project_docs)
void sendSensDat()
{   
    client.print(dumpToString());
    client.flush();

}
