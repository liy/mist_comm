#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <esp_random.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_crc.h>
#include "comm.h"

#define ESPNOW_MAXDELAY 512

static const char *TAG = "Comm";

// Queue for sending and receiving data
static QueueHandle_t s_espnow_queue = NULL;

// Register a new message handler
static comm_recv_msg_cb_t s_recv_msg_cb;

static CommTask_t* create_task(const uint8_t* buffer, const int64_t buffer_size, const uint8_t mac_addr[ESP_NOW_ETH_ALEN], bool is_inbound) {
    CommTask_t* task = malloc(sizeof(CommTask_t));
    if (task == NULL) {
        ESP_LOGE(TAG, "Malloc task fail");
        return NULL;
    }

    task->is_inbound = is_inbound;
    task->buffer = malloc(buffer_size);
    memcpy(task->buffer, buffer, buffer_size);
    task->buffer_size = buffer_size;
    memcpy(task->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);

    return task;
}

void comm_register_recv_msg_cb(comm_recv_msg_cb_t cb) {
    s_recv_msg_cb = cb;
}

void comm_deregister_recv_msg_cb(void) {
    s_recv_msg_cb = NULL;
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "Send callback, data to "MACSTR", status: %d", MAC2STR(mac_addr), status);
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t* buffer, int buffer_size) {
    uint8_t *mac_addr = recv_info->src_addr;
    // uint8_t *des_addr = recv_info->des_addr;
    ESP_LOGI(TAG, "Receive data from "MACSTR", len: %d", MAC2STR(mac_addr), buffer_size);

    CommTask_t* task = create_task(buffer, buffer_size, mac_addr, true);
    // Send the structure to the queue, the structure will be cloned.
    // The receiver will be responsible for freeing the data, so it is safe to send pointer into the queue
    if (xQueueSend(s_espnow_queue, &task, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Queue receive task fail");
    }
}

static void task_loop() {
    CommTask_t* task;
    while (xQueueReceive(s_espnow_queue, &task, portMAX_DELAY) == pdTRUE) {
        // Parse the incoming message
        if (task->is_inbound) {
            if(s_recv_msg_cb != NULL) {
                if (!s_recv_msg_cb(task)) {
                    ESP_LOGE(TAG, "Message handler failed");
                }
            } else {
                ESP_LOGW(TAG, "No message handler registered");
            }
        }
        else {
            ESP_LOGI(TAG, "Sending data to "MACSTR", len: %d", MAC2STR(task->mac_addr), task->buffer_size);
            // Send the message
            esp_err_t err = esp_now_send(task->mac_addr, task->buffer, task->buffer_size);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send message to "MACSTR": %s", MAC2STR(task->mac_addr), esp_err_to_name(err));
            }
        }
        free(task->buffer);
        free(task);

// Only print high water mark when optimization is set to no optimization
#if defined(CONFIG_COMPILER_OPTIMIZATION_SIZE) || defined(CONFIG_COMPILER_OPTIMIZATION_PERF)
        // The high water mark value indicates the minimum amount of stack space that has remained unused.
        // If the high water mark is very low (close to 0), it means the task is using almost all of its stack space, and you might need to increase the stack size.
        // If the high water mark is high, you can reduce the stack size to save memory.
        UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        ESP_LOGI(TAG, "High water mark: %d", uxHighWaterMark);
#endif

    }
    vQueueDelete(s_espnow_queue);
    vTaskDelete(NULL);
}

static esp_err_t init_queue() {
    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(CommTask_t*));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Error creating the ESPNOW queue");
    } else {
        ESP_LOGI(TAG, "ESPNOW queue created");
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL));
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    // Start the queue pulling loop
    // Check uxTaskGetStackHighWaterMark to see if the stack size is enough
    xTaskCreate(task_loop, "task_loop", 2400, NULL, 1, NULL);

    return ESP_OK;
}

esp_err_t comm_send(const uint8_t* buffer, const int64_t buffer_size, const uint8_t des_mac[ESP_NOW_ETH_ALEN]) {
    CommTask_t* task = create_task(buffer, buffer_size, des_mac, false);
    if (task == NULL) {
        ESP_LOGE(TAG, "Create task fail");
        return ESP_FAIL;
    }

    // Send the structure to the queue, the structure will be cloned.
    // The receiver will be responsible for freeing the data, so it is safe to send pointer into the queue
    if (xQueueSend(s_espnow_queue, &task, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Queue send task fail");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t comm_broadcast(const uint8_t* buffer, const int64_t buffer_size) {
    return comm_send(buffer, buffer_size, COMM_BROADCAST_MAC_ADDR);
}

esp_err_t comm_init() {
    if(init_queue() != ESP_OK) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
* @brief Clean up queue and deinitialize ESPNOW.
*/ 
void comm_deinit()
{
    vTaskDelete(NULL);
    vSemaphoreDelete(s_espnow_queue);
    esp_now_deinit();
}

esp_err_t comm_add_peer(const uint8_t *peer_mac_addr, bool encrypt) {
    // Add broadcast peer information to peer list
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        // return ESP_FAIL;
        return ESP_
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    // Data encryption configuration
    peer->encrypt = encrypt;
    memcpy(peer->peer_addr, peer_mac_addr, ESP_NOW_ETH_ALEN);
    const esp_err_t err = esp_now_add_peer(peer);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(err));
    }
    free(peer);

    ESP_LOGI(TAG, "Added peer: "MACSTR, MAC2STR(peer_mac_addr));

    return err;
}

bool comm_is_peer_exist(const uint8_t *peer_addr) {
    return esp_now_is_peer_exist(peer_addr);
}

// Function to remove a peer
esp_err_t comm_remove_peer(const uint8_t *peer_addr) {
    const esp_err_t err = esp_now_del_peer(peer_addr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove peer: %s", esp_err_to_name(err));
    }
    return err;
}