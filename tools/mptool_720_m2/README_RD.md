# mptool2 release preparasion

## components in mptool2


* mptool2_vX.X.X.zip
* fw.zip: firmware image - flash_image.bin
* fw_md5.txt

```
mptool2_v2.2.0.zip
├── README.md 
├── mptool2.bat
├── res
│   ├── bin
│   │   ├── generic_command.exe
│   │   ├── generic_inference.exe
│   │   ├── install_driver_windows.exe
│   │   ├── kl720_update_kdp2_to_kdp2_flash_boot.exe
│   │   ├── libkplus.dll
│   │   ├── libstdc++-6.dll
│   │   ├── libusb-1.0.dll
│   │   ├── libwdi.dll
│   │   ├── libwinpthread-1.dll
│   │   ├── output_car_park_barrier_608x608.bmp
│   │   └── scan_devices.exe
│   ├── firmware
│   │   └── KL720
│   │       ├── fw_ncpu.bin    >>> this is for an example only
│   │       └── fw_scpu.bin    >>> this is for an example only
│   ├── golden.txt
│   ├── VERSION
│   ├── images
│   │   └── car_park_barrier_608x608.bmp
│   └── models
│       └── KL720
│           └── YoloV5s_640_640_3
│               └── models_720.nef
└── thirdparty
    └── zadig-2.5.exe
```

## how to generate mptool2.zip
```
$ bash pack.sh -b

Then, "mptool2_v2.2.0.zip" will be generated

# if without '-b', examples binary won't be rebuilt 
```

## Firmware

preare fw.zip 
- *KL720_SDK\firmware\utils\bin_gen\flash_image_solution_kdp2_user_ex.bin*

prepare fw_md5.txt
```bash
md5 flash_image_solution_kdp2_user_ex.bin > fw_md5.txt
``` 

