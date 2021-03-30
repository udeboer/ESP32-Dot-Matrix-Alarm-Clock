/* Copyright (C) 2020 Udo de Boer <udo.de.boer1@gmail.com>
 * 
 * MIT Licensed as described in the file LICENSE
 */


#ifndef NETWORKSTART_H_
#define NETWORKSTART_H_

// Network startup and configaration changes are done within a FreeRTOS task.
void start_network_task();

// Start of apps after network is started
void start_network_apps();

// Stop network apps before network is restarted
void stop_network_apps();

// Send an event to the network configuration taks to reconfigure the network. 
// It uses the setupparams struct with all the params
void wifi_change_event();

// Simple call to open up the ESP AP. Is used for last resort network setup.
// This resets some parameters in setupparams
void open_wifi_ap();
#endif
