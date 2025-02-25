# BLE Joystick Library Tester for ESP32

## Overview

This project sets up basic BLE functionality in an ESP32 device. It allows a remote device to connect
and trigger events via toggles, triggers, joysticks. All while receiving simple console outputs. It should be added 
as a git submodule under the `components` folder in your project.

This project:

- was written and maintained with the latest versions of ESP-IDF (v5.4).
- was written in conjunction with its Android counterpart. Please refer to 
 [https://github.com/efhilton/BluetoothJoystick](https://github.com/efhilton/BluetoothJoystick)
 for further information.
- was written in conjunction with a [simple ESP-IDF test project that uses this library and can be used as an example](https://github.com/efhilton/BluetoothJoystickLibraryESP32Test).

## To Use
Follow these steps to add and use this library in your ESP32-IDF project. Whether you use Git or not, we've got you covered:
### Option 1: If Using Git
1. Navigate to your project's `components` folder and add this library as a Git submodule:
``` bash
   cd <your_project_dir>/components
   git submodule add https://github.com/efhilton/BluetoothJoystick ble_joystick
```
2. Initialize and update the submodule to ensure the required files are cloned correctly:
``` bash
   git submodule update --init --recursive
```
3. (Optional) If needed, add the following to your project's `main/idf_component.yml` file to link the library with the ESP-IDF build system:
``` yaml
   dependencies:
     ble-joystick:
       path: ../components/ble-joystick
       version: "1.0.0"
```
4. Build your project to verify integration:
``` bash
   idf.py build
```
### Option 2: If Not Using Git
1. Download the library archive directly from the GitHub repository:
 - Go to [BluetoothJoystick GitHub Repository](https://github.com/efhilton/BluetoothJoystick).
 - Click on the green **Code** button and select **Download ZIP**.
 - Extract the contents of the ZIP file.

2. Manually place the extracted directory into your project's `components` folder. Rename the directory to `ble_joystick` to maintain consistency:
``` 
   <your_project_dir>/components/ble_joystick
```
3. (Optional) Add the following to your `main/idf_component.yml` file if required by your project setup:
``` yaml
   dependencies:
     ble-joystick:
       path: ../components/ble-joystick
       version: "1.0.0"
```
4. Build your project to confirm the library has been integrated successfully:
``` bash
   idf.py build
```
## Examples and Help
For a complete example of how to use the library, refer to the [test project](https://github.com/efhilton/BluetoothJoystickLibraryESP32Test). 

## Questions?

Please contact [Edgar Hilton](mailto://edgar.hilton@gmail.com) if you have any questions or issues.
