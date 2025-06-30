/**
 * @file        kmdw_stereo_depth.h
 * @brief       structure for the tof related functions
 *
 * @copyright   Copyright (c) 2019-2022 Kneron Inc. All rights reserved.
 */

#ifndef __KMDW_STEREO_DEPTH_H__
#define __KMDW_STEREO_DEPTH_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "base.h"
#include "kmdw_ipc.h"
#include "stereo_depth_init.h"

struct model_stereo_depth_ctx_s {
    scpu_to_ncpu_t*         p_ipc_out;
    ncpu_to_scpu_result_t*  p_ipc_in;
    osEventFlagsId_t        stereo_depth_evt_id;
    uint32_t                stereo_depth_evt_flag;
    stereo_depth_result_t   npu_rslt;
};

void kmdw_stereo_depth_calculate_fusion_results_config(void);

int calculate_fusion_results(int16_t *stereo_depth_in_buf_addr, uint8_t* stereo_depth_in_len, float rescale_factor, struct fusion_result *fus_res, int x_offset_seg_depth);

stereo_depth_result_t* model_stereo_depth_get_rslt(void);
#ifdef __cplusplus
}
#endif
#endif
