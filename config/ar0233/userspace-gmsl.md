## Description

This document provides configuration details for **ar0233_intel** sensor. The BIOS configuration, camera XML file setup, and userspace commands for this sensor is using the **AR0820 GMSL sensor's** setup. Please refer to `config/ar0820/userspace-gmsl.md` for complete configuration details.

**Important:** Before using the AR0233 sensor, modify the sensor's XML file resolution value to **1920x1080** to match the AR0233's supported resolution.

## Sample Userspace Command

#### Sensor Device Selection

| Sensor Number | Command Pipeline |
|----------|----------|
| 1 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=**ar0820-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |
| 2 | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=**ar0820-2** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |

**Note**: Refer to icamerasrc device-name property for more sensor details.

#### Frame Buffer Memory Type (IO Mode) Selection

| IO Mode | Command Pipeline |
|----------|----------|
| MMAP | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0820-1 printfps=true io-mode=**1** ! '**video/x-raw,format=UYVY**,width=1920,height=1080' ! glimagesink sync=false |
| DMA MODE | gst-launch-1.0 icamerasrc num-buffers=-1 num-vc=1 scene-mode=normal device-name=ar0820-1 printfps=true io-mode=**4** ! '**video/x-raw(memory:DMABuf),drm-format=UYVY**,width=1920,height=1080' ! glimagesink sync=false |

**Note**: Refer to icamerasrc io-mode property for more sensor details.

#### Number of Stream (Single Stream / Multi Stream) Selection

| Number of Stream | Command Pipeline |
|----------|----------|
| x1 | gst-launch-1.0 icamerasrc num-buffers=-1 **num-vc=1** scene-mode=normal device-name=**ar0820-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |
| x2 | gst-launch-1.0 icamerasrc num-buffers=-1 **num-vc=2** scene-mode=normal device-name=**ar0820x2-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink icamerasrc num-buffers=-1 **num-vc=2** scene-mode=normal device-name=**ar0820x2-2** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |
| x4 | gst-launch-1.0 icamerasrc num-buffers=-1 **num-vc=4** scene-mode=normal device-name=**ar0820x4-1** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink icamerasrc num-buffers=-1 **num-vc=4** scene-mode=normal device-name=**ar0820x4-2** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink icamerasrc num-buffers=-1 **num-vc=4** scene-mode=normal device-name=**ar0820x4-3** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink icamerasrc num-buffers=-1 **num-vc=4** scene-mode=normal device-name=**ar0820x4-4** printfps=true io-mode=4 ! 'video/x-raw(memory:DMABuf),drm-format=UYVY,width=1920,height=1080' ! glimagesink sync=false |

#### FPS Result

| Number of Stream | IO Mode | FPS Result |
|----------|----------|----------|
| x1 | MMAP | 30 |
| x1 | DMA MODE | 30 |