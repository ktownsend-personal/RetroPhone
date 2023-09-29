/*
  TODO: web server for configuring other app settings
  TODO: set hostname to RetroPhone_867_5309 (need a way to configure the device phone number)

  REF: https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
*/

#include "wifiHandler.h"
#include "WiFi.h"

WiFiManager wm; // https://github.com/tzapu/WiFiManager

void wifiHandler::init(){
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  auto id = String("RetroPhone_") + String(ESP.getEfuseMac());
  connected = wm.autoConnect(id.c_str());
  if(!connected) Serial.println("WiFi config portal running for 60 seconds");
}

void wifiHandler::run(){
  if(!connected) connected = wm.process();  // process non-blocking wifi config portal if not yet connected
}

void wifiHandler::showNetworks(){
  Serial.println("scanning for networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.printf(" %d networks found:\n", n);
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.printf("\t%d: %s (%d)%s\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "" : "*");
      delay(10);
    }
  }
}

String status(){
  switch(WiFi.status()){
    case WL_CONNECTED:        return "Connected";
    case WL_CONNECT_FAILED:   return "Connect Failed";
    case WL_CONNECTION_LOST:  return "Connection Lost";
    case WL_DISCONNECTED:     return "Disconnected";
    case WL_IDLE_STATUS:      return "Idle";
    case WL_NO_SHIELD:        return "No Shield";
    case WL_NO_SSID_AVAIL:    return "No SSID Available";
    case WL_SCAN_COMPLETED:   return "Scan Completed";
    default:                  return "Unknown Status";
  }
}

void wifiHandler::showStatus(){
  Serial.printf("\n\n\tHostname: %s\n", WiFi.getHostname());
  Serial.printf("\tIP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("\tDNS: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("\tGateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("\tSSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("\tRSSI: %d, TX Power: %d\n", WiFi.RSSI(), WiFi.getTxPower());
  Serial.printf("\tMode: %d, Channel: %d\n", WiFi.getMode(), WiFi.channel());
  Serial.printf("\tStatus: %s\n", status().c_str());
}
