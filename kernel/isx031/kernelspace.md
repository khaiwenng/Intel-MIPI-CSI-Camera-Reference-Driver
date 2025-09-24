## Description

This document provides configuration details for the sensor below. The table below will show the validated configurations and on which platforms they are verified.

## ISX031 (D3 Embedded)

| Platform | Kernel Version |
|----------|----------|
| RPL | K6.12 |
| ARL | K6.12 |

| Sensor Details | Value |
|----------|----------|
| Sensor Model | ISX031 |
| Sensor Type | MIPI CSI |

| Configuration Parameter | Value |
|----------|----------|
| Lane | 4 |
| Resolution | 1920x1080 |
| Format | UYVY |
| Streaming FPS | 30 |

## Requirement for Kernel Compilation on Sensor

1. Append line below into **ipu_supported_sensors[]** in `<kernel>/drivers/media/pci/intel/ipu-bridge.c`
   ```xml
   IPU_SENSOR_CONFIG("INTC3031", 1, 300000000)
   ```

2. Append lines below to `<kernel>/drivers/media/i2c/Kconfig`
   ```xml
   config VIDEO_ISX031
        tristate "ISX031 sensor support"
        depends on VIDEO_DEV && I2C
        select VIDEO_V4L2_SUBDEV_API
        depends on MEDIA_CAMERA_SUPPORT
        help
          This is a Video4Linux2 sensor-level driver for ISX031 camera.
   ```

3. Append line below to `<kernel>/drivers/media/i2c/Makefile`
   ```xml
   obj-$(CONFIG_VIDEO_ISX031) += isx031.o
   ```

4. Enable sensor compilation in `<kernel>/.config`
   ```xml
   CONFIG_VIDEO_ISX031=m
   ```
