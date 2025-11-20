## Description

This document outlines the configuration parameters for Sensor AR0820, including validated settings and their compatibility across supported platforms. The table below summarizes the configurations that have been tested and verified, providing a reference for platform-specific implementation and integration.

## Requirement for Kernel Compilation on Sensor

1. Download linux-intel-lts kernel from https://github.com/intel/linux-intel-lts:
   - branch: 6.12/linux
   - tag: lts-v6.12.36-linux-250711T071314Z 

2. Apply patches below:
    1. 0001-serdes-add-fsin-gpio.patch
    2. 0001-Configure-max9296-with-4-lanes-setting.patch
    3. 0001-Add-driver-support-for-AR0820-sensor.patch

3. Enable sensor compilation in `<kernel>/.config`
   ```conf
   CONFIG_VIDEO_AR0820=m
   ```

#### For Sensor Type: GMSL
1. Update new pdata serdes physcial address into **PCA_00C003084()** in `<kernel>/drivers/media/i2c/max9x/serdes.c`
   ```c
   struct max9x_pdata *ser_pdata = pdata_ser(dev, ser_sdinfo, "max9295", <Physical Address>, virt_addr);
   ```
| Vendor | Physical Address |
|-----|-----|
| Sensing | 0x40 |

2. Update new sensor ISP's physcial address into **PCA_AR0820()** in `<kernel>/drivers/media/i2c/max9x/serdes.c`
   ```c
   pdata_sensor(dev, &ser_pdata->subdevs[0], "ar0820", <Physical Address>, virt_addr);;
   ```
**Note**: Both max9296 and ISP's physical address is set to a default value of 0x40 and 0x6D respectively.

| Vendor | Physical Address |
|-----|-----|
| Sensing | 0x6D |




 
