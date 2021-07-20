#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HX711.h>
#include <LittleFS.h>

#include "secrets.h"

WiFiServer liveDataServer(81);
WiFiClient client;

AsyncWebServer fileServer(80);

HX711 scale;

File dataFile;
void startDataFile() {
  dataFile = LittleFS.open("/data.csv", "w");
  dataFile.write("counter,timestamp,scaleValueRaw\n");
}
void removeDataFile() { LittleFS.remove("/data.csv"); }

const int DOUT = D6;
const int CLK = D7;

constexpr bool AP = false;

void blink() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
}

void errorLED() {
  while (1) {
    blink();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(2000000);

  scale.begin(DOUT, CLK);
  scale.tare();

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Failed");
    errorLED();
  }

  if (AP)
    WiFi.softAP("ESPTestStand", "All Hail Newton");
  else {
    WiFi.begin(SSID, PWD);
    while (!WiFi.isConnected()) {
      Serial.println("Connecting...");
      blink();
      blink();
      delay(800);
    }
  }

  if (MDNS.begin("ESPTestStand", WiFi.localIP())) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS Started");
  }

  Serial.println(WiFi.localIP());

  fileServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/fstest.txt", String(), false);
  });

  fileServer.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/data.csv", "text/csv", false);
  });

  fileServer.begin();
  Serial.println("File Server Started");

  liveDataServer.begin();
  client = liveDataServer.available();
  Serial.println("Live Data Server Started");

  digitalWrite(LED_BUILTIN, LOW);
}

unsigned long prevTime = 0;
unsigned long counter = 0;
char out[1024];
char fileOut[1024];
char ipt[10];
bool pauseFileWrites = true;

void actual_loop() {
  unsigned long t = millis();
  MDNS.update();

  long rawValue = scale.get_value();
  snprintf(out, sizeof(out), "%d,%lu,%lu,%ld\n", pauseFileWrites, counter, t,
           rawValue);

  if (client.connected()) {
    counter++;
    client.write(out);
  }

  int len = snprintf(NULL, 0, "%lu,%lu,%ld\n", counter, t, rawValue);
  snprintf(fileOut, sizeof(fileOut), "%lu,%lu,%ld\n", counter, t, rawValue);

  if (!pauseFileWrites) dataFile.write(fileOut, len);

  if (client.available()) {
    memset(ipt, 0, sizeof(ipt));
    client.readBytesUntil('\n', ipt, sizeof(ipt));

    Serial.print("Recieved command:");
    Serial.println(ipt);

    if (strcmp(ipt, "zero") == 0) {
      scale.tare(100);
      Serial.println("Zeroed Scale");
    } else if (strcmp(ipt, "pause") == 0) {
      if (pauseFileWrites) {
        startDataFile();
        Serial.println("Unpaused");
      } else {
        dataFile.close();
        Serial.println("Paused");
      }
      pauseFileWrites = !pauseFileWrites;
    } else if (strcmp(ipt, "clear") == 0) {
      dataFile.close();
      removeDataFile();
      if (!pauseFileWrites) startDataFile();

      Serial.println("Data cleared");
    } else if (strcmp(ipt, "reset") == 0) {
      Serial.println("Resetting...");
      ESP.reset();
    }
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
