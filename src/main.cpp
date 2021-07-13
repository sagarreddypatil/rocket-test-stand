#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <HX711.h>
#include <LittleFS.h>

WiFiServer server(80);
WiFiClient client;
// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");
// DNSServer dns;
HX711 scale;

File csvFile;

const int DOUT = D6;
const int CLK = D7;

// void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
// {
//   AwsFrameInfo *info = (AwsFrameInfo *)arg;
//   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
//   {
//     data[len] = 0;
//     if (strcmp((char *)data, "zero") == 0)
//     {
//       scale.tare(100);
//     }
//     else if (strcmp((char *)data, "reset") == 0)
//     {
//       ws.closeAll();
//       ESP.reset();
//     }
//   }
// }

// void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
//              void *arg, uint8_t *data, size_t len)
// {
//   switch (type)
//   {
//   case WS_EVT_CONNECT:
//     Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
//     break;
//   case WS_EVT_DISCONNECT:
//     Serial.printf("WebSocket client #%u disconnected\n", client->id());
//     break;
//   case WS_EVT_DATA:
//     handleWebSocketMessage(arg, data, len);
//     break;
//   case WS_EVT_PONG:
//   case WS_EVT_ERROR:
//     break;
//   }
// }

void setup()
{
  Serial.begin(2000000);

  scale.begin(DOUT, CLK);
  scale.tare();

  // AsyncWiFiManager wifiManager(&server, &dns);
  // wifiManager.autoConnect();
  // WiFi.hostname("ESPTestStand");
  // Serial.println("Connected");

  if (MDNS.begin("ESPTestStand", WiFi.localIP()))
  {
    MDNS.addService("ws", "tcp", 80);
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS Started");
  }

  // ws.onEvent(onEvent);
  // server.addHandler(&ws);

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //           { request->send_P(200, "text/html", "Hello World!"); });
  // server.begin();
  // Serial.println("Server Started");

  while (!WiFi.isConnected())
  {
    Serial.println("Connecting...");
    delay(1000);
  }

  Serial.println(WiFi.localIP());

  server.begin();
  client = server.available();
  Serial.println("Server Started");

  Serial.println("Inizializing FS...");
  if (LittleFS.begin())
  {
    Serial.println("done.");
  }
  else
  {
    Serial.println("fail. Halting");
    while (1)
      ;
  }

  LittleFS.remove("/log.csv");
  csvFile = LittleFS.open("/log.csv", "w");
  csvFile.println("millis,rawvalue");
}

unsigned long prevTime = 0;

void actual_loop()
{
  unsigned long t = millis();
  // ws.cleanupClients();
  MDNS.update();

  long rawValue = scale.get_value();

  char out[50];
  sprintf(out, "%lu,%ld\n", millis(), rawValue);

  // ws.textAll(out);
  client.write(out);

  Serial.print(out);
  csvFile.print(out);

  // Serial.println(1000.0 / (t - prevTime));

  prevTime = t;
}

void loop()
{
  // if (millis() > 10000)
  // {
  //   csvFile.close();
  //   csvFile = LittleFS.open("/log.csv", "r");
  //   Serial.println(csvFile.readString());
  //   csvFile.close();
  //   delay(100000);
  // }

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