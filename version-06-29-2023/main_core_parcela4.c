/**
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.

 *******************************************************************************
 * @file main_core.c
 * @author Ânderson Ignacio da Silva
 * @date 19 Ago 2016
 * @brief Main code to test MQTT-SN on Contiki-OS
 * @see http://www.aignacio.com
 * @license This project is licensed by APACHE 2.0.
 */

#include "contiki.h"
#include "lib/random.h"
#include "clock.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "mqtt_sn.h"
#include "dev/leds.h"
#include "net/rime/rime.h"
#include "net/ip/uip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint16_t udp_port = 1884;
static uint16_t keep_alive = 5;
static uint16_t broker_address[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};
static struct   etimer time_poll;
static struct   etimer et;
// static uint16_t tick_process = 0;
static char     pub_test[70];
static char     device_id[17];
static char     topic_hw[70];
static char     *topics_mqtt[] = {"/data_1",
                                  "/data_2",
                                  "/data_3",
                                  "/data_4",
                                  "/data_5",
                                  "/data_6",
                                  "/valve_1",
                                  "/valve_2",
                                  "/valve_3",
                                  "/valve_4",
                                  "/valve_5",
                                  "/valve_6"};
// static char     *will_topic = "/6lowpan_node/offline";
// static char     *will_message = "O dispositivo esta offline";
// This topics will run so much faster than others

mqtt_sn_con_t mqtt_sn_connection;

void mqtt_sn_callback(char *topic, char *message){
  printf("\nMessage received. Topic:%s Message:%s",topic,message);
}

void init_broker(void){
  char *all_topics[ss(topics_mqtt)+1];
  sprintf(device_id,"%02X%02X%02X%02X%02X%02X%02X%02X",
          linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1],
          linkaddr_node_addr.u8[2],linkaddr_node_addr.u8[3],
          linkaddr_node_addr.u8[4],linkaddr_node_addr.u8[5],
          linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  sprintf(topic_hw,"Hello addr:%02X%02X",linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);

  mqtt_sn_connection.client_id     = device_id;
  mqtt_sn_connection.udp_port      = udp_port;
  mqtt_sn_connection.ipv6_broker   = broker_address;
  mqtt_sn_connection.keep_alive    = keep_alive;
  //mqtt_sn_connection.will_topic    = will_topic;   // Configure as 0x00 if you don't want to use
  //mqtt_sn_connection.will_message  = will_message; // Configure as 0x00 if you don't want to use
  mqtt_sn_connection.will_topic    = 0x00;
  mqtt_sn_connection.will_message  = 0x00;

  mqtt_sn_init();   // Inicializa alocação de eventos e a principal PROCESS_THREAD do MQTT-SN

  size_t i;
  for(i=0;i<ss(topics_mqtt);i++)
    all_topics[i] = topics_mqtt[i];
  //all_topics[i] = topic_hw;

  mqtt_sn_create_sck(mqtt_sn_connection,
                     all_topics,
                     ss(all_topics),
                     mqtt_sn_callback);
  mqtt_sn_sub("/valve_4",1);
}

unsigned short temp = 20;
unsigned short humedad = 24;
unsigned short pH = 7;
unsigned long sen_data = 0;

/*---------------------------------------------------------------------------*/
PROCESS(init_system_process, "[Contiki-OS] Initializing OS");
AUTOSTART_PROCESSES(&init_system_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_system_process, ev, data) {
  PROCESS_BEGIN();

  debug_os("Initializing the MQTT_SN_DEMO");

  init_broker();

  etimer_set(&et, CLOCK_SECOND*5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  etimer_set(&time_poll, CLOCK_SECOND*10);

  while(1) {
      PROCESS_WAIT_EVENT();
      
      temp += (random_rand() % 3) - 1; // Variar la temperatura actual de forma aleatoria [-1:1]
      humedad += (random_rand() % 3) - 1; // Variar la humedad actual de forma aleatoria [-1:1]
      pH += (random_rand() % 3) - 1; // Variar el pH actual de forma aleatoria [-1:1]
      sen_data = 0; // Resetear información de sensores

      if (temp < 0)
      {
        temp = 0;
      }
      else if (temp > 40)
      {
        temp = 40;
      }

      if (humedad < 0)
      {
        humedad = 0;
      }
      else if (humedad > 35)
      {
        humedad = 35;
      }
      
      if (pH < 0)
      {
        pH = 0;
      }
      else if (pH > 14)
      {
        pH = 14;
      }

      sen_data = ((((sen_data | temp) << 8) | humedad) << 8) | pH;

      //printf("\nPRUEBA:%lu",sen_data);
      sprintf(pub_test,"%lu",sen_data);
      mqtt_sn_pub("/data_4",pub_test,true,1);
      printf("\nMessage sent. Topic: /data_4 Message:%s",pub_test);
      // debug_os("State MQTT:%s",mqtt_sn_check_status_string());
      if (etimer_expired(&time_poll))
        etimer_reset(&time_poll);
  }
  PROCESS_END();
}
