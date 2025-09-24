## Description

This document provides configuration details for **isx031** sensor. The table below outlines key parameters and their corresponding values used in the system setup.

## BIOS Configuration Table

**Note:** No External Clock required.

#### IPU6EP Camera Option

| | Camera Option 1 | Camera Option 2 |
|----------|----------|----------|
| Sensor Model | User Custom | User Custom |
| Custom HID | INTC3031 | INTC3031 |
| GPIO Control | Control Logic 1 | Control Logic 2 |
| MIPI Port| 1 | 2 |
| LaneUsed | x4 | x4 |
| Num of I2C component | 1 | 1 |
| I2C Address | 0x1A | 0x1A |

#### IPU6EP Control Logic

| | Control Logic 1 | Control Logic 2 |
|----------|----------|----------|
| Control Logic Type | Discrete | Discrete |
| Number of GPIOs | 1 | 1 |
| Group Pad Number | 5 | 15 |
| Group Number | F | E |
| Function | RESET | RESET |
| Active Value | 1 | 1 |
| Initial Value | 0 | 0 |

#### IPU6EPMTL Camera Option

| | Camera Option 1 | Camera Option 2 |
|----------|----------|----------|
| Sensor Model | User Custom | User Custom |
| Custom HID | INTC3031 | INTC3031 |
| GPIO Control | Control Logic 1 | Control Logic 2 |
| MIPI Port| 0 | 4 |
| LaneUsed | x4 | x4 |
| Num of I2C component | 1 | 1 |
| I2C Address | 0x1A | 0x1A |

#### IPU6EPMTL Control Logic

| | Control Logic 1 | Control Logic 2 |
|----------|----------|----------|
| Control Logic Type | Discrete | Discrete |
| Number of GPIOs | 1 | 1 |
| Group Pad Number | 23 | 0 |
| Group Number | D_E_F_V | A_B_H_S |
| Function | RESET | RESET |
| Active Value | 1 | 1 |
| Initial Value | 0 | 0 |

## Camera XML File Setup

#### IPU6EP Configuration

1. Import files below to `/etc/camera/ipu6ep/sensor`
   - isx031-1-mipi.xml
   - isx031-2-mipi.xml

2. Append new sensors in `/etc/camera/ipu6ep/libcamhal_profile.xml`
   ```xml
   <availableSensors value="...,isx031-1-mipi-1,isx031-2-mipi-2"/>
   ```

#### IPU6EPMTL Configuration

1. Import files below to `/etc/camera/ipu6epmtl/sensor`
   - isx031-1-mipi.xml
   - isx031-2-mipi.xml

2. Append new sensors into `/etc/camera/ipu6epmtl/libcamhal_profile.xml`
   ```xml
   <availableSensors value="...,isx031-1-mipi-0,isx031-2-mipi-4"/>
   ```

## Sample userspace command

#### Sensor device selection

| MIPI Port | Command Pipeline |
|----------|----------|
| CRD1 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=**isx031-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false
| CRD2 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=**isx031-2** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false

**Note**: Refer to icamerasrc device-name property for more sensor details.

#### Frame Buffer Memory Type (IO Mode) selection

| IO Mode | Command Pipeline |
|----------|----------|
| MMAP | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=isx031-1 printfps=true io-mode=**1** ! '**video/x-raw,format=UYVY**,width=1920,height=1080' ! glimagesink sync=false | 30 |
| DMA MODE | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=isx031-1 printfps=true io-mode=**4** ! '**video/x-raw(memory:DMABuf),drm-format=UYVY**,width=1920,height=1080' ! glimagesink sync=false | 30 |

**Note**: Refer to icamerasrc io-mode property for more sensor details.

#### Number of Stream (Single Stream / Multi Stream)

| Number of Stream | Command Pipeline |
|----------|----------|
| x1 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=**isx031-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |
| x2 | gst-launch-1.0 icamerasrc num-buffers=-1 scene-mode=normal device-name=**isx031-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink icamerasrc num-buffers=-1 scene-mode=normal device-name=**isx031-2** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |

#### FPS result

| Number of Stream | IO Mode | FPS Result |
|----------|----------|----------|
| x1 | MMAP | 30 |
| x1 | DMA MODE | 30 |
| x2 | DMA MODE | 30 |
