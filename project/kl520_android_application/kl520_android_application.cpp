/**
 * @file        listing_device.c
 * @brief       example of scanning Kneron KL series device
 * @version     0.1
 * @date        2025-07-01
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
#include "kp_core.h"
#include "kp_inference.h"
#include "helper_functions.h"
#include "postprocess.h"
}

// #include <opencv2/opencv.hpp>
#include <mutex>

int main() {
    printf("My custom inference program\n");
    // Insert SDK initialization / connection / inference logic
    return 0;
}