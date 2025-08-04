## Description

This document provides configuration details for **ar0830_intel** sensor. The table below outlines key parameters and their corresponding values used in the system setup.

## BIOS Configuration Table

**Note:** No External Clock required.

#### Camera Option

| | Camera Option 1 | Camera Option 2 |
|----------|----------|----------|
| Sensor Model | User Custom | User Custom|
| Custom HID | LIAR0830 | LIAR0830 |
| GPIO Control | Control Logic 1| Control Logic 2|
| MIPI Port| 1 | 4 |
| LaneUsed | x4 | x4 |
| Num of I2C component | 1 | 1 |
| I2C Address | 0x3c | 0x3c |

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

## XML file
ar0830_intel-1.xml
ar0830_intel-2.xml

**Note**: Add below lines in <availableSensors> in libcamhal_profile.xml

ar0830_intel-1-0,ar0830_intel-2-4,

## User Space command

gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0830_intel-2 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf), drm-format=UYVY, width=3840,height=2160' ! glimagesink sync=false

**Result**: 15fps


gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=ar0830_intel-1 printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf), drm-format=UYVY, width=3840,height=2160' ! glimagesink sync=false


**Result**: 15fps
