/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */



#ifndef DEFAULTS_H_
#define DEFAULTS_H_

// esp_system.h defines int types. Needed for setup struct
#include "esp_system.h"

// Defaults and system wide struct definitions.
// Global vars are possible here.
//

/* This project uses an AP WiFi configuration that you can set via project
  idf.py configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define DEFAULT_APSSID "mywifissid"
*/

// Define default setup values. In use when nothing is configured
#define DEFAULT_ESPNAME "project001"
#define NVFLASH_NAMESPACE "storage"
#define NVFLASH_SETUPBLOB "setup" // we store the setupparams struct with this name 
//WiFi AP settings 
#ifdef CONFIG_ESP_WIFI_SSID
#define DEFAULT_APSSID CONFIG_ESP_WIFI_SSID  
#else
#define DEFAULT_APSSID "espclock001"   
#endif
#ifdef CONFIG_ESP_WIFI_PASSWORD
#define DEFAULT_APPWD CONFIG_ESP_WIFI_PASSWORD
#else
#define DEFAULT_APPWD "" // if nothing is defined for ap key we have an OPEN AP
#endif
#ifdef ESP_MAX_STA_CONN
#define MAX_STA_CONN ESP_MAX_STA_CONN
#else
#define MAX_STA_CONN 4
#endif
#define DEFAULT_APCHANNEL 1                    // Start with default channel 1 for the AP
// WiFi mode
#ifdef ESP_WIFI_MODE
#define DEFAULT_WIFIMODE ESP_WIFI_MODE         
#else
#define DEFAULT_WIFIMODE 1                     // start always with AP only
#endif
// WiFi client mode
#ifdef ESP_WIFISTA_SSID
#define DEFAULT_WIFISSID ESP_WIFISTA_SSID
#else
#define DEFAULT_WIFISSID "DefaultStation"
#endif
#ifdef ESP_WIFISTA_PASSWORD
#define DEFAULT_WIFIPWD ESP_WIFISTA_PASSWORD
#else
#define DEFAULT_WIFIPWD "DefaultPassword"
#endif
// When in combined AP and wifi client mode we needs this timeout and retry.
// Wifi client reconnect scanning for AP's kills AP mode.
#define WIFI_STA_TIMEOUT 120
#define WIFI_STA_MAX_RETRY 10
#define JSONREADBUFSIZE 4096 // max length of json string
// max length of filename on filesystem. Including VFS path. In our case /www.
#define MAXVFSPATHLENGTH 30
#define TIMEZONE_TZ "CET-1CES-2,M3.5.0/2,M10.5.0/3"   //West Europe timezone info in 2020
#define NTPSERVER "pool.ntp.org"       //Should work al the time
//mDNS hostname is name from setupparams.name. This is an instance name
#define MDNS_INSTANCE_NAME "DotMatrixClock"

// struct type for the wifi setup parameters. Not all parameters are used. Manual 
// setting ip addresses not supported (yet).
typedef struct {
  char name[32];
  char apssid[33];
  char appwd[65];
  int8_t apchannel;
  int8_t wifimode; // 0 no Wifi, 1 AP , 2 APSTA, 3 STA
  char stssid[33];
  char stpwd[65];
  int8_t ipmode; //0 automatic, 1 manual dns, 2 manual everything
  uint8_t ipbyte1; // first byte of ip adress (manual)
  uint8_t ipbyte2;
  uint8_t ipbyte3;
  uint8_t ipbyte4;
  uint8_t maskbyte1; // first byte of ip mask (manual)
  uint8_t maskbyte2;
  uint8_t maskbyte3;
  uint8_t maskbyte4;
  uint8_t gwbyte1; // first byte of ip default route (manual)
  uint8_t gwbyte2;
  uint8_t gwbyte3;
  uint8_t gwbyte4;
  uint8_t dns1byte1; // first byte of first dns server(manual)
  uint8_t dns1byte2;
  uint8_t dns1byte3;
  uint8_t dns1byte4;
  uint8_t dns2byte1; // first byte of second dns server(manual)
  uint8_t dns2byte2;
  uint8_t dns2byte3;
  uint8_t dns2byte4;
  char timezone[40];
  char ntpserver[40];
} setupparams_t;


extern setupparams_t setupparams;
#endif
