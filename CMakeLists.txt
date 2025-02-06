idf_component_register(
    PRIV_REQUIRES esp_event esp_netif esp_wifi
    SRCS "comm.c"
    INCLUDE_DIRS "."
)