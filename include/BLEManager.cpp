#include "BLEManager.hpp"
#include "NetworkUtilities.hpp"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <array>
#include <cstring>
#include <esp_mac.h>
#include <functional>
#include <freertos/ringbuf.h>

static uint8_t ownAddressType;

namespace efhilton::ble
{
    BLEManager::DataCallback_t onData;
    BLEManager::ConnectionStatusCallback_t onConnectionStatus;
    uint16_t conn_handle;
    uint16_t char_handle;

    int BLEManager::onGapEvent(ble_gap_event* event, void* arg)
    {
        switch (event->type)
        {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0)
            {
                conn_handle = event->connect.conn_handle;
                if (onConnectionStatus != nullptr)
                {
                    onConnectionStatus(true);
                }
            }
            else
            {
                ESP_LOGE("BLEManager", "Connection failed; status=%d", event->connect.status);
                bleAdvertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            conn_handle = 0;
            if (onConnectionStatus != nullptr)
            {
                onConnectionStatus(false);
            }
            bleAdvertise();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            onSync();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
        case BLE_GAP_EVENT_MTU:
        case BLE_GAP_EVENT_CONN_UPDATE:
        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        case BLE_GAP_EVENT_LINK_ESTAB:
        case BLE_GAP_EVENT_DATA_LEN_CHG:
        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        case BLE_GAP_EVENT_NOTIFY_TX:
            break;
        default:
            ESP_LOGI(TAG, "Unknown gap event: %d", event->type);
            return 1;
        }
        return 0;
    }

    void BLEManager::bleAdvertise()
    {
        ble_gap_adv_params adv_params{};
        ble_hs_adv_fields fields{};

        memset(&fields, 0, sizeof(fields));

        fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

        fields.tx_pwr_lvl_is_present = 1;
        fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

        const char* name{ble_svc_gap_device_name()};
        fields.name = (uint8_t*)name;
        fields.name_len = strlen(name);
        fields.name_is_complete = 1;

        int rc = ble_gap_adv_set_fields(&fields);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error setting advertisement data; rc=%d", rc);
            return;
        }

        memset(&adv_params, 0, sizeof(adv_params));
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
        rc = ble_gap_adv_start(ownAddressType, nullptr, BLE_HS_FOREVER, &adv_params, onGapEvent, nullptr);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error enabling advertisement; rc=%d", rc);
            return;
        }
    }

    void BLEManager::hostTask(void* arg)
    {
        ESP_LOGI(TAG, "BLE Host Task Started");

        nimble_port_run();

        nimble_port_freertos_deinit();
    }

    void BLEManager::onReset(int reason)
    {
        ESP_LOGI(TAG, "Resetting BLE State. Reason: %d", reason);
    }


    void BLEManager::printStats()
    {
        std::array<uint8_t, 6> addr_val{};
        ble_hs_id_copy_addr(ownAddressType, addr_val.data(), nullptr);
        const std::string serviceUUID = NetworkUtilities::bleUuid128ToGuid(
            NetworkUtilities::reverseUuid(gattServiceBleManagerUuidBytes));
        const std::string characteristicUUID = NetworkUtilities::bleUuid128ToGuid(
            NetworkUtilities::reverseUuid(gattCharacteristicBleManagerUuidBytes));

        ESP_LOGI(TAG, "Device Address: %s", NetworkUtilities::macToString(addr_val.data()).c_str());
        ESP_LOGI(TAG, "Device Address Type: %d", ownAddressType);
        ESP_LOGI(TAG, "Device Name: %s", ble_svc_gap_device_name());
        ESP_LOGI(TAG, "Service UUID: %s", serviceUUID.c_str());
        ESP_LOGI(TAG, "Characteristic UUID: %s", characteristicUUID.c_str());
    }


    void BLEManager::onSync()
    {
        int rc = ble_hs_util_ensure_addr(0);
        assert(rc == 0);

        rc = ble_hs_id_infer_auto(0, &ownAddressType);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
            return;
        }

        printStats();
        bleAdvertise();
    }

    void BLEManager::onGattsRegister(ble_gatt_register_ctxt* ctxt, void* arg)
    {
        char buf[BLE_UUID_STR_LEN];

        switch (ctxt->op)
        {
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGI(TAG, "registered service %s with handle=%d",
                     ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                     ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGI(TAG, "registering characteristic %s with "
                     "def_handle=%d val_handle=%d",
                     ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                     ctxt->chr.def_handle,
                     ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGI(TAG, "registering descriptor %s with handle=%d",
                     ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                     ctxt->dsc.handle);
            break;

        default:
            ESP_LOGI(TAG, "unknown gatts register op: %d", ctxt->op);
        }
    }

    int BLEManager::onCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt* ctxt, void* arg)
    {
        static constexpr int PACKET_LENGTH{20};
        static uint8_t incomingCmd[PACKET_LENGTH];

        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            // Determine the length of the incoming data in the mbuf
            const int len = OS_MBUF_PKTLEN(ctxt->om);

            // Validate that the length doesn't exceed our buffer size
            if (len >= PACKET_LENGTH) // Leave room for the null terminator if needed
            {
                ESP_LOGE(BLEManager::TAG, "Incoming data too large for buffer");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN; // Return length error
            }

            // Extract the data from the mbuf into the incomingCmd buffer
            const int rc = ble_hs_mbuf_to_flat(ctxt->om, incomingCmd, len, nullptr);
            if (rc != 0)
            {
                ESP_LOGE(BLEManager::TAG, "Error extracting data from mbuf: %d", rc);
                return rc; // Return the error code
            }

            // Pass the extracted data to the corresponding callback, if set
            if (onData != nullptr)
            {
                onData(incomingCmd, len); // Trigger the callback with extracted data
            }

            return 0; // Success
        }

        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            ESP_LOGI(BLEManager::TAG, "Read Characteristic Value Request");

            // Append the current value of `incomingCmd` to the response buffer
            int rc = os_mbuf_append(ctxt->om, incomingCmd, sizeof(incomingCmd));
            if (rc != 0)
            {
                ESP_LOGE(BLEManager::TAG, "Error appending data to mbuf: %d", rc);
                return rc; // Return the error code
            }

            return 0; // Success
        }

        // Handle unknown characteristic access cases
        ESP_LOGI(BLEManager::TAG, "Unknown Characteristic Access Request");
        return BLE_ATT_ERR_UNLIKELY;
    }

    void BLEManager::defaultDataOutput(uint8_t* incomingData, int len)
    {
        const std::string incomingCmdString(reinterpret_cast<char*>(incomingData), len);
        ESP_LOGI(BLEManager::TAG, "Received command via Characteristic, value: %s", incomingCmdString.c_str());
    }

    void BLEManager::defaultConnectionCallback(bool connected)
    {
        if (connected)
        {
            ESP_LOGI(TAG, "CONNECTED");
        }
        else
        {
            ESP_LOGI(TAG, "DISCONNECTED");
        }
    }

    int BLEManager::gatt_svr_init()
    {
        ble_svc_gap_init();
        ble_svc_gatt_init();

        static ble_uuid128_t gatt_svr_svc_uuid{};
        gatt_svr_svc_uuid.u = {
            .type = BLE_UUID_TYPE_128
        };
        memcpy(gatt_svr_svc_uuid.value, gattServiceBleManagerUuidBytes.data(), sizeof(gatt_svr_svc_uuid.value));

        static ble_uuid128_t gatt_svr_chr_uuid{};
        gatt_svr_chr_uuid.u = {
            .type = BLE_UUID_TYPE_128
        };
        memcpy(gatt_svr_chr_uuid.value, gattCharacteristicBleManagerUuidBytes.data(), sizeof(gatt_svr_chr_uuid.value));

        ble_gatt_chr_def mainCharacteristic{};
        mainCharacteristic.uuid = &gatt_svr_chr_uuid.u;
        mainCharacteristic.access_cb = onCharacteristicAccess;
        mainCharacteristic.arg = this;
        mainCharacteristic.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY;
        mainCharacteristic.val_handle = &char_handle;

        static ble_gatt_chr_def characteristics[] = {
            mainCharacteristic,
            {} // no more characteristics.
        };

        ble_gatt_svc_def mainService{};
        mainService.type = BLE_GATT_SVC_TYPE_PRIMARY;
        mainService.uuid = &gatt_svr_svc_uuid.u;
        mainService.characteristics = characteristics;

        static const ble_gatt_svc_def gatt_svr_svcs[] = {
            mainService,
            {}, // no more services.
        };

        int rc = ble_gatts_count_cfg(gatt_svr_svcs);
        if (rc != 0)
        {
            return rc;
        }

        rc = ble_gatts_add_svcs(gatt_svr_svcs);
        if (rc != 0)
        {
            return rc;
        }
        return 0;
    }


    void BLEManager::transmissionTask(void* args)
    {
        const auto* bleManager = static_cast<BLEManager*>(args);
        xEventGroupSetBits(bleManager->eventGroup, bleManager->EVENT_TASK_IS_RUNNING);
        ESP_LOGI(TAG, "BLE transmission task started");
        const int bitsToWaitFor = bleManager->EVENT_CONSOLE_DATA_AVAILABLE | bleManager->EVENT_DO_SHUTDOWN;
        while (true)
        {
            const EventBits_t bits = xEventGroupWaitBits(bleManager->eventGroup,
                                                         bitsToWaitFor, true, false,
                                                         portMAX_DELAY);
            if (bits & bleManager->EVENT_DO_SHUTDOWN)
            {
                ESP_LOGI(TAG, "Shutting down transmission thread");
                break;
            }

            if (bits & bleManager->EVENT_CONSOLE_DATA_AVAILABLE)
            {
                char consoleMessage[255]{};
                while (true)
                {
                    const size_t bytesRead = xMessageBufferReceive(bleManager->messageBuffer,
                                                                   consoleMessage, sizeof(consoleMessage),
                                                                   0);
                    if (bytesRead == 0)
                    {
                        break;
                    }
                    putConsoleMessageOnWire(consoleMessage, bytesRead);
                }
            }
        }
        xEventGroupSetBits(bleManager->eventGroup, bleManager->EVENT_TASK_IS_SHUT_DOWN);
    }

    void BLEManager::setupTransmissionThread()
    {
        if (xEventGroupGetBits(eventGroup) & EVENT_TASK_IS_RUNNING)
        {
            ESP_LOGI(TAG, "Transmission thread already running");
            return;
        }

        ESP_LOGI(TAG, "Starting transmission thread");
        xTaskCreate(transmissionTask, TAG, 4096, this, 5, &transmissionTaskHandle);

        ESP_LOGI(TAG, "Waiting for transmission thread to start");
        xEventGroupWaitBits(eventGroup, EVENT_TASK_IS_RUNNING, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Transmission thread started");
    }

    void BLEManager::shutdownTransmissionThread() const
    {
        if (xEventGroupGetBits(eventGroup) & (EVENT_DO_SHUTDOWN | EVENT_TASK_IS_SHUT_DOWN))
        {
            ESP_LOGI(TAG, "Transmission thread already shutting down");
            return;
        }
        ESP_LOGI(TAG, "Shutting down transmission thread");
        xEventGroupSetBits(eventGroup, EVENT_DO_SHUTDOWN);
        ESP_LOGI(TAG, "Waiting for transmission thread to shut down");
        xEventGroupWaitBits(eventGroup, EVENT_TASK_IS_SHUT_DOWN, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Transmission thread shut down");
    }

    void BLEManager::onInitialize(const std::string& deviceName)
    {
        const size_t free_heap = esp_get_free_heap_size();
        ESP_LOGI(TAG, "Available heap memory before BLE initialization: %d bytes", free_heap);

        if (free_heap < 50000)
        {
            ESP_LOGE(TAG, "Insufficient heap memory for BLE initialization: %d bytes available", free_heap);
            return;
        }

        ESP_LOGI(TAG, "Initializing NimBLE stack");
        esp_err_t ret = nimble_port_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "NimBLE port initialization failed: %s", esp_err_to_name(ret));
            return;
        }

        ble_hs_cfg.reset_cb = onReset;
        ble_hs_cfg.sync_cb = onSync;
        ble_hs_cfg.gatts_register_cb = onGattsRegister;
        ble_hs_cfg.gatts_register_arg = this;
        ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
        ble_hs_cfg.sm_io_cap = 3;

        ret = gatt_svr_init();
        if (ret != 0)
        {
            ESP_LOGE(TAG, "GATT server initialization failed: %d", ret);
            return;
        }

        ret = ble_svc_gap_device_name_set(deviceName.c_str());
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Setting BLE device name failed: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Device name set successfully.");

        nimble_port_freertos_init(hostTask);

        setupTransmissionThread();
        ESP_LOGI(TAG, "Bluetooth initialization successfully completed.");
    }

    void BLEManager::onTerminate() const
    {
        shutdownTransmissionThread();

        esp_err_t stop_ret = nimble_port_stop();
        if (stop_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Stopping NimBLE port failed: %s", esp_err_to_name(stop_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE port stopped successfully.");
        esp_err_t deinit_ret = nimble_port_deinit();
        if (deinit_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Deinitializing NimBLE port failed: %s", esp_err_to_name(deinit_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE port deinitialized successfully.");

        esp_err_t hci_deinit_ret = esp_nimble_hci_deinit();
        if (hci_deinit_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Deinitializing NimBLE HCI failed: %s", esp_err_to_name(hci_deinit_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE HCI deinitialized successfully.");

        ESP_LOGI(TAG, "Bluetooth termination completed successfully.");
    }

    void BLEManager::setMacAddress(const std::string& newMacAddress)
    {
        macAddress = newMacAddress;
    }

    const std::string& BLEManager::getMacAddress()
    {
        if (macAddress.empty())
        {
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_BT);
            macAddress = NetworkUtilities::macToString(mac);
        }
        return macAddress;
    }

    void BLEManager::setOnDataCallback(const DataCallback_t& callback)
    {
        if (callback != nullptr)
        {
            onData = callback;
        }
        else
        {
            ESP_LOGI(TAG, "Setting default callback for data output.");
            onData = defaultDataOutput;
        }
    }

    void BLEManager::setConnectionStatusCallback(const ConnectionStatusCallback_t& callback)
    {
        if (callback != nullptr)
        {
            onConnectionStatus = callback;
        }
        else
        {
            ESP_LOGI(TAG, "Setting default callback for connection status.");
            onConnectionStatus = defaultConnectionCallback;
        }
    }

    size_t BLEManager::sendConsoleMessage(const std::string& consoleMessage, const TickType_t maxWaitTimeInTicks) const
    {
        if (messageBuffer == nullptr) { return 0; }
        if (consoleMessage.length() > MAX_SIZE_OF_CONSOLE_MESSAGE)
        {
            throw std::runtime_error(
                "Console message (" + std::to_string(consoleMessage.length()) +
                " characters) is too long. It should be no longer than " + std::to_string(MAX_SIZE_OF_CONSOLE_MESSAGE) +
                " characters.");
        }

        const size_t bytesAdded = xMessageBufferSend(messageBuffer,
                                                     consoleMessage.c_str(), consoleMessage.length(),
                                                     maxWaitTimeInTicks);
        if (bytesAdded == consoleMessage.length())
        {
            xEventGroupSetBits(eventGroup, EVENT_CONSOLE_DATA_AVAILABLE);
        }
        return bytesAdded;
    }

    size_t BLEManager::putConsoleMessageOnWire(const char* consoleMessage, const size_t length)
    {
        const std::string EOF_SEQUENCE {"\r\nEOF\r\n"}; // used to signal that the message is complete.
        if (conn_handle == 0)
        {
            ESP_LOGE(TAG, "No active BLE connection. Notification not sent.");
            return 0;
        }

        const size_t max_len = ble_att_mtu(conn_handle) - 3;
        const std::string message = std::string(consoleMessage, length) + EOF_SEQUENCE;

        size_t charsSent = 0;
        while (charsSent < message.length())
        {
            const size_t len = std::min(message.length() - charsSent, max_len);
            std::string chunk = message.substr(charsSent, len);

            os_mbuf* om = ble_hs_mbuf_from_flat(chunk.c_str(), chunk.length());
            if (!om)
            {
                ESP_LOGE(TAG, "Error: Failed to allocate mbuf for BLE notification.");
                return charsSent;
            }

            // Note that this method frees om internally. This does not make sense to me, but they do.
            // If you try to free om here in our method, then it will cause a lockup next time you try to run this
            // method.
            const int rc = ble_gatts_notify_custom(conn_handle, char_handle, om);
            if (rc != 0)
            {
                ESP_LOGE(TAG, "Error: Failed to send notification (rc=%d).", rc);
                return charsSent;
            }
            charsSent += len;
        }
        return charsSent;
    }
}
