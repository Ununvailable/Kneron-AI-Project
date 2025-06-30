
#ifndef _KDRV_CAMERA_H_
#define _KDRV_CAMERA_H_
#include "base.h"
#ifdef RMI_COMM_IF
#include "kneronrmi.h"
#endif
typedef struct cam_format {
    uint32_t width;
    uint32_t height;
    uint32_t pixelformat;    /* fourcc */
    uint32_t field;          /* enum v2k_field */
    uint32_t bytesperline;   /* for padding, zero if unused */
    uint32_t sizeimage;
    uint32_t colorspace;     /* enum v2k_colorspace */
    uint32_t mirror;
    uint32_t flip;
}cam_format;

typedef struct
{
    //f05_ctrl0
    uint8_t vstu_pip0       : 1;
    uint8_t vstu_pip1       : 1;
    uint8_t vstu_pip2       : 1;
    uint8_t rsvd0           : 5;
    //f05_ctrl1
    uint8_t vstr[2*3];//RMI_IMGSRC_NUM];
    //f05_ctrl2
    uint8_t pftr[3];//RMI_IMGSRC_NUM];
    //f05_ctrl3
    uint8_t d2a_fifo_thr[2*3];//RMI_IMGSRC_NUM];
    //f05_ctrl4
    uint8_t d2a_fdrop_num[3];//RMI_IMGSRC_NUM];
    //f05_ctrl5
    uint8_t pyh_settle_cnt[3];//RMI_IMGSRC_NUM];
    //f05_ctrl6
    uint8_t led_strength[2*3];//RMI_IMGSRC_NUM];
    //f05_ctrl7
    uint8_t flip_0          : 1;
    uint8_t flip_1          : 1;
    uint8_t flip_2          : 1;
    uint8_t mirro_0         : 1;
    uint8_t mirro_1         : 1;
    uint8_t mirro_2         : 1;
    uint8_t rsvd1           : 2;
    //f05_ctrl8
    uint8_t tp_en_0         : 1;
    uint8_t tp_en_1         : 1;
    uint8_t tp_en_2         : 1;
    uint8_t rsvd2           : 5;
}s_cam_rmi_field;

#endif
