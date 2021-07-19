#include "secrets.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <HX711.h>

WiFiServer liveDataServer(81);
WiFiClient client;
HX711 scale;

const int DOUT = D6;
const int CLK = D7;

constexpr bool AP = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(2000000);

  scale.begin(DOUT, CLK);
  scale.tare();

  if (AP)
    WiFi.softAP("ESPTestStand", "All Hail Newton");
  else {
    WiFi.begin(SSID, PWD);
    while (!WiFi.isConnected()) {
      Serial.println("Connecting...");
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(850);
    }
  }

  if (MDNS.begin("ESPTestStand", WiFi.localIP())) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS Started");
  }

  Serial.println(WiFi.localIP());

  liveDataServer.begin();
  client = liveDataServer.available();
  Serial.println("Server Started");

  digitalWrite(LED_BUILTIN, LOW);
}

unsigned long prevTime = 0;
unsigned long counter = 0;
char out[200];

void actual_loop() {
  unsigned long t = millis();
  MDNS.update();

  long rawValue = scale.get_value();

  if (client.connected()) {
    counter++;
    snprintf(out, sizeof(out), "%lu,%lu,%ld\n", counter, millis(), rawValue);
    client.write(out);
  } else {
    sprintf(out, "%lu,%ld\n", millis(), rawValue);
  }

  if (client.available()) {
    String ipt = client.readString();

    Serial.print("Recieved command: ");
    Serial.println(ipt);

    if (ipt == "zero") {
      scale.tare(10);
    } else if (ipt == "reset") {
      ESP.reset();
    }

    Serial.println("Command Finished");
  }

  // Serial.print(out);
  // Serial.println(1000.0 / (t - prevTime));

  prevTime = t;
}

void loop() {
  client = liveDataServer.available();
  client.setTimeout(100);
  if (client.connected()) {
    while (client.connected()) {
      actual_loop();
    }
  } else {
    actual_loop();
  }
}
