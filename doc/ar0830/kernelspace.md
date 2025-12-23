## Description

This document provides configuration details for the sensor below. The table below will show the validated configurations and on which platforms they are verified.

## AR0830 + AP1302 (Leopard Imaging)

| Platform | Kernel Version |
|----------|----------------|
| MTL      | K6.12          |
| PTL      | K6.17          |


| Sensor Details      | Value         |
|---------------------|--------------|
| Sensor Model        | AR0830       |
| Sensor Type         | MIPI Direct  |
| ISP Model           | AP1302       |

| Configuration Parameter | Value    |
|-------------------------|----------|
| Lane                    | 2/4      |
| Resolution              | 3840x2160|
| Format                  | UYVY     |
| Streaming FPS           | 15       |

### Ipu-Bridge changes
This configuration has been tested with kernel version 6.12, tag: `https://github.com/intel/linux-intel-lts/tree/lts-v6.12.36-linux-250711T071314Z`.

**Steps to apply the patch:**
> 1. Navigate to kernel source directory:
>    ```bash
>    cd linux-intel-lts/
>    ```
> 2. Apply the patch:
>    ```bash
>    git apply drivers.camera.scaling.sensor/patch/v6.12/0001-ar0830-Register-AR0830-HID-into-ipu-bridge.patch
>    ```
> 3. Recompile, install and reboot into the new kernel

### AR030 Firmware
Please obtain the firmware from your respective vendor. Copy said firmware without "<>" into /lib/firmware

```bash
cp <"firmware_file"> /lib/firmware
```
