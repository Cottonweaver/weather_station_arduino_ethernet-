#include <SPI.h>
#include <Ethernet.h>
#include <DS3231.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>

//--------------------------------------------------------------------
//declare / define var and objects

#define DHTTYPE DHT22

EthernetClient client;

//für connection wichtige ips und subnetmaske
IPAddress ServerIp(192, 168, 178, 21);
IPAddress ip(192, 168, 178, 11);
IPAddress gateway(192, 168,178,1);
IPAddress subnet(255, 255, 255, 0);

volatile long port = 1234;

//code defined mac adress
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5};

//pins für die Sensoren ### clock benutzt SPI bus vom Arduino
static const int DHT_PIN_1 =5;
static const int DHT_PIN_2 =6;

static const int lightSens = A2;



DS3231 clock;

DHT dht_1 = DHT(DHT_PIN_1, DHTTYPE);
DHT dht_2 = DHT(DHT_PIN_2, DHTTYPE);

//structure zum speichern von Messwerten
struct values{
  float temperature1 =0;
  float humidity1 =0;
  float temperature2 =0;
  float humidity2 =0;
  int lux;
  RTCDateTime dt;
};

//input und output string
String input;
String output;
char trennzeichen = ';';

//create values object
values v;

int analog_light;
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
  Serial.println("Test");
  dht_1.begin();
  dht_2.begin();
  Serial.println("Test");
  
  //init sensor pins and SPI bus for RTC=real time clock
  pinMode(lightSens, INPUT);
  Serial.println("Test");
  clock.begin();
  
  Serial.println("Test");
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
  Serial.println("Test");
}

//--------------------------------------------------------------------
//Main loop

void loop() 
{

  //read analog voltage values (converting into lightintensity on the server side)
  v.lux = analogRead(lightSens);
    
  //put DHT11 values into structure
  //dht11.read(DHT_PIN_1, &v.temperature1, &v.humidity1, NULL);
  //dht11.read(DHT_PIN_2, &v.temperature2, &v.humidity2, NULL);

  v.temperature1 = dht_1.readTemperature();
  v.humidity1 = dht_1.readHumidity();
  v.temperature2 = dht_2.readTemperature();
  v.humidity2 = dht_2.readHumidity();

  //save time in RTCDateTime object
  v.dt = clock.getDateTime();

  //chekc if client ist connected to server if so send values to server and to the Serial output
  if(!client.connected())
  {
    client.stop();
    client.connect(ServerIp, port);
    client.setTimeout(1500);
    Serial.println(dumpToString(&v));
    delay(1500);
  }

  //if not connected to server just print values to Serial output
  else
  {
    input=read_(&client);
  
    if(input == "GET_SEN")
    {
      delay(1500);
      dumpToEthernet(&client, &v);
      Serial.println(dumpToString(&v));
    }
  }
}


//--------------------------------------------------------------------
//selfmade functions

//blocking read function that can react on an disconnect
//return
//received string or ERR if disconnecting to the server
String read_(EthernetClient* client)
{
  char c;
  String input="";
  int ba;

  while(!client->available())
  {
    if(!client->connected())
    {
      return("ERR");
    }
  }

  ba = client->available();
  
    for(int i = 0; i < ba; i++)
    {
        c = client->read();
        input += c;
    }
  return input;
}

//create string out of values structure
String dumpToString(values *v)
{
  String out = "";
  out += v->dt.year;
  out += "-";
  out += v->dt.month;
  out += "-";
  out += v->dt.day;
  out += trennzeichen;
  out += v->dt.hour;
  out += trennzeichen;
  out += v->dt.minute;
  out += trennzeichen;
  out += v->dt.second;
  out += trennzeichen;
  out += v->lux;
  out += trennzeichen;
  out += v->temperature1;
  out += trennzeichen;
  out += v->humidity1;
  out += trennzeichen;
  out += v->temperature2;
  out += trennzeichen;
  out += v->humidity2;
  
  return out;
}

//send whole value structure to server with simple value -> OK structure (see simple example communication in the project_docs)
void dumpToEthernet(EthernetClient *client, values *v)
{
    String out = "";
    client->print(v->dt.year);

    if(read_(client) == "OK")
    {
      client->print(v->dt.month);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      client->print(v->dt.day);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      client->print(v->dt.hour);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      client->print(v->dt.minute);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      client->print(v->dt.second);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      client->print(v->lux);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      out = v->temperature1;
      client->print(out);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      out = v->humidity1;
      client->print(out);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      out = v->temperature2;
      client->print(out);
    }
    else
    {
      return;
    }
    client->flush();
    if(read_(client) == "OK")
    {
      out = v->humidity2;
      client->print(out);
    }
    else
    {
      return;
    }
    client->flush();
}
