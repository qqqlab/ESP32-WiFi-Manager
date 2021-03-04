#include <qqqWiFiManager.h>

#define CONF_NAME "qqq"

typedef struct {
  String ssid;
  String pw;
} config_t;
config_t config;

//WiFiManager callback to presist SSID and password
void WiFiManager_saveCredentials() {
  Serial.printf("WiFiManager_saveCredentials ssid=%s\n",WiFiManager.wifiSSID);
  config.ssid = WiFiManager.wifiSSID;
  config.pw = WiFiManager.wifiPassword;  
  config_save();
}  
  
void setup() {
  Serial.begin(115200);
  while(!Serial);

  Serial.println("WiFiManager AutoConnect Demo");

  //load config
  config_setup();
  
  //config.ssid = ""; config_save(); //remove ssid (for testing)

  //start WiFiManager
  WiFiManager.portalSSID = "qqqlab";
  WiFiManager.portalPassword = "12345678";
  WiFiManager.begin(config.ssid, config.pw, WiFiManager_saveCredentials);
  //WiFiManager.waitConnected(); //wait for connection (optional)
}

void loop() {
  static int cnt;
  
  WiFiManager.loop();

  Serial.print("loop:");
  Serial.println(++cnt); 
  delay(1000); 
}


//===========================================================================
// config (Preferences)
//===========================================================================
#include <Preferences.h>

void config_setup() {
  Preferences prefs;
  prefs.begin(CONF_NAME, true); //readonly
  config.ssid        = prefs.getString("ssid", "");
  config.pw          = prefs.getString("pw", "");
  prefs.end();  
}

void config_save() {
  Preferences prefs;
  prefs.begin(CONF_NAME, false); //readwrite
  prefs.putString("ssid",        config.ssid);
  prefs.putString("pw",          config.pw);
  prefs.end();
}
