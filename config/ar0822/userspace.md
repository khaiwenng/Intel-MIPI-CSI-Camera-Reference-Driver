# Description

This document provides configuration details for **AR0822** sensor. The table below outlines key parameters and their corresponding values used in the system setup. This sensor requires a binary containing the ISP's firmware and Intel **will not** release it alongside the driver. Please obtain it from your respective vendor.

## BIOS Configuration Table

**Note:** No External Clock required.

## IPU7
### BIOS option for PTL platform.

**Note:** The maximum number of lanes supported on CRD2 port is **2**.

#### Control Logic
| | Control Logic 2 |
|----------|----------|
| Control Logic Type |  Discrete |
| Number of GPIOs | 1 |
| Group Pad Number | 1 |
| Group Number | C_D_E_H |
| Com Number | COM1 |
| Function | RESET |
| Active Value | 1 |
| Initial Value | 0 |

#### Camera Option
| | Camera Option 2 |
|----------|----------|
| Sensor Model | User Custom |
| Custom HID | EV8MOOM1 |
| GPIO Control | Control Logic 2|
| MIPI Port| 2 |
| LaneUsed | x2 |
| Num of I2C component | 1 |
| I2C Channel | I2C2 |
| I2C Address | 0x3D |

#### JSON file
1. Import the files from ipu7/ below to `/etc/camera/ipu75xa/sensor`
   - ar0822-2.json

2. Append the new sensors in `/etc/camera/ipu75xa/libcamhal_configs.json`
   ```
   "availableSensors": [ ...,"ar0822-2-2"]"
   ```

## Sensor Device Selection

| MIPI Port | Device Name |
|----------|----------|
| CRD2 | ar0822-2 |

## User Space command

| Number of Stream | Command Pipeline |
|----------|----------|
| x1 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0822-2 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=3840,height=2160' ! glimagesink sync=false |

## FPS Result

| Number of Stream | IO Mode | FPS Result |
|----------|----------|----------|
| x1 | DMA MODE | 15 |
