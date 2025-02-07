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
static const uint8_t COMM_BROADCAST_MAC_ADDR[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define COMM_IS_BROADCAST_ADDR(addr) (memcmp(addr, BROADCAST_MAC_ADDR, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    bool is_inbound;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *buffer;
    size_t buffer_size;
} CommTask_t;

esp_err_t comm_init(void);

void comm_deinit(void);

// Send data to a specific MAC address
esp_err_t comm_send(const uint8_t* buffer, const int64_t buffer_size, const uint8_t des_mac[ESP_NOW_ETH_ALEN]);

// Broadcast data to all peers
esp_err_t comm_broadcast(const uint8_t* buffer, const int64_t buffer_size);

esp_err_t comm_add_peer(const uint8_t *peer_mac_addr, bool encrypt);

esp_err_t comm_remove_peer(const uint8_t *peer_mac_addr);

/**
 * @typedef recv_msg_cb_t
 * @brief A function pointer type for handling messages.
 *
 * This type defines a function pointer that takes a constant pointer to a CommTask_t
 * structure and returns a boolean value indicating the success or failure of the
 * message handling operation.
 *
 * @param task A constant pointer to a CommTask_t structure representing the task to be handled.
 * @return A boolean value indicating the success (true) or failure (false) of the message handling.
 */
typedef esp_err_t (*comm_recv_msg_cb_t)(const CommTask_t* task);

void comm_register_recv_msg_cb(comm_recv_msg_cb_t handler);

void comm_deregister_recv_msg_cb(void);

bool comm_is_peer_exist(const uint8_t *peer_mac_addr);