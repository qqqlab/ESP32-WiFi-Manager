//===========================================================================
// WiFi Manager (captive portal)
//===========================================================================
#define WM_CONNECT_TIMEOUT 20 //seconds to wait to establish wifi connection
#define WM_PORTAL_TIMEOUT 300 //seconds to keep captive portal alive before trying to reconnect
#define WM_DROPPED_TIMEOUT 60 //seconds to wait for dropped wifi reconnection before starting captive portal
#include <WiFi.h>

#include <DNSServer.h>
IPAddress wm_apIP(192, 168, 1, 1);
DNSServer wm_dnsServer;

#include <WebServer.h>
WebServer wm_webserver(80);

bool wm_saved = false;
String wm_ssid = "";
String wm_pw = "";

//returns false if connected to ssid
//returns true with newly saved ssid + pw
//blocks (does not return) if can't connect
bool wm_setup(String &ssid, String &pw, String portal_ssid, String portal_pw) {
  wm_ssid = ssid;
  wm_pw = pw;
  
  //attempt to connect to network
  if(wm_ssid=="") {
    Serial.println("No SSID stored, starting captive portal");
  } else{
    Serial.print("Connecting to " + wm_ssid + " ");
    WiFi.begin(wm_ssid.c_str(), wm_pw.c_str());
    int timeout = WM_CONNECT_TIMEOUT;
    while (timeout && WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
      timeout--;      
    }
    if(timeout) {
      Serial.println(" connected");
      return false;
    }else{
      Serial.println(" FAILED, starting captive portal");
    }
  }
  
  //================================
  //captive portal
  Serial.println("Starting captive portal");
  
  //wifi 
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  //WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_AP);
  delay(2000); //needed, otherwise crash on connect
  WiFi.softAP(portal_ssid.c_str(),portal_pw.c_str());
  WiFi.softAPConfig(wm_apIP, wm_apIP, IPAddress(255, 255, 255, 0));
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("portal started");

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  wm_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  wm_dnsServer.start(53, "*", wm_apIP);
  Serial.println("dns server started");

  //setup webserver
  wm_webserver.onNotFound(wm_handleNotFound);
  wm_webserver.begin();
  Serial.println("webserver started");
  Serial.println("captive portal started");


  uint32_t portal_millis = millis();
  while(millis() - portal_millis <= ((WM_PORTAL_TIMEOUT)*1000)) {
    if(WiFi.softAPgetStationNum() > 0) portal_millis = millis();
    wm_dnsServer.processNextRequest();
    wm_webserver.handleClient();
    if(wm_saved) {
      ssid = wm_ssid;
      pw = wm_pw;
      delay(1000);
      return true;
    }
  }
  wm_reboot("captive portal timeout");
  return false; //keep compiler happy
}

void wm_loop() {
  static uint32_t dropped_millis;
  if(WiFi.status() == WL_CONNECTED) {
    dropped_millis = millis();
    return;
  }
  if(millis() - dropped_millis > ((WM_DROPPED_TIMEOUT)*1000)){
    wm_reboot("Connection dropped");
  }
}

String wm_scan()
{
  String s = "";
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    if (n == 0) {
        s += "no networks found";
    } else {
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            s += "<button onclick=\"upd('" + WiFi.SSID(i) + "')\">" 
            //+ String(i + 1) + ": " 
            + WiFi.SSID(i) + " (" 
            + String(WiFi.RSSI(i)) + ")" 
            + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " " : "*") 
            + "</button><br />";
            delay(10);
        }
        s += "<script>function upd(s){document.getElementById('ssid').value=s;var pw=document.getElementById('pw');pw.value='';pw.focus();}</script>";
    }
    return s;
}

void wm_handleNotFound() {
  String message;
  String ssid = wm_webserver.arg("ssid");
  String pw = wm_webserver.arg("pw");
  if(ssid!="") {
    wm_ssid = ssid;
    wm_pw = pw;
    wm_saved = true;
    message = "Settings Saved - Rebooting";
  }else{
    message = "<html><body><form>Network<input id=\"ssid\" name=\"ssid\"><br />Password<input id=\"pw\" name=\"pw\" type=\"password\"><br /><input type=\"submit\" value=\"Save\"></form>" 
    + wm_scan();
    message += "URI: ";
    message += wm_webserver.uri();
    message += "\nMethod: ";
    message += (wm_webserver.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += wm_webserver.args();
    message += "\n";
    for (uint8_t i = 0; i < wm_webserver.args(); i++) {
      message += " " + wm_webserver.argName(i) + ": " + wm_webserver.arg(i) + "\n";
    }
  }
  wm_webserver.send(200, "text/html", message);
}

void wm_reboot(String msg) {
  Serial.println(msg + " --> rebooting...");
  Serial.flush();
  delay(1000); 
  ESP.restart();
  while(1) {delay(1000); yield();}  
}

//===========================================================================
// config (Preferences)
//===========================================================================
#include <Preferences.h>

typedef struct {
  String ssid;
  String pw;
//  String myinflux;
  String mqtt_server;
  uint16_t mqtt_port;
} config_t;
config_t config;
#define CONF_NAME "qqq"

void config_setup() {
  Preferences prefs;
  prefs.begin(CONF_NAME, true); //readonly
  config.ssid        = prefs.getString("ssid", "");
  config.pw          = prefs.getString("pw", "");
//  config.myinflux    = prefs.getString("myinflux",    CONF_RX_URL);
  //config.mqtt_server = prefs.getString("mqtt_server", CONF_MQTT);
  //config.mqtt_port   = prefs.getUShort("mqtt_port",   1883);
  prefs.end();  
}

void config_save() {
  Preferences prefs;
  prefs.begin(CONF_NAME, false); //readwrite
  prefs.putString("ssid",        config.ssid);
  prefs.putString("pw",          config.pw);
//  prefs.putString("myinflux",    config.myinflux);
  //prefs.putString("mqtt_server", config.mqtt_server);
  //prefs.putUShort("mqtt_port",   config.mqtt_port);
  prefs.end();
}


//===========================================================================
// MAIN
//===========================================================================

void setup() {
  Serial.begin(115200);
  while(!Serial);

  config_setup();
    
  if(wm_setup(config.ssid, config.pw, "qqqlab", "12345678")) {
    config_save();
    wm_reboot("Storing new ssid="+config.ssid);
  }

  //...
}

void loop() {
  wm_loop();

  //...
}
