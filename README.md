# drivers.camera.scaling.sensor

This repository contains reference drivers and configurations for Intel MIPI CSI cameras, supporting various sensor modules and Image Processing Units (IPUs).

## Supported Sensors

| Sensor Name     |Sensor Type | Vendor          | Kernel Version | IPU Version |
|-----------------|------------|-----------------|----------------|-------------|
| AR0233          | GMSL       | Sensing         | K6.12          | IPU6EPMTL   |
| AR0234          | MIPI CSI-2 | D3 Embedded     | K6.12          | IPU6EPMTL   |
| AR0820          | GMSL       | Sensing         | K6.12          | IPU6EPMTL   |
| ISX031          | GMSL       | D3 Embedded     | K6.12          | IPU6EPMTL   |
| ISX031          | GMSL       | Leopard Imaging | K6.12          | IPU6EPMTL   |
| ISX031          | GMSL       | Sensing         | K6.12          | IPU6EPMTL   |
| ISX031          | MIPI CSI-2 | D3 Embedded     | K6.12          | IPU6EPMTL   |

> **Note:** IPU6EPMTL represents MTL and ARL platforms

## Directory Structure

- [drivers/](drivers): Host Linux kernel drivers for supported sensors
- [config/](config): Host middleware configuration
- [doc/](doc): Documentation on Kernel driver dependency, BIOS setting and sample commands for supported sensors
- [patch/](patch): Host Dependent Kernel patches to enable specific sensors
- [include/](include): Header files for driver compilation

## Getting Started Guide

Install Intel BKC image:

1. Select the correct `Getting Started Guide` from table below according to your terget `Platform`.
2. Inside `Getting Started Guide`, follow all the steps in `Getting Started with Ubuntu with Kernel Overlay` section

> **Note:** All collaterals belows can be downloaded in [rdc.intel.com](https://www.intel.com/content/www/us/en/resources-documentation/developer.html) with proper granted access.

| Platform | Getting Started Guide | Software Package |
|---|---|---|
| ARL | [828853](https://www.intel.com/content/www/us/en/secure/content-details/828853/ubuntu-with-kernel-overlay-on-intel-core-ultra-200u-and-200h-series-processors-code-named-arrow-lake-u-h-for-edge-platforms-get-started-guide.html?DocID=828853) | [831484](https://www.intel.com/content/www/us/en/secure/design/confidential/software-kits/kit-details.html?kitId=831484) |
| MTL | [779460](https://www.intel.com/content/www/us/en/secure/content-details/779460/ubuntu-with-kernel-overlay-on-intel-core-mobile-processors-code-named-meteor-lake-u-h-for-edge-platforms-get-started-guide.html?DocID=779460) | [790840](https://www.intel.com/content/www/us/en/secure/content-details/790840/meteor-lake-ps-ubuntu-with-kernel-overlay-software-packages.html?DocID=790840) |
| TWL | [793827](https://www.intel.com/content/www/us/en/secure/content-details/793827/ubuntu-with-kernel-overlay-intel-atom-x7000re-x7000c-x7000fe-processor-series-intel-processor-n150-n250-intel-core-3-processor-n355-for-edge-applications-get-started-guide-amston-lake-mr5-amston-lake-fusa-pv-twin-lake-mr2.html?DocID=793827) | [803960](https://www.intel.com/content/www/us/en/secure/design/confidential/software-kits/kit-details.html?kitId=803960) |
| PTL | [858119](https://edc.intel.com/content/www/us/en/secure/design/confidential/products-and-solutions/processors-and-chipsets/panther-lake-h/with-linux-os-get-started-guide-for-edge-compute-applications/) | [871556](https://www.intel.com/content/www/us/en/secure/design/confidential/software-kits/kit-details.html?kitId=860689) |

Reference: [Intel® IPU6 Enabling Partners Technical Collaterals Advisory](https://www.intel.com/content/www/us/en/secure/content-details/817101/intel-ipu6-enabling-partners-technical-collaterals-advisory.html?DocID=817101)

## Setup Procedure

Clone this repository and checkout to your desired release tag

    export HOME=$(pwd)
    cd $HOME
    git clone https://github.com/intel/Intel-MIPI-CSI-Camera-Reference-Driver.git
    cd Intel-MIPI-CSI-Camera-Reference-Driver
    git checkout <release-tag>
    git submodule update --init --recursive

#### Setup for IPU6 Support

Clone IPU repositories and checkout to below commits

    cd $HOME
    git clone -b iotg_ipu6 https://github.com/intel/ipu6-camera-bins.git
    cd ipu6-camera-bins
    git checkout 0b102acf2d95f86ec85f0299e0dc779af5fdfb81

    cd $HOME
    git clone -b iotg_ipu6 https://github.com/intel/ipu6-camera-hal.git
    cd ipu6-camera-hal
    git checkout a647a0a0c660c1e43b00ae9e06c0a74428120f3a

    cd $HOME
    git clone -b icamerasrc_slim_api https://github.com/intel/icamerasrc.git
    cd icamerasrc
    git checkout 4fb31db76b618aae72184c59314b839dedb42689

Deploy ipu6-camera-bins runtime & development files

    sudo mkdir -p /lib/firmware/intel/ipu
    sudo cp -r $HOME/ipu6-camera-bins/lib/lib* /usr/lib/
    sudo cp -r $HOME/ipu6-camera-bins/lib/firmware/intel/ipu/*.bin /lib/firmware/intel/ipu

    sudo mkdir -p /usr/include /usr/lib/pkgconfig
    sudo cp -r $HOME/ipu6-camera-bins/include/* /usr/include/
    sudo cp -r $HOME/ipu6-camera-bins/lib/pkgconfig/* /usr/lib/pkgconfig/

    for lib in $HOME/ipu6-camera-bins/lib/lib*.so.*; do \
      lib=${lib##*/}; \
      sudo ln -s $lib /usr/lib/${lib%.*}; \
    done

Build ipu6-camera-hal and install built libraries to target system

> **Important:** ipu6-camera-hal & icamerasrc repository must be in same directory.

    cd $HOME
    cp $HOME/ipu6-camera-hal/build.sh .
    sudo chmod +x build.sh

> **Note:** Steps below only required for IPU6EP

    ./build.sh -d --board ipu_adl
    sudo cp -r $HOME/out/ipu_adl/install/etc/* /etc/
    sudo cp -r $HOME/out/ipu_adl/install/usr/include/* /usr/include/
    sudo cp -r $HOME/out/ipu_adl/install/usr/lib/* /usr/lib/

> **Note:** Steps below only required for IPU6EPMTL

    ./build.sh -d --board ipu_mtl
    sudo cp -r $HOME/out/ipu_mtl/install/etc/* /etc/
    sudo cp -r $HOME/out/ipu_mtl/install/usr/include/* /usr/include/
    sudo cp -r $HOME/out/ipu_mtl/install/usr/lib/* /usr/lib/

Configure isys_freq value in target system `/etc/modprobe.d/ipu.conf`

    sudo bash -c 'echo "options intel-ipu6 isys_freq_override=475" >> /etc/modprobe.d/ipu.conf'

#### Setup for IPU7 Support

Clone IPU repositories and checkout to below commits

    cd $HOME
    git clone -b main https://github.com/intel/ipu7-camera-bins.git
    cd ipu7-camera-bins
    git checkout 2ef0857570b2dde3c2072fdacf22fdfff1a89bf2

    cd $HOME
    git clone -b main https://github.com/intel/ipu7-camera-hal.git
    cd ipu7-camera-hal
    git checkout 3b9388ecdb682b6e7e9f57a4192b4612bfb43410

    cd $HOME
    git clone -b icamerasrc_slim_api https://github.com/intel/icamerasrc.git
    cd icamerasrc
    git checkout 4fb31db76b618aae72184c59314b839dedb42689

Deploy ipu7-camera-bins runtime & development files

    sudo mkdir -p /lib/firmware/intel/ipu
    sudo cp -P $HOME/ipu7-camera-bins/lib/lib* /usr/lib/
    sudo cp -r $HOME/ipu7-camera-bins/lib/firmware/intel/ipu/*.bin /lib/firmware/intel/ipu

    sudo mkdir -p /usr/include/ipu7 /usr/lib/pkgconfig
    sudo cp -r $HOME/ipu7-camera-bins/include/* /usr/include/
    sudo cp -r $HOME/ipu7-camera-bins/lib/pkgconfig/* /usr/lib/pkgconfig/

Build ipu7-camera-hal and install built libraries to target system

    cd $HOME/ipu7-camera-hal
    mkdir build
    cd build

    cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_CAMHAL_ADAPTOR=ON \
    -DBUILD_CAMHAL_PLUGIN=ON \
    -DIPU_VERSIONS="ipu7x;ipu75xa" \
    -DUSE_STATIC_GRAPH=ON \
    -DUSE_STATIC_GRAPH_AUTOGEN=ON \

    make
    sudo make install

Build icamerasrc and install built libraries to target system

    cd $HOME/icamerasrc
    export CHROME_SLIM_CAMHAL=ON
    ./autogen.sh
    ./configure --prefix=/usr --enable-gstdrmformat=yes

    make
    sudo make install

#### Continue Setup Procedure

(optional) Setup media-ctl to Version 1.30 for debugging

    sudo apt-get install \
        debhelper doxygen gcc git graphviz \
        libasound2-dev libjpeg-dev libqt5opengl5-dev libudev-dev libx11-dev \
        meson pkg-config qtbase5-dev udev libsdl2-dev libbpf-dev llvm clang \
        libjson-c-dev

    git clone https://github.com/gjasny/v4l-utils.git -b stable-1.30
    cd v4l-utils
    meson build/
    sudo ninja -C build/ install

Build and install `Intel-MIPI-CSI-Camera-Reference-Driver` using DKMS

    cd $HOME/Intel-MIPI-CSI-Camera-Reference-Driver

    sudo dkms remove ipu-camera-sensor/0.1
    sudo rm -rf /usr/src/ipu-camera-sensor-0.1/

    sudo dkms add .
    sudo dkms build -m ipu-camera-sensor -v 0.1
    sudo dkms install -m ipu-camera-sensor -v 0.1

Power cycle the board with sensor connected on-board following steps below:

- Power off the board
- Connect sensor hardware
- Power on the board and sensor (if external power required)

Boot into BIOS menu to setup recommended BIOS configuration

- Look for your sensor userspace.md
    - E.g. ISX031 GMSL using [userspace-gmsl.md](doc/isx031/userspace-gmsl.md)
    - E.g. AR0234 MIPI using [userspace-mipi.md](doc/ar0234/userspace-mipi.md)

- Setup `Camera Option` under section `BIOS Configuration Table`
    - E.g. `IPU6EP Camera Option` for IPU6EP
    - E.g. `IPU6EPMTL Camera Option` for IPU6EPMTL
    - E.g. `IPU75XA Camera Option` for IPU75XA

> **Note:** Control Logic configuration only required for MIPI CSI-2 only

- Setup `Control Logic` under section `BIOS Configuration Table`
    - E.g. `IPU6EP Control Logic` for IPU6EP
    - E.g. `IPU6EPMTL Control Logic` for IPU6EPMTL
    - E.g. `IPU75XA Control Logic` for IPU75XA

Sensor should be enumerated correctly upon boot into OS. Sensor can be verified using media-ctl

    media-ctl -p | grep -ie <sensor>

> **Note:** If sensor not found, recheck sensor hardware physical connection and BIOS configuration

> **Note:** If issue persists, contact Intel IPU Team for support

Setup XML file

- Goto your sensor userspace.md

- Setup XML file under section `Camera XML File Setup`
    - E.g. `IPU6EP Configuration` for IPU6EP
    - E.g. `IPU6EPMTL Configuration` for IPU6EPMTL
    - E.g. `IPU75XA Configuration` for IPU75XA

Export below environment variables or add to shell profile (e.g. `~/.bashrc`)

    export DISPLAY=:0; xhost +
    export GST_PLUGIN_PATH=/usr/lib/gstreamer-1.0
    export LIBVA_DRIVER_NAME=iHD
    export GST_GL_API=gles2
    export GST_GL_PLATFORM=egl
    export LIBVA_DRIVERS_PATH=/usr/lib/x86_64-linux-gnu/dri
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/lib/pkgconfig
    export LD_LIBRARY_PATH=/usr/local/lib/pkgconfig:/usr/local/lib:/usr/lib64:/usr/lib:/usr/lib/x86_64-linux-gnu
    export logSink=terminal
    rm -rf ~/.cache/gstreamer-1.0

Run GStreamer streaming command

- Goto your sensor userspace.md

- Copy and run command based on your use case referring to section `Sample Userspace Command`
    - E.g. `Sensor Device Selection` if to select which sensor for streaming
    - E.g. `Number of Stream (Single Stream / Multi Stream) Selection` if to select single / multi streaming

Enjoy your camera stream!

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on contributing to this project.

## Security

For security concerns, please see [SECURITY.md](SECURITY.md).

## Code of Conduct

This project follows our [Code of Conduct](CODE_OF_CONDUCT.md).
