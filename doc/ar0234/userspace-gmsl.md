## Description

This document details the configuration settings for the AR0234 GMSL sensor, providing essential information for system integration. The table below presents the key parameters and their respective values used during system setup and validation.

## BIOS Configuration Table

> **Note:** No External Clock required.

#### IPU6EPMTL Camera Option

|                            | Camera1 Link options |
|---                         |---                   |
| Sensor Model               | User Custom          |
| Custom HID                 | INTC0234             |
| Lanes Clock division       | 4 4 2 2              |
| CRD Version                | CRD-D                |
| GPIO control               | No Control Logic     |
| Camera position            | Front                |
| Flash Support              | Driver default       |
| Privacy LED                | Driver default       |
| Rotation                   | 90                   |
| PPR Value                  | 2                    |
| PPR Unit                   | 2                    |
| Camera module name         |                      |
| MIPI port                  | 0                    |
| LaneUsed                   | x2                   |
| MCLK                       | 19200000             |
| EEPROM Type                | ROM_NONE             |
| VCM Type                   | VCM_NONE             |
| Number of I2C Components   | 3                    |
| I2C Channel                | I2C1                 |
| Device 0                   |                      |
| I2C Address                | 48                   |
| Device Type                | Sensor               |
| Device 1                   |                      |
| I2C Address                | 44                   |
| Device Type                | Sensor               |
| Device 2                   |                      |
| I2C Address                | 50                   |
| Device Type                | Sensor               |
| Customize Device ID List   |                      |
| Customize Device ID Number | 17                   |
| Customize Device ID Number | 18                   |
| Customize Device ID Number | 19                   |
| Flash Driver Selection     | Disabled             |

## Camera XML File Setup

#### IPU6EPMTL Configuration

Replace target system with recommended [ipu6epmtl](../../config/ar0234/ipu6epmtl) setting

    sudo cp -r ../../config/ar0234/ipu6epmtl /etc/camera

## Camera Tuning File Setup

#### IPU6EPMTL Configuration

Import [AR0234_TGL_10bits.aiqb](https://github.com/intel/ipu6-camera-hal/blob/iotg_ipu6/config/linux/ipu6epmtl/AR0234_TGL_10bits.aiqb) into target system `/etc/camera/ipu6epmtl`

## Sample Userspace Command

#### Sensor Device Selection

| Sensor Number | Command Pipeline |
|---|---|
| 1 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |
| 2 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-2 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |

> **Note**: Refer to icamerasrc device-name property for more sensor details.

#### Frame Buffer Memory Type (IO Mode) Selection

| IO Mode | Command Pipeline |
|---|---|
| USERPTR | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=userptr ! 'video/x-raw,format=NV12,width=1280,height=960' ! glimagesink sync=false |
| DMA MODE | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |

> **Note**: Refer to icamerasrc io-mode property for more sensor details.

#### Sensor Resolution Selection

| Resolution | Command Pipeline |
|---|---|
| 1280x960 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |

#### Sensor Format Selection

| Format | Command Pipeline |
|---|---|
| NV12 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |

#### Number of Stream (Single Stream / Multi Stream) Selection

| Number of Stream | Command Pipeline |
|---|---|
| x1 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0234-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |
| x2 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=2 scene-mode=normal device-name=ar0234x2-1 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false icamerasrc num-buffers=-1 num-vc=2 scene-mode=normal device-name=ar0234x2-2 printfps=true io-mode=dma_mode ! 'video/x-raw(memory:DMABuf),drm-format=NV12,width=1280,height=960' ! glimagesink sync=false |

#### FPS Result

| Number of Stream | IO Mode  | FPS Result |
|---               |---       |---         |
| x1               | USERPTR  | 30         |
| x1               | DMA MODE | 30         |
| x2               | USERPTR  | 30         |
| x2               | DMA MODE | 30         |

**Note:** Please ensure your system enable support for specified  number of stream before test.
