# BLE Joystick Library Tester for ESP32

# Overview

This project sets up basic BLE functionality in an ESP32 device. It allows a remote device to connect
and trigger events via toggles, triggers, joysticks. All while receiving simple console outputs. It should be added 
as a git submodule under the `components` folder in your project.

This project:

- was written and maintained with the latest versions of ESP-IDF (v5.4).
- was written in conjunction with its Android counterpart. Please refer to 
 [https://github.com/efhilton/BluetoothJoystick](https://github.com/efhilton/BluetoothJoystick)
 for further information.
- was written in conjunction with a [simple ESP-IDF test project that uses this library and can be used as an example](https://github.com/efhilton/BluetoothJoystickLibraryESP32Test).

# To Use

To use this within your project, do the following. Go into your components folder and add the library.  In this case, we are adding it into `components/ble_joystick`:

```bash
cd <your project dir>/components
git submodule add https://github.com/efhilton/BluetoothJoystick ble_joystick
```

Once added, you may or may not need to add the following to your project's `main/idf_component.yml` file:

```yaml
dependencies:
  ble-joystick:
    path: ../components/ble-joystick
    version: "1.0.0"
```

And that's it! Please take a look at the example code above for further information.

# Questions?

Please contact [Edgar Hilton](mailto://edgar.hilton@gmail.com) if you have any queestions.
