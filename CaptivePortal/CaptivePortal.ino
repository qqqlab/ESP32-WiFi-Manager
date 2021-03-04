#include <qqqWiFiManager.h>

uint32_t interval = 1000;
uint32_t interval_start = -interval;
int cnt;

void webPage() {
  Serial.println("webPage");
  String s = "<html>"
"<head><title>Test Page</title></head>"
"<script>setTimeout(function() {  location.reload();}, 1000);</script>"
"<body><h1>count = ";
s += String(cnt);
s += "</h1></body></html>";
  WiFiManager.webServer.send(200, "text/html", s);
}

void setup()
{
  Serial.begin(115200);
  Serial.flush();
  delay(50);

  WiFiManager.portalTimeout = 0; //never exit portal
  WiFiManager.portalSSID = "Captive-Portal-Demo";
  WiFiManager.portalPassword = "";

  Serial.println("--------------------------------------------------------");
  Serial.println("Captive Portal Demo");
  Serial.println("Board: " ARDUINO_BOARD);
  Serial.println("--------------------------------------------------------");
  Serial.println("Use your phone and connect to:");
  Serial.println();
  Serial.println("   Network: " + WiFiManager.portalSSID);
  Serial.println("   Password: " + WiFiManager.portalPassword);
  Serial.println();
  Serial.println("--------------------------------------------------------");
  Serial.flush();

  WiFiManager.startPortal(webPage);
}


void loop()
{
  if(millis() - interval_start >  interval) {
    interval_start = millis();
    Serial.println(++cnt);
  }

  WiFiManager.loop();
}
