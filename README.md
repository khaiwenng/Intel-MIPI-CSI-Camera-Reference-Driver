# drivers.camera.scaling.sensor
This repository hosts kernel driver code and user space config file for sensor driver


# Note for subsequent sensor enablements

1. Only add verified kernel driver in kernel directory
2. For each sensor enablement, create new `config/<sensor>` directory
    - Add `userspace.md` specifying BIOS configuration, validation result, platform information
    - Add xml file used

# DKMS Build Instructions

1. Dkms build drivers.camera.scaling.sensor
    ```bash
    cd drivers.camera.scaling.sensor

    sudo dkms remove ipu-camera-sensor/0.1
    sudo rm -rf /usr/src/ipu-camera-sensor-0.1/

    sudo dkms add .
    sudo dkms build -m ipu-camera-sensor -v 0.1
    sudo dkms install -m ipu-camera-sensor -v 0.1
    ```
2. Reboot system
    ```bash
    sudo reboot
    ```
