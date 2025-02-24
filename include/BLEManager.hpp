#pragma once

#include <functional>
#include <cstring>
#include <string>
#include <host/ble_gap.h>

namespace efhilton::ble
{
    class BLEManager
    {
    public:
        typedef std::function<void(uint8_t*, int)> DataCallback_t;
        typedef std::function<void(bool)> ConnectionStatusCallback_t;

        explicit BLEManager(const std::string& deviceName)
            : messageBuffer(xMessageBufferCreate(MAX_NUMBER_OF_CONSOLE_MESSAGES * MAX_SIZE_OF_CONSOLE_MESSAGE)),
              eventGroup(xEventGroupCreate())
        {
            onInitialize(deviceName);
        }

        ~BLEManager()
        {
            onTerminate();
            vMessageBufferDelete(messageBuffer);
        }

        [[nodiscard]] const std::string& getMacAddress();
        static void setOnDataCallback(const DataCallback_t& callback);
        static void setConnectionStatusCallback(const ConnectionStatusCallback_t& callback);
        size_t sendConsoleMessage(const std::string& consoleMessage,
                                  TickType_t maxWaitTimeInTicks = portMAX_DELAY) const;
        static size_t putConsoleMessageOnWire(const char* consoleMessage, size_t length);

    private:
        static constexpr auto TAG{"BLEManager"};
        static constexpr std::array<uint8_t, 16> gattServiceBleManagerUuidBytes{
            0xf0, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12,
            0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x12, 0xf0
        };

        static constexpr std::array<uint8_t, 16> gattCharacteristicBleManagerUuidBytes{
            0x12, 0x90, 0x78, 0x56, 0xef, 0xcd, 0x12, 0xab,
            0x90, 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab
        };

        const int EVENT_DO_SHUTDOWN{1 << 0};
        const int EVENT_TASK_IS_RUNNING{1 << 1};
        const int EVENT_TASK_IS_SHUT_DOWN{1 << 2};
        const int EVENT_CONSOLE_DATA_AVAILABLE{1 << 3};

        const int MAX_NUMBER_OF_CONSOLE_MESSAGES{3};
        const size_t MAX_SIZE_OF_CONSOLE_MESSAGE{255};

        MessageBufferHandle_t messageBuffer;
        TaskHandle_t transmissionTaskHandle;
        EventGroupHandle_t eventGroup;
        std::string macAddress;

        void setupTransmissionThread();
        void onInitialize(const std::string& deviceName);
        void shutdownTransmissionThread() const;
        void onTerminate() const;
        void setMacAddress(const std::string& newMacAddress);
        static int onGapEvent(ble_gap_event* event, void* arg);
        static void bleAdvertise();
        static void hostTask(void* arg);
        static void onReset(int reason);
        static void printStats();
        static void onSync();
        static void onGattsRegister(ble_gatt_register_ctxt* ctxt, void* arg);
        static int onCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt,
                                          void* arg);
        static void defaultDataOutput(uint8_t* incomingData, int len);
        static void defaultConnectionCallback(bool connected);

        int gatt_svr_init();
        static void transmissionTask(void* args);
    };
}
