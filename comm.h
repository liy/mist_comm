/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#define ESPNOW_QUEUE_SIZE           6

// Broadcast MAC address
static const uint8_t COMM_BROADCAST_MAC_ADDR[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define COMM_IS_BROADCAST_ADDR(addr) (memcmp(addr, BROADCAST_MAC_ADDR, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    bool is_inbound;
    // Mac address could be NULL, therefore cannot be declared as an array
    uint8_t* mac_addr;
    uint8_t *buffer;
    size_t buffer_size;
} CommTask_t;

esp_err_t comm_init(void);

void comm_deinit(void);

// Send data to a specific MAC address
esp_err_t comm_send(const uint8_t* buffer, const int64_t buffer_size, const uint8_t* des_mac);

// Broadcast data to all peers
esp_err_t comm_broadcast(const uint8_t* buffer, const int64_t buffer_size);

esp_err_t comm_add_peer(const uint8_t *peer_mac_addr, bool encrypt);

/**
 * @brief Get the list of peers.
 *
 * @param include_broadcast Whether to include the broadcast address in the list.
 * @param peer_num Pointer to store the number of peers.
 * @param peers Pointer to store the dynamically allocated array of MAC addresses. The caller is responsible for freeing the memory.
 * @return ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t comm_get_peers(bool include_broadcast, uint16_t* peer_num, uint8_t** peers);

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

typedef esp_err_t (*comm_send_msg_cb_t)(const uint8_t* mac_addr, esp_now_send_status_t status);

void comm_register_send_msg_cb(comm_send_msg_cb_t handler);

void comm_deregister_send_msg_cb(void);

bool comm_is_peer_exist(const uint8_t *peer_mac_addr);