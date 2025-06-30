/*
 * Kneron ToF API 
 *
 * Copyright (C) 2022 Kneron, Inc. All rights reserved.
 *
 */

//#include <stdlib.h>
#include <string.h>
#include "project.h"      /*for DDR_MEM_TOF_GAMMA_TABLE*/
#include "kmdw_tof.h"
#include "kmdw_memxfer.h"
#include "kmdw_console.h" /*for dbg_msg */
#include "kmdw_memory.h"
#include "math.h"

#if (APP_LOCK_PROJECT!=1)
#define GAMMA 1.0
#endif
/***************** TOF dec functions ***********/
tof_dec_kmdw_ctx_t tof_dec_kmdw_ctx = {0};

ir_bright_kmdw_ctx_t ir_bright_kmdw_ctx = {0};

/***************** TOF dec functions ***********/
void calculate_gamma_table(int16_t *gamma_table){
    int i;
    float f;
     #ifdef GAMMA
        float fProcompensation = 1/GAMMA;
	 #endif
    for(i = 0; i< 256; i++){
        f = (i + 0.5F) / 256;
        #ifdef GAMMA
        f = pow(f, fProcompensation);
        #endif
        f = f*256 - 0.5F;
        gamma_table[i] = (int16_t) f;
    }
}
static tof_decode_result_t *dec_result;
static ir_bright_result_t *ir_bright_result;

uint32_t tof_temp_depth_buf_addr,tof_depth_out_buf_addr, tof_ir_out_buf_addr, tof_table_gen_addr, tof_lut_addr;
uint32_t tof_table_gen_low_freq_addr, tof_filter_buf_addr;
uint32_t tof_ir_bright_buf_addr;

void kmdw_tof_set_decode_mode(uint8_t mode)
{
    scpu_to_ncpu_t *p_out_ipc = (scpu_to_ncpu_t *)tof_dec_kmdw_ctx.p_ipc_out;
    tof_decode_params_t *pDec = (tof_decode_params_t *)p_out_ipc->pExtInParam;
    pDec->flag = mode | 0x01;
}

void kmdw_tof_config_init()
{
    tof_decode_params_t *pDec;
    scpu_to_ncpu_t *p_out_ipc;

    dec_result = (void *)kmdw_ddr_reserve(sizeof(tof_decode_result_t));
    if (dec_result == 0)
        kmdw_printf("error !! dec_result DDR malloc failed\n");

    tof_temp_depth_buf_addr = kmdw_ddr_reserve(TOF_TEMP_DEPTH_SIZE);
    tof_depth_out_buf_addr = kmdw_ddr_reserve(TOF_DEPTH_IMG_SIZE);
    tof_ir_out_buf_addr = kmdw_ddr_reserve(TOF_IR_IMG_SIZE);
    tof_table_gen_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);
    tof_lut_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);

    int16_t *gamma_table = (int16_t *)(DDR_MEM_TOF_GAMMA_TABLE);
    // load sensor calibration data
    // kdp_memxfer_module.flash_to_ddr(DDR_MEM_TOF_RGB_CAM_PARAMS, FLASH_TOF_BUF_ADDR, DDR_MEM_TOF_RGB_CAM_PARAMS_SIZE);
    // kmdw_printf("DDR_MEM_TOF_RGB_CAM_PARAMS: %p\n", DDR_MEM_TOF_RGB_CAM_PARAMS);
    // kmdw_printf("DDR_MEM_TOF_RGB_DEPTH: %p\n", DDR_MEM_TOF_RGB_DEPTH);
    // kdp_memxfer_module.flash_to_ddr(DDR_MEM_TOF_GOLDEN_DATA, FLASH_TOF_BUF_ADDR + DDR_MEM_TOF_RGB_CAM_PARAMS_SIZE, DDR_MEM_TOF_GOLDEN_DATA_SIZE);
    // kmdw_printf("DDR_MEM_TOF_GOLDEN_DATA: %p\n", DDR_MEM_TOF_GOLDEN_DATA);
    // unsigned char *cali_table = (unsigned char*)(DDR_MEM_TOF_CALI_TABLE);

    // memcpy(cali_table, &cal_data, DDR_MEM_TOF_CALI_TABLE_SIZE);

    calculate_gamma_table(gamma_table);

    tof_dec_kmdw_ctx.tof_dec_evt_id = osEventFlagsNew(0);
    tof_dec_kmdw_ctx.tof_dec_evt_flag = 0x100;
    tof_dec_kmdw_ctx.p_ipc_out = kmdw_ipc_get_output();
    tof_dec_kmdw_ctx.p_ipc_in = kmdw_ipc_get_input();

    p_out_ipc = (scpu_to_ncpu_t *)tof_dec_kmdw_ctx.p_ipc_out;

    pDec = (tof_decode_params_t *)p_out_ipc->pExtInParam;
    p_out_ipc->nLenExtInParam = sizeof (tof_decode_params_t);

    p_out_ipc->pExtOutRslt = (void *)dec_result;
    p_out_ipc->nLenExtOutRslt = sizeof (tof_decode_result_t);

    memset((void *)pDec, 0, sizeof (tof_decode_params_t));

    pDec->p1_table = (int16_t *)(DDR_MEM_TOF_P1_TABLE);
    pDec->p2_table = (int16_t *)(DDR_MEM_TOF_P2_TABLE);
    pDec->shift_table = (int16_t *)(DDR_MEM_TOF_SHIFT_TABLE);
    pDec->table_gen = (int16_t *)(tof_table_gen_addr);
    pDec->gradient_lut = (float *)(tof_lut_addr);
    pDec->gamma_table = (int16_t *)(DDR_MEM_TOF_GAMMA_TABLE);
    pDec->non_linear_H = (int16_t *)(DDR_MEM_TOF_NON_LINEAR);
    pDec->CalibParameter = (unsigned char*)(DDR_MEM_TOF_CALI_TABLE);
    pDec->temperature_coeff = (float *)(DDR_MEM_TOF_TEMPERATURE_COEFF);
    pDec->ir_params = (ir_params *)(DDR_MEM_TOF_IR_CAM_PARAMS);

    pDec->temp_depth_buf = tof_temp_depth_buf_addr;
    pDec->temp_depth_buf_size = TOF_TEMP_DEPTH_SIZE;

    pDec->flag = 0x01;

    pDec->width = 640;
    pDec->height = 240;

    p_out_ipc->cmd = CMD_TOF_DECODE;
}

int kmdw_tof_decode(uint32_t tof_in_buf_addr, uint32_t tof_in_len, uint32_t out_depth_buf_addr, uint32_t out_ir_buf_addr)
{

    scpu_to_ncpu_t *p_out_ipc = (scpu_to_ncpu_t *)tof_dec_kmdw_ctx.p_ipc_out;
    tof_decode_params_t *pDec = (tof_decode_params_t *)p_out_ipc->pExtInParam;

    pDec->src_buf.buf_addr = tof_in_buf_addr;
    pDec->src_buf.buf_filled_len = tof_in_len;

    pDec->dst_depth_buf = out_depth_buf_addr;
    pDec->dst_depth_buf_size = 307200*2;  // estimated max size, optional

    pDec->dst_ir_buf = out_ir_buf_addr;
    pDec->dst_ir_buf_size = 307200;  // estimated max size, optional

    //trigger ncpu/npu
    kmdw_ipc_trigger_int(CMD_TOF_DECODE);

    uint32_t flags = osEventFlagsWait(tof_dec_kmdw_ctx.tof_dec_evt_id, tof_dec_kmdw_ctx.tof_dec_evt_flag, osFlagsWaitAny, 30000);
    if (flags == (uint32_t)osFlagsErrorTimeout)
    {
        kmdw_printf("error ! TOF decode timeout!!!!!!!!!!!!!!!\n");
        return -7; // FIXME: for now it is timeout
    }
    else
    {
        if(dec_result->sts == TOF_DECODE_OPERATION_SUCCESS)
            return 0;
        else{
            kmdw_printf("error ! TOF decode error code = %d\n, dec_result->sts");
            return dec_result->sts;
        }
    }
}

tof_decode_result_t* model_tof_dec_get_rslt(void)
{
    return (tof_decode_result_t*)&tof_dec_kmdw_ctx.npu_rslt;
}

void kmdw_tof_dual_config_init()
{
#if TOF_DUAL_FREQUENCY
    kmdw_printf("kmdw_tof_dual_config_init begin\n");
    tof_decode_params_t *pDec;
    scpu_to_ncpu_t *p_out_ipc;

    dec_result = (void *)kmdw_ddr_reserve(sizeof(tof_decode_result_t));
    if (dec_result == 0)
        kmdw_printf("error !! dec_result DDR malloc failed\n");

    // tof_working_buf_addr = kmdw_ddr_reserve(TOF_DUAL_WORKING_BUF_SIZE);
    // if (tof_working_buf_addr == 0)
    //     kmdw_printf("error !! tof_working_buf DDR malloc failed\n");

    tof_temp_depth_buf_addr = kmdw_ddr_reserve(TOF_TEMP_DEPTH_SIZE);
    tof_depth_out_buf_addr = kmdw_ddr_reserve(TOF_DEPTH_IMG_SIZE);
    tof_ir_out_buf_addr = kmdw_ddr_reserve(TOF_IR_IMG_SIZE);
    tof_table_gen_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);
    tof_table_gen_low_freq_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);
    tof_lut_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);
    tof_filter_buf_addr = kmdw_ddr_reserve(TOF_INIT_TABLE_SIZE);


    // int16_t *p1_table = (int16_t *)(tof_working_buf_addr + 1228800);
    // int16_t *p2_table = (int16_t *)(tof_working_buf_addr + 1536000);
    // int16_t *non_H = (int16_t *)(tof_working_buf_addr + 1843200);
    // unsigned char *cali_table = (unsigned char *)(tof_working_buf_addr + 1843200 + 4096 * 2);
    // int16_t *gamma_table = (int16_t *)(tof_working_buf_addr + 1843200 + 4096 * 2 + 45*4);
    int16_t *gamma_table = (int16_t *)(DDR_MEM_TOF_GAMMA_TABLE);
    // load sensor calibration data
    kdp_memxfer_module.flash_to_ddr(DDR_MEM_TOF_RGB_CAM_PARAMS, FLASH_TOF_BUF_ADDR, DDR_MEM_TOF_RGB_CAM_PARAMS_SIZE);
    kmdw_printf("DDR_MEM_TOF_RGB_CAM_PARAMS: %p\n", DDR_MEM_TOF_RGB_CAM_PARAMS);
    kmdw_printf("DDR_MEM_TOF_RGB_DEPTH: %p\n", DDR_MEM_TOF_RGB_DEPTH);
#if 0 // read param from sensor
    unsigned char *cali_table = (unsigned char*)(DDR_MEM_TOF_CALI_TABLE);
    unsigned char real_cali[DDR_MEM_TOF_CALI_TABLE_SIZE];

    kdev_sensor_get_calibration_data(cali_table, DDR_MEM_TOF_CALI_TABLE_SIZE/4);//&real_cali[0]
    memcpy(cali_table, real_cali, DDR_MEM_TOF_CALI_TABLE_SIZE);
    // bool need_fix_cali = false;
    // for (size_t i = 0; i < DDR_MEM_TOF_CALI_TABLE_SIZE; i++)
    // {
    //     if (real_cali[i] != cali_table[i])
    //     {
    //         memcpy(cali_table, real_cali, DDR_MEM_TOF_CALI_TABLE_SIZE);
    //         need_fix_cali = true;
    //         break;
    //     }
    // }
#endif

    // kmdw_printf("kdp2_app_tof_init_once non_H[1000]: %d, non_H[1001]: %d,  non_H[1002]: %d    \n",*(non_H+1000), *(non_H+1001),*(non_H+1002));
    // kmdw_printf("non_H: %d, p1(0,0) = %d   p2(0,0) =%d    \n",*(non_H+1000), *(p1_table),*(p2_table) );
    // kmdw_printf("p1(320,120) = %d   p2(320,120) =%d    \n",*(p1_table+640*120+320),*(p2_table+640*120+320));
    calculate_gamma_table(gamma_table);

    tof_dec_kmdw_ctx.tof_dec_evt_id = osEventFlagsNew(0);
    tof_dec_kmdw_ctx.tof_dec_evt_flag = 0x100;
    tof_dec_kmdw_ctx.p_ipc_out = kmdw_ipc_get_output();
    tof_dec_kmdw_ctx.p_ipc_in = kmdw_ipc_get_input();

    p_out_ipc = (scpu_to_ncpu_t *)tof_dec_kmdw_ctx.p_ipc_out;

    pDec = (tof_decode_params_t *)p_out_ipc->pExtInParam;
    p_out_ipc->nLenExtInParam = sizeof (tof_decode_params_t);

    p_out_ipc->pExtOutRslt = (void *)dec_result;
    p_out_ipc->nLenExtOutRslt = sizeof (tof_decode_result_t);

    memset((void *)pDec, 0, sizeof (tof_decode_params_t));

    pDec->p1_table = (int16_t *)(DDR_MEM_TOF_P1_TABLE);
    pDec->p2_table = (int16_t *)(DDR_MEM_TOF_P2_TABLE);
    pDec->shift_table = (int16_t *)(DDR_MEM_TOF_SHIFT_TABLE);
    pDec->undistort_buf = (int16_t *)(DDR_MEM_TOF_UNDISTORT_BUF);
    pDec->table_gen = (int16_t *)(tof_table_gen_addr);
    pDec->table_gen_low_freq = (int16_t *)(tof_table_gen_low_freq_addr);

    pDec->gradient_lut = (float *)(tof_lut_addr);
    pDec->filter_buf = (int16_t *)(tof_filter_buf_addr);
    pDec->gamma_table = (int16_t *)(DDR_MEM_TOF_GAMMA_TABLE);
    pDec->non_linear_H = (int16_t *)(DDR_MEM_TOF_NON_LINEAR);
    pDec->CalibParameter = (unsigned char*)(DDR_MEM_TOF_CALI_TABLE);
    pDec->temperature_coeff = (float *)(DDR_MEM_TOF_TEMPERATURE_COEFF);
    pDec->ir_params = (ir_params *)(DDR_MEM_TOF_IR_CAM_PARAMS);

    pDec->temp_depth_buf = tof_temp_depth_buf_addr;
    pDec->temp_depth_buf_size = TOF_TEMP_DEPTH_SIZE;

    pDec->flag = 0x01;
    // pDec->flag |= (only_init == true)? BIT0:0;

    pDec->width = 640;
    pDec->height = 240;

    // pDec->temp_depth_buf = tof_working_buf_addr;
    // pDec->temp_depth_buf_size = 614400;

    p_out_ipc->cmd = CMD_TOF_DECODE;
    kmdw_printf("kmdw_tof_dual_config_init finished\n");
#endif
}

int kmdw_tof_decode_dual(uint32_t tof_in_buf_addr, uint32_t tof_in_len, uint32_t out_depth_buf_addr, uint32_t out_ir_buf_addr)
{
#if TOF_DUAL_FREQUENCY
    scpu_to_ncpu_t *p_out_ipc = (scpu_to_ncpu_t *)tof_dec_kmdw_ctx.p_ipc_out;
    tof_decode_params_t *pDec = (tof_decode_params_t *)p_out_ipc->pExtInParam;

    pDec->src_buf.buf_addr = tof_in_buf_addr;
    pDec->src_buf.buf_filled_len = tof_in_len;

    pDec->dst_depth_buf = out_depth_buf_addr;
    pDec->dst_depth_buf_size = 307200;  // estimated max size, optional

    pDec->dst_ir_buf = out_ir_buf_addr;
    pDec->dst_ir_buf_size = 307200;  // estimated max size, optional

    //trigger ncpu/npu
    kmdw_ipc_trigger_int(CMD_TOF_DECODE_DUAL);

    uint32_t flags = osEventFlagsWait(tof_dec_kmdw_ctx.tof_dec_evt_id, tof_dec_kmdw_ctx.tof_dec_evt_flag, osFlagsWaitAny, 30000);

    if (flags == (uint32_t)osFlagsErrorTimeout)
    {
        kmdw_printf("error ! TOF decode timeout!!!!!!!!!!!!!!!\n");
        return -7; // FIXME: for now it is timeout
    }
    else
    {
        if(dec_result->sts == TOF_DECODE_OPERATION_SUCCESS)
            return 0;
        else{
            kmdw_printf("error ! TOF decode error code = %d\n, dec_result->sts");
            return dec_result->sts;
        }
    }
#else
    return 0;
#endif
}

void kmdw_tof_ir_bright_init(uint32_t src_addr, uint32_t src_w, uint32_t src_h)
{
    ir_bright_params_t *pDec;
    scpu_to_ncpu_t *p_out_ipc;

    ir_bright_result = (void *)kmdw_ddr_reserve(sizeof(ir_bright_result_t));
    if (ir_bright_result == 0)
        kmdw_printf("error !! ir_bright_result DDR malloc failed\n");
    
    // kmdw_printf("cala ir bright config\n");
    tof_ir_bright_buf_addr = kmdw_ddr_reserve(sizeof(bounding_box_t));

    ir_bright_kmdw_ctx.ir_bright_evt_id = osEventFlagsNew(0);
    ir_bright_kmdw_ctx.ir_bright_evt_flag = BIT8;//vt_flag;
    ir_bright_kmdw_ctx.p_ipc_out = kmdw_ipc_get_output();
    ir_bright_kmdw_ctx.p_ipc_in = kmdw_ipc_get_input();

    p_out_ipc = (scpu_to_ncpu_t *)ir_bright_kmdw_ctx.p_ipc_out;

    pDec = (ir_bright_params_t *)p_out_ipc->pExtInParam;
    p_out_ipc->nLenExtInParam = sizeof (ir_bright_params_t);

    p_out_ipc->pExtOutRslt = (void *)ir_bright_result;
    p_out_ipc->nLenExtOutRslt = sizeof (ir_bright_result_t);

    memset((void *)pDec, 0, sizeof (ir_bright_params_t));

    // change here to yolo result?
    bounding_box_t *p_aec_bbox = (bounding_box_t *)tof_ir_bright_buf_addr;
    p_aec_bbox->x1 = 0;
    p_aec_bbox->y1 = 0;
    p_aec_bbox->x2 = 64;//src_w-1;
    p_aec_bbox->y2 = 64;//src_h-1;
    
    pDec->ir_aec_bbox = (bounding_box_t *)p_aec_bbox;

    pDec->src_buf.buf = src_addr;
    pDec->src_buf.width = src_w;
    pDec->src_buf.height = src_h;

    p_out_ipc->cmd = CMD_TOF_CALC_IR_BRIGHT;
}

int32_t kmdw_tof_ir_bright_aec(uint32_t nir_output_buf, int32_t src_width, uint32_t src_height, bounding_box_t *single_tof_box, bool is_target_found)
{
    //trigger ncpu/npu
    // dbg_msg("[%.3f][%s] kmdw_tof_ir_bright_aec\n", (float)osKernelGetTickCount()/osKernelGetTickFreq(), __func__);
    /*  update nir output_buf, src_width, src_height */
    scpu_to_ncpu_t *p_out_ipc = (scpu_to_ncpu_t *)ir_bright_kmdw_ctx.p_ipc_out;
    ir_bright_params_t *pDec = (ir_bright_params_t *)p_out_ipc->pExtInParam;

    // critical_msg("ir_bright_kmdw_ctx.ir_bright_evt_id : %d\n",ir_bright_kmdw_ctx.ir_bright_evt_id);
    if (NULL == ir_bright_kmdw_ctx.ir_bright_evt_id)
    {
        ir_bright_kmdw_ctx.ir_bright_evt_id = osEventFlagsNew(0);
        ir_bright_kmdw_ctx.ir_bright_evt_flag = BIT8;
    }

    bounding_box_t *p_aec_bbox = (bounding_box_t *)tof_ir_bright_buf_addr;

    pDec->src_buf.buf = nir_output_buf;
    pDec->src_buf.width = src_width;
    pDec->src_buf.height = src_height;
    pDec->ir_aec_bbox = (bounding_box_t *)p_aec_bbox;
    pDec->is_target_found = is_target_found;

    /* --------------------------------------------- */
    // critical_msg("kmdw_tof_ir_bright_aec 1\n");
    kmdw_ipc_trigger_int(CMD_TOF_CALC_IR_BRIGHT);
    // critical_msg("kmdw_tof_ir_bright_aec 2\n");
    uint32_t flags = osEventFlagsWait(ir_bright_kmdw_ctx.ir_bright_evt_id, ir_bright_kmdw_ctx.ir_bright_evt_flag, osFlagsWaitAny, 5000);
    if (flags == (uint32_t)osFlagsErrorTimeout)
    {
        kmdw_printf("error ! kmdw_tof_ir_bright_aec timeout!!!!!!!!!!!!!!!\n");
        return -7; // FIXME: for now it is timeout
    }
    else
    {
        if(ir_bright_result->sts == TOF_IR_AEC_OPERATION_SUCCESS)
            return 0;
        else{
            kmdw_printf("error ! TOF ir aec error code = %d\n", ir_bright_result->sts);
            return ir_bright_result->sts;
        }
    }
}

ir_bright_result_t* model_ir_bright_get_rslt(void)
{
    return (ir_bright_result_t*)&ir_bright_kmdw_ctx.npu_rslt;
}
