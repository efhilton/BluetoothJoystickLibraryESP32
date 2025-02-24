idf_component_register(
        SRCS "BLEManager.cpp" "BLEJoystick.cpp"

        INCLUDE_DIRS "include"

        REQUIRES "driver" "bt" "esp_common" "nvs_flash" "esp_hw_support" "esp_netif"
)
