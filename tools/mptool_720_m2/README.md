# MPTool2 for KL720 M.2 Manufactory 

MPtool2 for KL720 M.2 test and large model inference test
> Note:
> 	Windows 10/11 only



## 1. Windows Driver Installation

1. Connect Kneron Device to Windows PC
2. open Zadig
3. exectue `zadig-2.5.exe` by double clicking in Windows File Explorer 
4. Click "**Options -> List All Devices**" to list all devices
5. Click "*KL720*" from the drop-down list to select Kneron Device
6. Select "**WinUSB(v6.1.7600.16385)**" and then click "**Install Driver**" to install driver

> Note for #2: 
>
> ​	There is a copy, *{mptool2}\thirdparty\zadig-2.5.exe*", or you can download from https://zadig.akeo.ie/
>
> Note for #5:
>
> ​	"*Unknown device#1*" with USB VID/PID(3231/0200) or (3231/0720) might be shown 





## 2. Run MPtool2

MPtool2 is a Windows BAT script to execute programs under `res/bin`



Start with Windows Command prompt by pressing '**win key+ R**', then type '**cmd**'

```
C:\> cd /d c:\{path_to_mptool2}

C:\mptool2> mptool2.bat -h

Usage: mptool2.bat [-sd] [-i {index}] [-l {number}] [-h]
-sd        : Show device
-d         : Test M.2 dual chip
-i index   : Test the specified device by index
            optional, default is -1: all devices
-l number  : Set loop count
            optional, default is 1
-h         : Show this help message
```

---

### 2.1 Scan Device 

To scan connected devices  


```
C:\> cd /d c:\{path_to_mptool2}

C:\mptool2> mptool2.bat -sd

[info] Executing scan_device.exe to show devices

scanning kneron devices ...
number of Kneron devices found: 1

listing devices infomation as follows:

[0] scan_index: '0'
[0] port ID: '21'
[0] product_id: '0x720' (KL720)
[0] USB link speed: 'High-Speed'
[0] USB port path: '1-5'
[0] kn_number: '0x2F06284C'
[0] Connectable: 'True'
[0] Firmware: 'KDP2 Comp/F'

```

If **M.2 dual chip** is plugged, `number of Kneron devices found` will be 2.  
Then, you can do inference test for the specific `scan_index` number with `-i` later



---

### 2.2 Inference test

To test inference
```
C:\> cd /d c:\{path_to_mptool2}

C:\mptool2> mptool2.bat
[info] No device index specified, using default index -1(all devices)
[info] Executing generic_inference.exe with index=-1 loop=1

PASS >> Golden patten found in the output

```
#### Check test result

There are 3 Possible results:

* PASS >> Golden patten found in the output
* FAIL >> Golden patten not found in the output
* FAIL >> Device connection check failed

#### Test logs

Naming rule of the log file:

For M.2 Single-chip:

**./test_log/[fail | pass]/{kn_number}\_{date}\_{time}.txt**



For M.2 dual-chip:

**./test_log/[fail | pass]/dual_{kn_number}\_{date}\_{time}.txt**

> Note:
>
> ​	{kn_number} is the kn_number for first chip of module as a representative

```
Example: 

./test_log/
├── fail
│   ├── 0xA80A29CC_20240220_1469990.txt
│   └── dual_0x2F06284C_20240220_3025196.txt
└── pass
    ├── 0xA80A29CC_20240220_1901996.txt
    ├── 0xA80A29CC_20240220_4148395.txt
    ├── dual_0xA80A29CC_20240220_1383596.txt
    └── dual_0xA80A29CC_20240220_4666795.txt
```



---

### 2.3 Other Controls 

#### 2.3.1 Test inference for M.2 dual-chip

```
C:\mptool2> mptool2.bat -d
```

#### 2.3.2 test inference for a specific scanned index got from `scan device`

```
C:\mptool2> mptool2.bat -i 0
```

#### 2.3.3. test inference multiple times

```
Test inference for 100 times

C:\mptool2> mptool2.bat -l 100
```



