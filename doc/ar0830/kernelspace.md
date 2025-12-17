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

### AR0830 Firmware
Please obtain the firmware from your respective vendor. Copy said firmware without "<>" into /lib/firmware

```bash
cp <"firmware_file"> /lib/firmware
```
