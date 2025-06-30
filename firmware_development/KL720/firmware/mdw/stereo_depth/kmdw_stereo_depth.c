/*
 * Kneron Stereo Depth API 
 *
 * Copyright (C) 2022 Kneron, Inc. All rights reserved.
 *
 */

//#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "kmdw_stereo_depth.h"
#include "kmdw_memxfer.h"
#include "kmdw_console.h" /*for dbg_msg */
#include "kmdw_memory.h"
#include "math.h"

struct model_stereo_depth_ctx_s stereo_depth_kmdw_ctx = {0};

/***************** stereo depth function **********/
static stereo_depth_result_t *dec_result;

kdrv_status_t load_depth_table(void)
{
    kdp_memxfer_module.flash_to_ddr((uint32_t)DDR_MEM_RCALI_TABLE, FLASH_RCALI_TABLE_ADDR, DDR_MEM_RCALI_TABLE_SIZE);
    kdp_memxfer_module.flash_to_ddr((uint32_t)DDR_MEM_MASK_TABLE, FLASH_MASK_TABLE_ADDR, DDR_MEM_MASK_TABLE_SIZE);
    kdp_memxfer_module.flash_to_ddr((uint32_t)DDR_MEM_RMI_CONFIG_TABLE, FLASH_RMI_CONFIG_ADDR, DDR_MEM_RMI_CONFIG_TABLE_SIZE);

    return KDRV_STATUS_OK;
}

void kmdw_stereo_depth_calculate_fusion_results_config(){
    stereo_depth_params_t *pDec;
    scpu_to_ncpu_t *p_out_ipc;

    dec_result = (void *)kmdw_ddr_reserve(sizeof(stereo_depth_result_t));
    if (dec_result == 0)
        kmdw_printf("error !! stereo_depth_result_t DDR malloc failed\n");

    stereo_depth_kmdw_ctx.stereo_depth_evt_id = osEventFlagsNew(0);
    stereo_depth_kmdw_ctx.stereo_depth_evt_flag = 0x100;
    stereo_depth_kmdw_ctx.p_ipc_out = kmdw_ipc_get_output();
    stereo_depth_kmdw_ctx.p_ipc_in = kmdw_ipc_get_input();

    p_out_ipc = (scpu_to_ncpu_t *)stereo_depth_kmdw_ctx.p_ipc_out;

    pDec = (stereo_depth_params_t *)p_out_ipc->pExtInParam;

    memset((void *)pDec, 0, sizeof (stereo_depth_params_t));

    p_out_ipc->nLenExtInParam = sizeof (stereo_depth_params_t);
    p_out_ipc->pExtOutRslt = (void *)dec_result;
    p_out_ipc->nLenExtOutRslt = sizeof (stereo_depth_result_t);
    p_out_ipc->cmd = CMD_STEREO_DEPTH_FUSION;
}

int calculate_fusion_results(int16_t * stereo_depth_in_buf_addr, uint8_t* pidnet_segmentation_in_buf_addr, float rescale_factor, struct fusion_result* result, int x_offset_seg_depth){
    scpu_to_ncpu_t *p_out_ipc = (scpu_to_ncpu_t *)stereo_depth_kmdw_ctx.p_ipc_out;
    stereo_depth_params_t *pDec = (stereo_depth_params_t *)p_out_ipc->pExtInParam;

    pDec->src_buf.buf_segmentation_addr = pidnet_segmentation_in_buf_addr;
    pDec->src_buf.buf_depth_data_addr = stereo_depth_in_buf_addr;
    pDec->x_offset = x_offset_seg_depth;
    pDec->factor = rescale_factor;
    pDec->result = (struct fusion_result *)result;

    //trigger ncpu/npu
    kmdw_ipc_trigger_int(CMD_STEREO_DEPTH_FUSION);

    uint32_t flags = osEventFlagsWait(stereo_depth_kmdw_ctx.stereo_depth_evt_id, stereo_depth_kmdw_ctx.stereo_depth_evt_flag, osFlagsWaitAny, 300000);
    if (flags == (uint32_t)osFlagsErrorTimeout)
    {
        kmdw_printf("error ! dsp stereo depth process timeout!!!!!!!!!!!!!!!\n");
        return -7; // FIXME: for now it is timeout
    }
    else
    {
        if(dec_result->sts == STEREO_DEPTH_OPERATION_SUCCESS)
            return 0;
        else{
            kmdw_printf("error ! DSP stereo depth process error code = %d\n", dec_result->sts);
            return dec_result->sts;
        }
    }
}

stereo_depth_result_t* model_stereo_depth_get_rslt(void){
    return (stereo_depth_result_t*)&stereo_depth_kmdw_ctx.npu_rslt;
}
