#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <HX711.h>
#include "secrets.h"

WiFiServer server(80);
WiFiClient client;
HX711 scale;

const int DOUT = D6;
const int CLK = D7;

void setup()
{
  Serial.begin(2000000);

  scale.begin(DOUT, CLK);
  scale.tare();

  WiFi.begin(SSID, PWD); //insert ssid and pwd here
  while (!WiFi.isConnected())
  {
    Serial.println("Connecting...");
    delay(1000);
  }

  if (MDNS.begin("ESPTestStand", WiFi.localIP()))
  {
    MDNS.addService("ws", "tcp", 80);
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS Started");
  }

  Serial.println(WiFi.localIP());

  server.begin();
  client = server.available();
  Serial.println("Server Started");

  Serial.println("\nWaiting 1 second");
  delay(1000);
}

unsigned long prevTime = 0;

void actual_loop()
{
  unsigned long t = millis();
  MDNS.update();

  long rawValue = scale.get_value();

  char out[50];
  sprintf(out, "%lu,%ld\n", millis(), rawValue);

  if (client.connected())
  {
    client.write(out);
  }

  Serial.print(out);

  // Serial.println(1000.0 / (t - prevTime));

  prevTime = t;
}

void loop()
{
  client = server.available();
  if (client.connected())
  {
    while (client.connected())
    {
      actual_loop();
    }
  }
  else
  {
    actual_loop();
  }
}