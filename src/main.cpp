#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <HX711.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;
HX711 scale;

const int DOUT = D6;
const int CLK = D7;

float calibration_factor = 25225;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "zero") == 0)
    {
      scale.tare(100);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  Serial.begin(2000000);

  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect();
  Serial.println("Connected");

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", "Hello World!"); });
  server.begin();

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

unsigned long prevTime = 0;

void loop()
{
  unsigned long t = millis();
  ws.cleanupClients();

  long rawValue = scale.get_value();

  char out[50];
  sprintf(out, "%lu,%ld", millis(), rawValue);

  ws.textAll(out);
  // Serial.println(out);
  Serial.println(1000.0 / (t - prevTime));

  prevTime = t;
}