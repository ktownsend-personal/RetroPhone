/*
  TODO: add *70 for showing connection status with IP, SSID and RSSI (signal strength)
  TODO: web server and mechanism to startup in AP mode to configure wifi connection (is there something existing for this?)
  TODO: web server for configuring other app settings
  TODO: periodically ensure still connected
  TODO: handle wifi events to manage wifi state
  TODO: set hostname to RetroPhone_867_5309 (need a way to configure the device phone number)

  REF: https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
*/

#include "wifiHandler.h"
#include "WiFi.h"

void wifiHandler::init(){
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
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
