#ifndef __STEREO_DEPTH_INTERFACE_H__
#define __STEREO_DEPTH_INTERFACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "kp_struct.h"

#define MAX_INT_FOR_ALIGN 0x10000000

#define HIST_GRID 16
    typedef struct
    {
        uint8_t * buf_segmentation_addr;
        int16_t * buf_depth_data_addr;
        int32_t buf_filled_len;
    } stereo_depth_buf_t;

    struct fusion_result {
        int class_name;
        float min;
        float avg;
        uint32_t hist[HIST_GRID];
    };


    typedef struct
    {
        /*  flag: pamameter from scpu
            Bit0: only do initialization, calculate parameters
        */
        uint8_t flag;

        /* src img buf */
        stereo_depth_buf_t src_buf;

        float factor;
        
        int32_t x_offset;
        
        int8_t * img_buf;

        /* */
        uint32_t width;
        uint32_t height;

        struct fusion_result *result;

        /* frame time stamp, used for tracking/debug */
        int32_t frame_seq;

    } stereo_depth_params_t;

    typedef enum
    {
        STEREO_DEPTH_OPERATION_FAILED = -1,   /* Failure in doing operation */
        STEREO_DEPTH_OPERATION_SUCCESS = 1,   /* Operation Succeded */
        STEREO_DEPTH_OPERATION_INVALID_PARM,  /* Inavlid parameter provided */
        STEREO_DEPTH_OPERATION_NOT_SUPPORTED, /* Parameter/operation not supported */
        STEREO_DEPTH_OPER_ALIGN_32 = MAX_INT_FOR_ALIGN,
    } stereo_depth_oper_sts_t;

    typedef struct
    {

        int32_t rslt_type; //NCPU_TO_SCPU_RESULT_TYPE

        stereo_depth_oper_sts_t sts __attribute__((aligned(4)));

        /* frame time stamp, used for tracking/debug */
        int32_t frame_seq;

    } stereo_depth_result_t;

#ifdef __cplusplus
}
#endif
#endif
