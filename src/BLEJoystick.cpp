#include <BLEManager.hpp>
#include <BleJoystick.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <freertos/ringbuf.h>


namespace efhilton::ble
{
    BLEJoystick::BLEJoystick(const std::string& deviceName) : bleManager(std::make_unique<BLEManager>(deviceName)),
                                                              onTriggersCallback(nullptr),
                                                              onFunctionsCallback(nullptr), onJoysticksCallback(nullptr)
    {
        setOnDisconnectedCallback(nullptr);
        setOnConnectedCallback(nullptr);
        setOnFunctionsCallback(nullptr);
        setOnTriggersCallback(nullptr);
        setOnJoysticksCallback(nullptr);

        BLEManager::setOnDataCallback([this](const uint8_t* incomingData, const int len)
        {
            switch (incomingData[0])
            {
            case 'F':
                onRawFunctionToggled(incomingData, len);
                break;
            case 'T':
                onRawTriggerTriggered(incomingData, len);
                break;
            case 'L':
            case 'R':
                onRawJoystickMotion(incomingData, len);
                break;
            default:
                ESP_LOGI(TAG, "Unknown command: %d", incomingData[0]);
            }
        });
        BLEManager::setConnectionStatusCallback([this](const bool isConnected)
        {
            if (isConnected)
            {
                if (onConnectedCallback != nullptr)
                {
                    onConnectedCallback();
                }
            }
            else
            {
                if (onDisconnectedCallback != nullptr)
                {
                    onDisconnectedCallback();
                }
            }
        });
    }


    BLEJoystick::~BLEJoystick()
    {
        if (bleManager != nullptr)
        {
            BLEManager::setOnDataCallback(nullptr);
            BLEManager::setConnectionStatusCallback(nullptr);
            bleManager.reset();
            bleManager = nullptr;
        }
    }


    void BLEJoystick::setOnTriggersCallback(const std::function<void(const Trigger&)>& callback)
    {
        if (callback != nullptr)
        {
            onTriggersCallback = callback;
        }
        else
        {
            onTriggersCallback = [this](const Trigger& trigger)
            {
                ESP_LOGI(TAG, "Trigger %c(%d) triggered", trigger.trigger, trigger.id);
            };
        }
    }

    void BLEJoystick::setOnFunctionsCallback(const std::function<void(const Function&)>& callback)
    {
        if (callback != nullptr)
        {
            onFunctionsCallback = callback;
        }
        else
        {
            onFunctionsCallback = [this](const Function& function)
            {
                ESP_LOGI(TAG, "Function: %c(%d) is %s", function.function, function.id, function.state ? "ON" : "OFF");
            };
        }
    }

    void BLEJoystick::setOnJoysticksCallback(const std::function<void(const Joystick&)>& callback)
    {
        if (callback != nullptr)
        {
            onJoysticksCallback = callback;
        }
        else
        {
            onJoysticksCallback = [this](const Joystick& joystick)
            {
                ESP_LOGI(TAG, "Joystick: %c (%f, %f)", joystick.joystick, joystick.x, joystick.y);
            };
        }
    }

    void BLEJoystick::setOnConnectedCallback(const std::function<void()>& callback)
    {
        if (callback != nullptr)
        {
            onConnectedCallback = callback;
        }
        else
        {
            onConnectedCallback = [this]()
            {
                ESP_LOGI(TAG, "Connected");
            };
        }
    }

    void BLEJoystick::setOnDisconnectedCallback(const std::function<void()>& callback)
    {
        if (callback != nullptr)
        {
            onDisconnectedCallback = callback;
        }
        else
        {
            onDisconnectedCallback = [this]()
            {
                ESP_LOGI(TAG, "Disconnected");
            };
        }
    }

    void BLEJoystick::onRawFunctionToggled(const uint8_t* incoming_data, const int len) const
    {
        if (incoming_data[0] != 'F' && len != sizeof(Function))
        {
            return;
        }
        auto f = reinterpret_cast<const Function&>(*incoming_data);
        if (onFunctionsCallback != nullptr)
        {
            onFunctionsCallback(f);
        }
    }

    void BLEJoystick::onRawTriggerTriggered(const uint8_t* incoming_data, const int len) const
    {
        if (incoming_data[0] != 'T' || len != sizeof(Trigger))
        {
            return;
        }
        auto t = reinterpret_cast<const Trigger&>(*incoming_data);
        if (onTriggersCallback != nullptr)
        {
            onTriggersCallback(t);
        }
    }

    double BLEJoystick::normalizeJoystickInput(const int16_t value)
    {
        return static_cast<double>(value) / 32768.0;
    }

    void BLEJoystick::onRawJoystickMotion(const uint8_t* incoming_data, const int len) const
    {
        if ((incoming_data[0] != 'L' && incoming_data[0] != 'R'))
        {
            ESP_LOGI(TAG, "Unknown joystick: %d", incoming_data[0]);
            return;
        }
        if (len != sizeof(RawJoystickData))
        {
            ESP_LOGI(TAG, "Joystick data length incorrect: %d, expected %d", len, sizeof(RawJoystickData));
            return;
        }
        RawJoystickData j = *reinterpret_cast<const RawJoystickData*>(incoming_data);
        Joystick joystick{
            .joystick = j.joystick,
            .x = normalizeJoystickInput(j.x),
            .y = normalizeJoystickInput(j.y)
        };
        if (onJoysticksCallback != nullptr)
        {
            onJoysticksCallback(joystick);
        }
    }

    size_t BLEJoystick::sendConsoleMessage(const std::string& message, const TickType_t maxDelayInTicks) const
    {
        return bleManager != nullptr ? bleManager->sendConsoleMessage(message, maxDelayInTicks) : 0;
    }
};
