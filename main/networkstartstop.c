/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


// Wifi functions
//
//
#include <string.h>
#include <sys/param.h>

#include "display_functions.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mdns.h"
#include "sdkconfig.h"

// Own files to include
#include "defaults_globals.h"
#include "eventhandler.h"
#include "networkstartstop.h"
#include "nvramfunctions.h"

//
// Included the start and stop actions for network applications
#include "time_task.h"
#include "webserver.h"

// Set logging tag per module
static const char *TAG = "NetStartStop";

// Wifi FreeRtos events.
const int WIFI_CHANGE_EVENT = BIT0;
static EventGroupHandle_t wifi_events;

// Arguments in the WIFI event handler
typedef struct {
  int wifimode;
  int retry_cnt;
} pass_to_wifi_event_t;

// function declarations
void start_stop_mDNS(bool action);
// End function declarations

// This starts all networks apps. Called from eventhandler
void start_network_apps() {
  ESP_LOGI(TAG, "All network apps will be started");
  start_stop_mDNS(1);
  start_stop_httpd(1);
  start_stop_sntp(1);
}

// This stops all network apps. Called from eventhandler when network is gone.
// Or when the network is restarted in the task
void stop_network_apps() {
  ESP_LOGI(TAG, "All network apps will be stopped");
  ESP_LOGI(TAG, "Stop httpd");
  start_stop_httpd(0);
  ESP_LOGI(TAG, "Stop SNTP");
  start_stop_sntp(0);
  ESP_LOGI(TAG, "Stop mDNS");
  start_stop_mDNS(0);
  ESP_LOGI(TAG, "All network apps are stopped");
  sleep(1);
}

// Function is copied from esp-idf/components/esp_wifi/src/wifi_default.c
// But with the added option to add hostname. This hostname shows in the dhcp
// request.
static esp_netif_t *esp_netif_create_default_wifi_ap_with_hostname(char *hostname) {
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_WIFI_AP();
  esp_netif_t *netif = esp_netif_new(&cfg);
  esp_netif_set_hostname(netif, hostname);
  assert(netif);
  esp_netif_attach_wifi_ap(netif);
  esp_wifi_set_default_wifi_ap_handlers();
  return netif;
}

// Function is copied from esp-idf/components/esp_wifi/src/wifi_default.c with hostname
// as extra option.
static esp_netif_t *esp_netif_create_default_wifi_sta_with_hostname(char *hostname) {
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_WIFI_STA();
  esp_netif_t *netif = esp_netif_new(&cfg);
  esp_netif_set_hostname(netif, hostname);
  assert(netif);
  esp_netif_attach_wifi_station(netif);
  esp_wifi_set_default_wifi_sta_handlers();
  return netif;
}

// WiFi stack is sending events that need to be reacted to.
static void wifi_event_handler(void *from_task, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
  // from_task is used to pass settings to event_handler
  pass_to_wifi_event_t *to_wifi_event = (pass_to_wifi_event_t *)from_task;
  ESP_LOGI(TAG, "Entering event handler");
  if (event_base == WIFI_EVENT) {
    // just some usefull messages on the ESP32 console output
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
      wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
      ESP_LOGW(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
      ESP_LOGW(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
    // end of messages
    if (event_id == WIFI_EVENT_STA_START) {
      ESP_LOGI(TAG, "Entering station start handler");
      to_wifi_event->retry_cnt = 0;  // Reset count for retries of STA start.
      sleep(1);
      esp_wifi_connect();
    }
    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      // Caused by shutdown or temporary loss of connection
      // When shutdown retry_cnt is set to -1. Do not try reconnect
      if (to_wifi_event->retry_cnt >= 0) {
        // we want to connect to wifi
        if (to_wifi_event->retry_cnt < WIFI_STA_MAX_RETRY) {
          ESP_LOGI(TAG, "Before retry to connect to the AP");
          sleep(1);
          esp_wifi_connect();
          to_wifi_event->retry_cnt++;  // increment retry counter
          ESP_LOGI(TAG, "After Retry to connect to the AP");
        } else {
          ESP_LOGI(TAG, "Connect to the AP failed. Try again later");
          // we back off (sleep) and set the countdown to 0
          sleep(WIFI_STA_TIMEOUT);
          if (to_wifi_event->wifimode == 3) {
            ESP_LOGI(TAG, "STA only mode. No network. Stop network apps");
            stop_network_apps();
          }
          to_wifi_event->retry_cnt = 0;  // Reset count for retries of STA start.
          sleep(1);
          esp_wifi_connect();
        }
      }
    }
  }
  if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGW(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
      to_wifi_event->retry_cnt = 0;  // Reset count for retries of STA start.
    }
  }
  // Start network apps with working network
  if ((event_id == IP_EVENT_STA_GOT_IP) || (event_id == WIFI_EVENT_AP_START)) {
    ESP_LOGI(TAG, "Calling start network apps");
    start_network_apps();
    sleep(1);
  }
  // Stop apps when we are sta only. And lose the ip address.
  // AP should never go down on event
  if ((event_id == IP_EVENT_STA_LOST_IP) && (to_wifi_event->wifimode == 3)) {
    ESP_LOGI(TAG, "Calling stop network apps");
    stop_network_apps();
    sleep(1);
  }
}

// Function is started as a task. And waits for events. A task is needed to
// reconfigure wifi from a webpage. Receive event. Sleep a little. In meanwhile web serving is
// finished.
void wifi_setup() {
  // Static allocate the vars. This function will never end.
  static esp_netif_t *netif_ap;
  static esp_netif_t *netif_sta;
  static bool netif_started = 0;
  // For passing data to the WiFi event handler
  static pass_to_wifi_event_t to_wifi_event = {.wifimode = 0, .retry_cnt = 0};
  // wifi_config_t is a union. Hence the declaration below
  static wifi_config_t wifi_config_ap = {
      .ap = {.ssid = "", .password = ""},
  };
  static wifi_config_t wifi_config_sta = {
      .sta = {.ssid = "", .password = ""},
  };
  static wifi_mode_t current_wifi_mode;
  static wifi_init_config_t wifi_cfg;
  // This function is running in seperate task. Never ending while loop
  while (1) {
    // Proceed after a FreeRTOS event.
    // These events can timeout. While loop tests if the correct event is received.
    while ((xEventGroupWaitBits(wifi_events, WIFI_CHANGE_EVENT, true, true, portMAX_DELAY) &
            WIFI_CHANGE_EVENT) == 0) {
      // waiting for event had timeout. But no event received.
      ESP_LOGE(TAG, "Still no wifi setup event");
    }

    ESP_LOGI(TAG, "Wifi setup event received");
    // Sleeping to give other tasks the change to finish their network activity.
    ESP_LOGI(TAG, "Sleep 3 seconds");
    sleep(3);

    // Get current running ESP32 WiFi mode
    ESP_LOGI(TAG, "Entering WiFi setup");
    if (esp_wifi_get_mode(&current_wifi_mode) == ESP_OK) {
      ESP_LOGI(TAG, "Got current WiFi mode");
    } else {
      ESP_LOGI(TAG, "Problems after geting WiFi mode");
      current_wifi_mode = WIFI_MODE_NULL;
    }
    if (current_wifi_mode != WIFI_MODE_NULL) {
      // WiFi was started. Deconfigure WiFi.
      // And afterwards start when needed with new parameters
      // First send stop to all network apps
      stop_network_apps();
      // Disable the function of the sta_disconnect_event. Otherwise it will try connecting
      to_wifi_event.retry_cnt = -1;
      ESP_LOGI(TAG, "WiFi will be stopped");
      sleep(1);
      ESP_LOGI(TAG, "Deregistering event handlers");
      ESP_ERROR_CHECK(
          esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
      ESP_ERROR_CHECK(
          esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler));
      ESP_LOGI(TAG, "Wifi structures cleanup");
      sleep(1);

      if (current_wifi_mode == WIFI_MODE_AP) {
        esp_wifi_deauth_sta(0);
        esp_wifi_stop();
        esp_wifi_deinit();
        ESP_LOGI(TAG, "Stop dhcp server");
        esp_netif_dhcps_stop(netif_ap);
        sleep(1);
        ESP_LOGI(TAG, "Clears wifi AP network");
        esp_wifi_clear_default_wifi_driver_and_handlers(netif_ap);
        sleep(1);
        esp_netif_destroy(netif_ap);
      }
      if (current_wifi_mode == WIFI_MODE_APSTA) {
        // disconnect wifi clients from ESP32
        esp_wifi_deauth_sta(0);
        // disconnect ESP32 from AP
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        ESP_LOGI(TAG, "Stop dhcp server");
        esp_netif_dhcps_stop(netif_ap);
        sleep(1);
        ESP_LOGI(TAG, "Stop dhcp client");
        esp_netif_dhcpc_stop(netif_sta);
        sleep(1);
        ESP_LOGI(TAG, "Clears wifi STA network");
        esp_wifi_clear_default_wifi_driver_and_handlers(netif_sta);
        ESP_LOGI(TAG, "Clears wifi AP network");
        esp_wifi_clear_default_wifi_driver_and_handlers(netif_ap);
        sleep(1);
        esp_netif_destroy(netif_sta);
        esp_netif_destroy(netif_ap);
      }
      if (current_wifi_mode == WIFI_MODE_STA) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
        ESP_LOGI(TAG, "Stop dhcp client");
        esp_netif_dhcpc_stop(netif_sta);
        sleep(1);
        ESP_LOGI(TAG, "Clears wifi STA network");
        esp_wifi_clear_default_wifi_driver_and_handlers(netif_sta);
        sleep(1);
        esp_netif_destroy(netif_sta);
      }
      ESP_LOGI(TAG, "After clear wifi network");
    }

    sleep(2);
    // Now we can start everything again when asked for
    ESP_LOGI(TAG, "Starting wifi network");
    if (setupparams.wifimode > 0) {
      // The event handler needs the mode to run the correct actions
      // for AP or STA or APSTA. Wifimode is static and
      // passed to the eventhandler.
      to_wifi_event.wifimode = setupparams.wifimode;
      // start network structures only one time. Stop not supported yet (v4.2).
      if (netif_started == 0) {
        ESP_ERROR_CHECK(esp_netif_init());
        netif_started = 1;
      }
      // start event loop
      start_stop_default_event_handler(1);
      // setting up the AP
      if (setupparams.wifimode <= 2) {
        // AP or APSTA
        ESP_LOGI(TAG, "Creating netif for AP");
        netif_ap = esp_netif_create_default_wifi_ap_with_hostname(setupparams.name);
        wifi_config_ap.ap.ssid_len = strlen(setupparams.apssid);
        wifi_config_ap.ap.max_connection = MAX_STA_CONN;
        wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        wifi_config_ap.ap.channel = setupparams.apchannel;
        strcpy((char *)wifi_config_ap.ap.ssid, setupparams.apssid);
        strcpy((char *)wifi_config_ap.ap.password, setupparams.appwd);

        if (strlen(setupparams.appwd) == 0) {
          wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
        }
      }
      // setting up the STA
      if (setupparams.wifimode >= 2) {
        // Make sure sta start will try a couple of times
        to_wifi_event.retry_cnt = 0;
        // APSTA or STA
        ESP_LOGI(TAG, "Creating netif for sta");
        netif_sta = esp_netif_create_default_wifi_sta_with_hostname(setupparams.name);
        strcpy((char *)wifi_config_sta.sta.ssid, setupparams.stssid);
        strcpy((char *)wifi_config_sta.sta.password, setupparams.stpwd);
        ESP_LOGI(TAG, "Sta connecting with %s and %s .", wifi_config_sta.sta.ssid,
                 wifi_config_sta.sta.password);
      }
      // starting Wifi
      ESP_LOGI(TAG, "Init config default");
      ESP_LOGI(TAG, "Wifi init with config default");
      // here we initialize the structure with it default values. Need to use the cast
      // to make the WIFI_INT_CONFIG_DEFAULT work.
      wifi_cfg = (wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT();
      // ESP_ERROR_CHECK is used below. If the actions below do not work, do an assert.
      ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

      // events are generate by wifi networking
      ESP_LOGI(TAG, "Registering event handlers");
      ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                                                 &to_wifi_event));
      ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                                                 &to_wifi_event));

      switch (setupparams.wifimode) {
        case 1:
          ESP_LOGI(TAG, "Mode is access-point");
          ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
          ESP_LOGI(TAG, "Wifi set AP config");
          ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
          ESP_LOGI(TAG, "Wifi_init_softap finished. SSID:%s password:%s", wifi_config_ap.ap.ssid,
                   wifi_config_ap.ap.password);
          break;
        case 2:
          ESP_LOGI(TAG, "Mode is access-point and wifi client");
          ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
          ESP_LOGI(TAG, "Wifi set AP and STA config");
          ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
          ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
          ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", wifi_config_ap.ap.ssid,
                   wifi_config_ap.ap.password);
          ESP_LOGI(TAG, "wifi_init_sta finished.");
          break;
        case 3:
          ESP_LOGI(TAG, "Mode is wifi client");
          ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
          ESP_LOGI(TAG, "Wifi set STA config");
          ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
          ESP_LOGI(TAG, "wifi_init_sta finished.");
          break;
      }
      ESP_LOGI(TAG, "Finally wifi starts");
      ESP_ERROR_CHECK(esp_wifi_start());
      // add ipv6 link local
      ESP_LOGI(TAG, "Adding IPv6 link local addresses ");
      if (setupparams.wifimode <= 2) {
        esp_netif_create_ip6_linklocal(netif_ap);
      }
      if (setupparams.wifimode >= 2) {
        esp_netif_create_ip6_linklocal(netif_sta);
      }
    }
  }
}

// calling this function reconfigures the wifi
void wifi_change_event() {
  xEventGroupSetBits(wifi_events, WIFI_CHANGE_EVENT);
}

// wrapper to start network task
void start_network_task() {
  ESP_LOGI(TAG, "Start Wifi event group");
  wifi_events = xEventGroupCreate();
  // wifi is as event task. When wifi has connection to network (event) than
  // it starts the network applications.
  ESP_LOGI(TAG, "Start Wifi task");
  xTaskCreate(&wifi_setup, "WifiSetup", 4096, NULL, 4, NULL);
  ESP_LOGI(TAG, "Start Wifi task");
  // signal Wifi task to change status. And start wifi.
  wifi_change_event();
}

// This functions starts the WIFI AP without password.
// Used for one time configuration
void open_wifi_ap() {
  strcpy(setupparams.appwd, "");
  setupparams.apchannel = 1;
  switch (setupparams.wifimode) {
    case 2:
      break;
    case 3:
      setupparams.wifimode = 2;
      break;
    default:
      setupparams.wifimode = 1;
  }
  ESP_LOGI(TAG, "WIFI open AP will be started");
  // need to write to ram. Still cannot do compleet restart of WIFI
  // without restart.
  write_nvram(&setupparams, sizeof(setupparams_t), NVFLASH_SETUPBLOB);
  wifi_change_event();
}

// Start mDNS (Bonjour)
void start_stop_mDNS(bool action) {
  if (action == 1) {
    ESP_LOGI(TAG, "Start mDNS");
    esp_err_t err = mdns_init();
    if (err) {
      printf("mDNS Init failed: %d\n", err);
      return;
    }
    // set hostname
    mdns_hostname_set(setupparams.name);
    // set default instance
    mdns_instance_name_set(MDNS_INSTANCE_NAME);
    // add our services
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    // mdns_service_instance_name_set("_http", "_tcp", MDNS_INSTANCE_NAME);
    // end of start
  } else {
    // Stop mDNS
    mdns_service_remove_all();
    mdns_free();
    ESP_LOGI(TAG, "mDNS stopped");
  }
}
