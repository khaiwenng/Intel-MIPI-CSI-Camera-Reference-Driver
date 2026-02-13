# Description

This document provides configuration details for **ar0830** sensor. The table below outlines key parameters and their corresponding values used in the system setup. This sensor requires a binary containing the ISP's firmware and Intel **will not** release it alongside the driver. Please obtain it from your respective vendor.

## BIOS Configuration Table

**Note:** No External Clock required.

## IPU6
### BIOS option for MTL platform.

#### Control Logic

| | Control Logic 1 | Control Logic 2 |
|----------|----------|----------|
| Control Logic Type | Discrete | Discrete |
| Number of GPIOs | 1 | 1 |
| Group Pad Number | 23 | 0 |
| Group Number | D_E_F_V | A_B_H_S |
| Com Number | COM0 | COM3 |
| Function | RESET | RESET |
| Active Value | 1 | 1 |
| Initial Value | 1 | 1 |

#### Camera Option

| | Camera Option 1 | Camera Option 2 |
|----------|----------|----------|
| Sensor Model | User Custom | User Custom|
| Custom HID | LIAR0830 | LIAR0830 |
| GPIO Control | Control Logic 1| Control Logic 2|
| MIPI Port| 0 | 4 |
| LaneUsed | x4 | x4 |
| Num of I2C component | 1 | 1 |
| I2C Address | 0x3c | 0x3c |

## XML file
1. Import the files from ipu6/ below to `/etc/camera/ipu6epmtl/sensor`
   - ar0830-1-mipi.xml
   - ar0830-2-mipi.xml

2. Rename the files to 
   - ar0830-1.xml
   - ar0830-2.xml

3. Append the new sensors into `/etc/camera/ipu6epmtl/libcamhal_profile.xml`
   ```xml
   <availableSensors value="...,ar0830-1-0,ar0830-2-4"/>
   ```

## IPU7
### BIOS option for PTL platform.

**Note:** The maximum number of lanes supported on CRD2 port is **2**.

#### Control Logic
| | Control Logic 1 | Control Logic 2 |
|----------|----------|----------|
| Control Logic Type | Discrete | Discrete |
| Number of GPIOs | 1 | 1 |
| Group Pad Number | 10 | 1 |
| Group Number | C_D_E_H | C_D_E_H |
| Com Number | COM1 | COM1 |
| Function | RESET | RESET |
| Active Value | 1 | 1 |
| Initial Value | 0 | 0 |

#### Camera Option
| | Camera Option 1 | Camera Option 2 |
|----------|----------|----------|
| Sensor Model | User Custom | User Custom|
| Custom HID | LIAR0830 | LIAR0830 |
| GPIO Control | Control Logic 1| Control Logic 2|
| MIPI Port| 0 | 2 |
| LaneUsed | x4 | x2 |
| Num of I2C component | 1 | 1 |
| I2C Channel | I2C1 | I2C2 |
| I2C Address | 0x3c | 0x3c |

#### JSON file
1. Import the files from ipu7/ below to `/etc/camera/ipu75xa/sensor`
   - ar0830-1-mipi.json
   - ar0830-2-mipi.json

2. Rename the files to 
   - ar0830-1.json
   - ar0830-2.json

3. Append the new sensors in `/etc/camera/ipu75xa/libcamhal_configs.json`
   ```
   "availableSensors": [ ...,"ar0830-1-0","ar0830-2-2"]"
   ```

## Sensor Device Selection

| MIPI Port | Device Name |
|----------|----------|
| CRD1 | ar0830-1
| CRD2 | ar0830-2

## User Space command

| Number of Stream | Command Pipeline |
|----------|----------|
| x1 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0830-1 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=3840,height=2160' ! glimagesink sync=false |
| x2 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0830-1 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=3840,height=2160' ! glimagesink icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0830-2 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=3840,height=2160' ! glimagesink sync=false |

## FPS Result

| Number of Stream | IO Mode | FPS Result |
|----------|----------|----------|
| x1 | DMA MODE | 15 |
| x2 | DMA MODE | 15 |
