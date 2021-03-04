/*
  qqqWiFiManager.h - Dead simple ESP32 WiFi Manager (captive portal)

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

#ifndef QQQWIFIMANAGER_H
#define QQQWIFIMANAGER_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

class WiFiManagerClass {
public:
  //timeouts, set to 0 to disable
  int connectingTimeout = 20;     //seconds to wait for initial wifi connection before starting captive portal
  int lostConnectionTimeout = 60; //seconds to wait for lost wifi to reconnect before starting captive portal
  int portalTimeout = 300;        //seconds to keep captive portal alive before rebooting and trying to reconnect to wifi

  //wifi credentials
  String wifiSSID = "";
  String wifiPassword = "";
  String wifiHostname = "";

  //portal credentials
  IPAddress portalIP;                 
  String portalSSID = "qqqlab";       
  String portalPassword = "12345678";

  DNSServer dnsServer;
  WebServer webServer;
 
  enum StatusEnum {
    CONNECTING, //attempting connect to wifi
    CONNECTED,  //connected to wifi
    PORTAL      //portal started
  };
  
  typedef std::function<void(void)> THandlerFunction;

  WiFiManagerClass();
  void begin(String wifiSSID, String wifiPassword, THandlerFunction saveHandler);
  StatusEnum loop();
  StatusEnum status();
  void waitConnected();

protected:
  StatusEnum _status = CONNECTING;
  uint32_t _startPortalMillis;     //timestamp start portal
  uint32_t _lastConnectedMillis;   //timestamp last connected state
  uint32_t _startConnectingMillis; //timestamp start connecting 
  
  THandlerFunction _saveHandler;

  void _startPortal();
  static String _scan();
  static void _handleNotFound();
  void _reboot(String msg);

};

extern WiFiManagerClass WiFiManager;

#endif
