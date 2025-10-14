## Description

This document outlines the configuration parameters for Sensor ISX031, including validated settings and their compatibility across supported platforms. The table below summarizes the configurations that have been tested and verified, providing a reference for platform-specific implementation and integration.

## Requirement for Kernel Compilation on Sensor

1. Append lines below to `<kernel>/drivers/media/i2c/Kconfig`
   ```xml
   config VIDEO_ISX031
        tristate "ISX031 sensor support"
        depends on VIDEO_DEV && I2C
        select VIDEO_V4L2_SUBDEV_API
        depends on MEDIA_CAMERA_SUPPORT
        help
          This is a Video4Linux2 sensor-level driver for ISX031 camera.
   ```

2. Append line below to `<kernel>/drivers/media/i2c/Makefile`
   ```xml
   obj-$(CONFIG_VIDEO_ISX031) += isx031.o
   ```

3. Enable sensor compilation in `<kernel>/.config`
   ```xml
   CONFIG_VIDEO_ISX031=m
   ```

#### For Sensor Type: MIPI CSI

1. Append line below into **ipu_supported_sensors[]** in `<kernel>/drivers/media/pci/intel/ipu-bridge.c`
   ```xml
   IPU_SENSOR_CONFIG("INTC3031", 1, 300000000)
   ```

#### For Sensor Type: GMSL

1. Update new pdata serdes physcial address into **PCA_00C003084()** in `<kernel>/drivers/media/i2c/max9x/serdes.c`
   ```xml
   struct max9x_pdata *ser_pdata = pdata_ser(dev, ser_sdinfo, "max9295", <Physical Address>, virt_addr);
   ```
| Vendor | Physical Address |
|-----|-----|
| D3 Embedded | 0x40 |
| Leopard Imaging | 0x62 |
| Sensing | 0x40 |
