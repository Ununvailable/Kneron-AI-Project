/**
 * @file        DFUT_console.cpp
 * @brief
 * @version     0.1
 * @date        2021-12-27
 *
 * @copyright   Copyright (c) 2021 Kneron Inc. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <getopt.h>

#include "KneronDFUT.h"
#include "WarningMessages.h"

#define KL520_UPDATE_PLUS_HELPER                  "../../res/firmware/KL520/fw_scpu.bin"
#define KL520_UPDATE_PLUS_LOADER                  "../../res/firmware/KL520/fw_loader.bin"
#define KL630_UPDATE_PLUS_HELPER                  "../../res/firmware/KL630/kp_firmware.tar"
#define KL730_UPDATE_PLUS_HELPER                  "../../res/firmware/KL730/kp_firmware.tar"
#define KL830_UPDATE_PLUS_HELPER                  "../../res/firmware/KL730/kp_firmware.tar"

using namespace std;

void DisplayHelpMessage()
{
    std::cout << std::endl;
    std::cout << "[Display help message]" << std::endl;
    std::cout << "    --help                : [no argument]         help message" << std::endl;
    std::cout << std::endl;
    std::cout << "[Scan and list all information]" << std::endl;
    std::cout << "    --list                : [no argument]         list all dongles' information" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to usb boot] (Only works for KL520)" << std::endl;
    std::cout << "    --kl520-usb-boot      : [no argument]         choose update to Usb Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to flash boot] (Only works for KL520)" << std::endl;
    std::cout << "    --kl520-flash-boot    : [no argument]         choose update to Flash Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << "    --scpu                : [argument required]   self pointed scpu firmware file path (.bin)" << std::endl;
    std::cout << "    --ncpu                : [argument required]   self pointed ncpu firmware file path (.bin)" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to usb boot] (Only works for KL630)" << std::endl;
    std::cout << "    --kl630-usb-boot      : [no argument]         choose update to Usb Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to flash boot] (Only works for KL630)" << std::endl;
    std::cout << "    --kl630-flash-boot    : [no argument]         choose update to Flash Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << "    --scpu                : [argument required]   self pointed scpu firmware file path (.tar)" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update firmware file to flash memory in dongles] (Only works for KL720)" << std::endl;
    std::cout << "    --kl720-update        : [no argument]         choose write firmware to flash memory" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << "    --scpu                : [argument required]   self pointed scpu firmware file path (.bin)" << std::endl;
    std::cout << "    --ncpu                : [argument required]   self pointed ncpu firmware file path (.bin)" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to usb boot] (Only works for KL730)" << std::endl;
    std::cout << "    --kl730-usb-boot      : [no argument]         choose update to Usb Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to flash boot] (Only works for KL730)" << std::endl;
    std::cout << "    --kl730-flash-boot    : [no argument]         choose update to Flash Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << "    --scpu                : [argument required]   self pointed scpu firmware file path (.tar)" << std::endl;
    std::cout << "[Update dongles to usb boot] (Only works for KL830)" << std::endl;
    std::cout << "    --kl830-usb-boot      : [no argument]         choose update to Usb Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update dongles to flash boot] (Only works for KL830)" << std::endl;
    std::cout << "    --kl830-flash-boot    : [no argument]         choose update to Flash Boot" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << "    --scpu                : [argument required]   self pointed scpu firmware file path (.tar)" << std::endl;
    std::cout << std::endl;
    std::cout << "[Update model file to flash memory in dongles]" << std::endl;
    std::cout << "    --model-to-flash      : [argument required]   self pointed model file path (.nef)" << std::endl;
    std::cout << "    --type                : [argument required]   type of device (\"KL520\", \"KL630\", \"KL720\", \"KL730\" or \"KL830\")" << std::endl;
    std::cout << "    --port                : [argument required]   port id set (\"all\" or specified multiple port ids \"13,537\")" << std::endl;
    std::cout << std::endl;
    std::cout << "[Get Current DFUT console Version]" << std::endl;
    std::cout << "    --version             : [no argument]         display the version of DFUT console" << std::endl;
    std::cout << std::endl;
}

void ParseArguments(int argc, char *argv[], std::unordered_map<char, std::string> &ArgumentMap)
{
    int Option = -1;

    const char* OptionShortString = "h:l:p:s:n";
    struct option OptionList[] = {{"list", no_argument, nullptr, ON_LIST},
                                  {"kl520-usb-boot", no_argument, nullptr, ON_520_USB_BOOT}, {"kl520-flash-boot", no_argument, nullptr, ON_520_FLASH_BOOT},
                                  {"kl630-usb-boot", no_argument, nullptr, ON_630_USB_BOOT}, {"kl630-flash-boot", no_argument, nullptr, ON_630_FLASH_BOOT},
                                  {"kl730-usb-boot", no_argument, nullptr, ON_730_USB_BOOT}, {"kl730-flash-boot", no_argument, nullptr, ON_730_FLASH_BOOT},
                                  {"kl830-usb-boot", no_argument, nullptr, ON_830_USB_BOOT}, {"kl830-flash-boot", no_argument, nullptr, ON_830_FLASH_BOOT},
                                  {"kl520-update", no_argument, nullptr, ON_520_UPDATE}, {"kl720-update", no_argument, nullptr, ON_720_UPDATE},
                                  {"kl630-update", no_argument, nullptr, ON_630_UPDATE_LOADER},
                                  {"kl730-update", no_argument, nullptr, ON_730_UPDATE_LOADER},
                                  {"kl830-update", no_argument, nullptr, ON_830_UPDATE_LOADER},
                                  {"model-to-flash", required_argument, nullptr, ON_FLASH_MODEL},
                                  {"port", required_argument, nullptr, ON_PORT}, {"type", required_argument, nullptr, ON_TYPE},
                                  {"scpu", required_argument, nullptr, ON_SCPU}, {"ncpu", required_argument, nullptr, ON_NCPU},
                                  {"help", no_argument, nullptr, ON_HELP},
                                  {"version", no_argument, nullptr, ON_VERSION}, {"quiet", no_argument, nullptr, ON_QUIET},
                                  {nullptr, no_argument, nullptr, 0}};

    while (-1 != (Option = getopt_long_only(argc, argv, OptionShortString, OptionList, nullptr))) {
        switch (Option) {
            case ON_LIST:
                ScanAndPrintList();
                exit(0);
            case ON_520_USB_BOOT:
                ArgumentMap[ON_520_USB_BOOT] = "true";
                break;
            case ON_520_FLASH_BOOT:
                ArgumentMap[ON_520_FLASH_BOOT] = "true";
                break;
            case ON_520_UPDATE:
                ArgumentMap[ON_520_UPDATE] = "true";
                break;
            case ON_630_USB_BOOT:
                ArgumentMap[ON_630_USB_BOOT] = "true";
                break;
            case ON_630_FLASH_BOOT:
                ArgumentMap[ON_630_FLASH_BOOT] = "true";
                break;
            case ON_630_UPDATE_LOADER:
                ArgumentMap[ON_630_UPDATE_LOADER] = "true";
                break;
            case ON_720_UPDATE:
                ArgumentMap[ON_720_UPDATE] = "true";
                break;
            case ON_730_USB_BOOT:
                ArgumentMap[ON_730_USB_BOOT] = "true";
                break;
            case ON_730_FLASH_BOOT:
                ArgumentMap[ON_730_FLASH_BOOT] = "true";
                break;
            case ON_730_UPDATE_LOADER:
                ArgumentMap[ON_730_UPDATE_LOADER] = "true";
                break;
            case ON_830_USB_BOOT:
                ArgumentMap[ON_830_USB_BOOT] = "true";
                break;
            case ON_830_FLASH_BOOT:
                ArgumentMap[ON_830_FLASH_BOOT] = "true";
                break;
            case ON_830_UPDATE_LOADER:
                ArgumentMap[ON_830_UPDATE_LOADER] = "true";
                break;
            case ON_FLASH_MODEL:
                ArgumentMap[ON_FLASH_MODEL] = optarg;
                break;
            case ON_TYPE:
                ArgumentMap[ON_TYPE] = optarg;
                break;
            case ON_PORT:
                ArgumentMap[ON_PORT] = optarg;
                break;
            case ON_SCPU:
                ArgumentMap[ON_SCPU] = optarg;
                break;
            case ON_NCPU:
                ArgumentMap[ON_NCPU] = optarg;
                break;
            case ON_VERSION:
                DisplayVersion();
                exit(0);
            case ON_QUIET:
                ArgumentMap[ON_QUIET] = "true"; // do not show the confirm message and ask what to do next
                break;
            case ON_HELP:
            default:
                /* help or unknown params */
                DisplayHelpMessage();
                exit(0);
        }
    }
}

int main(int argc, char *argv[])
{
    if (1 == argc){
        /* help or unknown params */
        DisplayHelpMessage();
    }

    std::unordered_map<char, std::string> ArgumentMap;
    std::vector<int> PortIdList;
    int Ret;

    ParseArguments(argc, argv, ArgumentMap);
    Ret = CheckArgument(ArgumentMap);

    if (0 != Ret) {
        return Ret;
    }

    Ret = InstallDriver(ArgumentMap);

    if (0 != Ret) {
        return Ret;
    }

    if ("all" == ArgumentMap[ON_PORT]) {
        if ((false == ArgumentMap[ON_520_USB_BOOT].empty()) || (false == ArgumentMap[ON_520_FLASH_BOOT].empty()) || (false == ArgumentMap[ON_520_UPDATE].empty())) {
            GetPortIdList(PortIdList, KL520_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_630_USB_BOOT].empty()) || (false == ArgumentMap[ON_630_FLASH_BOOT].empty())) {
            GetPortIdList(PortIdList, KL630_PRODUCT_NAME);
        } else if (false == ArgumentMap[ON_720_UPDATE].empty()) {
            GetPortIdList(PortIdList, KL720_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_730_USB_BOOT].empty()) || (false == ArgumentMap[ON_730_FLASH_BOOT].empty())) {
            GetPortIdList(PortIdList, KL730_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_830_USB_BOOT].empty()) || (false == ArgumentMap[ON_830_FLASH_BOOT].empty())) {
            GetPortIdList(PortIdList, KL830_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL520_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
            GetPortIdList(PortIdList, KL520_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL720_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
            GetPortIdList(PortIdList, KL720_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL630_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
            GetPortIdList(PortIdList, KL630_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL730_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
            GetPortIdList(PortIdList, KL730_PRODUCT_NAME);
        } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL830_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
            GetPortIdList(PortIdList, KL830_PRODUCT_NAME);
        }
    } else {
        SplitString(ArgumentMap[ON_PORT], ",", PortIdList);
    }

    if (false == ArgumentMap[ON_520_USB_BOOT].empty()) {
        return UpdateKl520ToUsbLoader(PortIdList, ArgumentMap, KL520_UPDATE_PLUS_LOADER, KL520_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_520_FLASH_BOOT].empty()) {
        return UpdateKl520ToFlashBoot(PortIdList, ArgumentMap, KL520_UPDATE_PLUS_LOADER, KL520_UPDATE_PLUS_HELPER);
    } else if ((false == ArgumentMap[ON_520_UPDATE].empty()) || (false == ArgumentMap[ON_720_UPDATE].empty())) {
        return UpdateFwToFlash(PortIdList, ArgumentMap, KL520_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_630_USB_BOOT].empty()) {
        return UpdateKl630ToUsbLoader(PortIdList, ArgumentMap, KL630_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_630_FLASH_BOOT].empty()) {
        return UpdateKl630ToFlashBoot(PortIdList, ArgumentMap, KL630_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_630_UPDATE_LOADER].empty()) {
        return UpdateFwToFlash(PortIdList, ArgumentMap, KL630_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_730_USB_BOOT].empty()) {
        return UpdateKl730ToUsbLoader(PortIdList, ArgumentMap, KL730_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_730_FLASH_BOOT].empty()) {
        return UpdateKl730ToFlashBoot(PortIdList, ArgumentMap, KL730_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_730_UPDATE_LOADER].empty()) {
        return UpdateFwToFlash(PortIdList, ArgumentMap, KL730_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_830_USB_BOOT].empty()) {
        return UpdateKl830ToUsbLoader(PortIdList, ArgumentMap, KL830_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_830_FLASH_BOOT].empty()) {
        return UpdateKl830ToFlashBoot(PortIdList, ArgumentMap, KL830_UPDATE_PLUS_HELPER);
    } else if (false == ArgumentMap[ON_830_UPDATE_LOADER].empty()) {
        return UpdateFwToFlash(PortIdList, ArgumentMap, KL830_UPDATE_PLUS_HELPER);
    } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL520_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
        return UpdateModelToFlash(PortIdList, ArgumentMap, KL520_UPDATE_PLUS_HELPER);
    } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL630_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
        return UpdateModelToFlash(PortIdList, ArgumentMap, KL630_UPDATE_PLUS_HELPER);
    } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL720_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
        return UpdateModelToFlash(PortIdList, ArgumentMap, KL520_UPDATE_PLUS_HELPER); // KL720 does not need helper
    } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL730_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
        return UpdateModelToFlash(PortIdList, ArgumentMap, KL730_UPDATE_PLUS_HELPER);
    } else if ((false == ArgumentMap[ON_FLASH_MODEL].empty()) && (KL830_PRODUCT_NAME == ArgumentMap[ON_TYPE])) {
        return UpdateModelToFlash(PortIdList, ArgumentMap, KL830_UPDATE_PLUS_HELPER);
    }

    return 0;
}
