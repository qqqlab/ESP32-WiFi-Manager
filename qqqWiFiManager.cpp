/*
  qqqWiFiManager.cpp - Dead simple ESP32 WiFi Manager (captive portal)

  Copyright (c) 2021 qqqlab.com. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/

#include "qqqWiFiManager.h"

WiFiManagerClass::WiFiManagerClass() {
  portalIP.fromString("192.168.1.1");
}

WiFiManagerClass::StatusEnum WiFiManagerClass::status() {
  return _status;
}

//connect to wifi or start portal
void WiFiManagerClass::begin(String wifiSSID, String wifiPassword, THandlerFunction saveHandler) {
  this->wifiSSID = wifiSSID;
  this->wifiPassword = wifiPassword;
  _saveHandler = saveHandler;

  if(wifiSSID == "") {
    Serial.println("WiFiManager.loop: can't connect, SSID not set");
    startPortal();
  }else{
    //start wifi
    Serial.println("WiFiManager.loop: Connecting to " + wifiSSID + " ");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    if(wifiHostname!="") WiFi.setHostname(wifiHostname.c_str());
    _startConnectingMillis = millis();
    _status = CONNECTING;
  }
}

WiFiManagerClass::StatusEnum WiFiManagerClass::loop() {
  switch(_status) {

    //=====================================      
    //connect to wifi, wait for connect_timeout then start portal
    case CONNECTING:
      //wait for connect
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFiManager.loop: Connected");
        _status = CONNECTED;
      }else if(connectingTimeout > 0 && millis() - _startConnectingMillis > (connectingTimeout * 1000)) {
        startPortal();
      }    
      break;

    //=====================================
    //if disconnected, wait for DROPPED_TIMEOUT then reboot   
    case CONNECTED:         
      if(WiFi.status() == WL_CONNECTED) {
        _lastConnectedMillis = millis(); //reset timer if connected
      }else if(lostConnectionTimeout > 0 && millis() - _lastConnectedMillis > (lostConnectionTimeout * 1000)) {
        _reboot("WiFiManager.loop: Connection dropped");
      }
      break;

    //=====================================
    //wait for portal_timeout or new wifi credentials, then reboot
    case PORTAL: 
      //process dns & webserver events
      dnsServer.processNextRequest();
      webServer.handleClient(); //this will call saveHandler + reboot upon posting http form with a new ssid + pw
      //reboot on portal_timeout if nobody connected to portal (don't reboot if no SSID is stored)
      if(wifiSSID != "" && portalTimeout > 0 && millis() - _startPortalMillis > (portalTimeout * 1000)) {
        if(WiFi.softAPgetStationNum() > 0) {
          _startPortalMillis = millis(); //reset timer if client is connected to portal
        }else{
          _reboot("WiFiManager.loop: Captive portal timeout");
        }
      }
      break;
  }   
  return _status;
}

void WiFiManagerClass::waitConnected() {
  while(StatusEnum::CONNECTED != WiFiManager.loop());
}

//start captive portal
void WiFiManagerClass::startPortal(THandlerFunction notFoundHandler) { 
  Serial.println("Starting captive portal");

  //wifi 
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.mode(WIFI_AP);
  //delay(2000); //needed, otherwise crash on connect in 1.0.4 - appears to work without this delay in 1.0.5
  WiFi.softAP(portalSSID.c_str(),portalPassword.c_str());
  WiFi.softAPConfig(portalIP, portalIP, IPAddress(255, 255, 255, 0)); //1.0.5 does not always set WiFi.softAPIP() correctly, solved by using WiFi.softAPIP() for DNS
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP()); //sometimes this is not the IP set in WiFi.softAPConfig ...
  Serial.println("portal started");

  //dns
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP()); //"*" reply this IP to all queries
  Serial.println("DNS server started");

  //setup webserver
  if(notFoundHandler) {
    webServer.onNotFound(notFoundHandler);
  }else{
    webServer.onNotFound(_handleNotFound);
  }
  webServer.begin(80);
  Serial.println("webserver started");
  Serial.println("captive portal started");

  _startPortalMillis = millis();
  _status = PORTAL;
}

//list available wifi networks
String WiFiManagerClass::_scan()
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

//static webserver callback
void WiFiManagerClass::_handleNotFound() {
  //handle form data, reboot to start wifi
  String ssid = WiFiManager.webServer.arg("ssid");
  String pw = WiFiManager.webServer.arg("pw");
  if(ssid!="") {
    WiFiManager.wifiSSID = ssid;
    WiFiManager.wifiPassword = pw;
    WiFiManager._saveHandler();
    WiFiManager._reboot("Stored new ssid="+ssid);
  }

  //send html form
  String message;
  message = "<html><body><form>Network<input id=\"ssid\" name=\"ssid\"><br />Password<input id=\"pw\" name=\"pw\" type=\"password\"><br /><input type=\"submit\" value=\"Save\"></form>" 
  + _scan();
  message += "URI: ";
  message += WiFiManager.webServer.uri();
  message += "\nMethod: ";
  message += (WiFiManager.webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += WiFiManager.webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < WiFiManager.webServer.args(); i++) {
    message += " " + WiFiManager.webServer.argName(i) + ": " + WiFiManager.webServer.arg(i) + "\n";
  }
  WiFiManager.webServer.send(200, "text/html", message);
}

void WiFiManagerClass::_reboot(String msg) {
  Serial.println(msg + " --> rebooting...");
  Serial.flush();
  delay(1000); 
  ESP.restart();
  while(1) {delay(1000); yield();}  
}


WiFiManagerClass WiFiManager;