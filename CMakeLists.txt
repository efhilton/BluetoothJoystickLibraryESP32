idf_component_register(
        SRCS
        "src/BLEManager.cpp"
        "src/BLEJoystick.cpp"

        INCLUDE_DIRS
        "include"

        REQUIRES
        "driver"
        "bt"
        "esp_common"
        "nvs_flash"
        "esp_hw_support"
        "esp_netif"
)
