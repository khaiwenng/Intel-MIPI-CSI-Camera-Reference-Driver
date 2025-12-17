## Description

This document outlines the configuration parameters for Sensor AR0822, including validated settings and their compatibility across supported platforms.

## AR0822 + AP1302 (Innodisk)

## Requirement for Kernel Compilation on Sensor

1. Append lines below to `<kernel>/drivers/media/i2c/Kconfig`
   ```xml
    config VIDEO_AR0822
        tristate "Innodisk AR0822 sensor support"
        depends on VIDEO_DEV && I2C
        select VIDEO_V4L2_SUBDEV_API
        depends on MEDIA_CAMERA_SUPPORT
        help
        This is a Video4Linux2 sensor driver for the Innodisk
        AR0822 camera.

        To compile this driver as a module, choose M here: the
        module will be called ar0822.
   ```

2. Append line below to `<kernel>/drivers/media/i2c/Makefile`
   ```xml
   obj-$(CONFIG_VIDEO_AR0822) += ar0822.o
   ```

3. Enable sensor compilation in `<kernel>/.config`
   ```xml
   CONFIG_VIDEO_AR0822=m
   ```

### To enable continuous clock
1. Add '#define CONT_CLK_ENABLE 1 in ar0822.c
