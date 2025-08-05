## Description

This document provides configuration details for below sensor. The table below will show the validated configurations and on which platforms they are verified.

## AR0830 + AP1302 (Leopard Imaging)


| Platform | Kernel Version |
|----------|----------------|
| MTL      | K6.12          |


| Sensor Configuration Details | Value    |
|-----------------------------|----------------|
| Sensor Model  | AR0830        |
| Sensor Type   | MIPI Direct   |
| ISP Model     | AP1302        |
| Lane                 | 4              |
| Resolution           | 3840x2160      |
| Format               | UYVY           |
| Streaming FPS        | 15             |


### Ipu-Bridge changes

IPU_SENSOR_CONFIG("LIAR0830", 1, 600000000)