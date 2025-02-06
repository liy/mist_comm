/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE           6

// Broadcast MAC address
static const uint8_t BROADCAST_MAC_ADDR[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define IS_BROADCAST_ADDR(addr) (memcmp(addr, BROADCAST_MAC_ADDR, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    bool is_inbound;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *buffer;
    size_t buffer_size;
} task_t;

esp_err_t comm_init(void);

void comm_deinit(void);

// Send data to a specific MAC address
esp_err_t send(const uint8_t* buffer, const int64_t buffer_size, const uint8_t des_mac[ESP_NOW_ETH_ALEN]);

// Broadcast data to all peers
esp_err_t broadcast(const uint8_t* buffer, const int64_t buffer_size);

esp_err_t add_peer(const uint8_t *peer_mac_addr, bool encrypt);

esp_err_t remove_peer(const uint8_t *peer_mac_addr);

/**
 * @typedef message_handler_t
 * @brief A function pointer type for handling messages.
 *
 * This type defines a function pointer that takes a constant pointer to a task_t
 * structure and returns a boolean value indicating the success or failure of the
 * message handling operation.
 *
 * @param task A constant pointer to a task_t structure representing the task to be handled.
 * @return A boolean value indicating the success (true) or failure (false) of the message handling.
 */
typedef bool (*message_handler_t)(const task_t* task);

void register_message_handler(message_handler_t handler);

void deregister_message_handler(void);