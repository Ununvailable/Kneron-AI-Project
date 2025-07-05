/**
 * @file        kp_inference.c
 * @brief       inference functions
 * @version     2.0
 * @date        2022-05-23
 *
 * @copyright   Copyright (c) 2021 Kneron Inc. All rights reserved.
 */

// #define DEBUG_PRINT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <float.h>

#include "kp_inference.h"
#include "kp_usb.h"
#include "kp_core.h"

#include "kdp2_ipc_cmd.h"
#include "kdp2_inf_generic_raw.h"
#include "kdp2_inf_dbg.h"
#include "kp_internal.h"
#include "internal_func.h"
#include "model_type.h"

#ifdef DEBUG_PRINT
#define dbg_print(format, ...)  { printf(format, ##__VA_ARGS__); fflush(stdout); }
#else
#define dbg_print(format, ...)
#endif

#define err_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }

static int check_inf_desc_error(int ll_return)
{
    if (ll_return == KP_USB_USB_TIMEOUT)
        return KP_ERROR_USB_TIMEOUT_N7;

    if (ll_return != KP_USB_RET_OK)
        return KP_ERROR_SEND_DESC_FAIL_13;

    return KP_SUCCESS;
}

static int check_send_image_error(int ll_return)
{
    if (ll_return == KP_USB_USB_TIMEOUT)
        return KP_ERROR_USB_TIMEOUT_N7;

    if (ll_return != KP_USB_RET_OK)
        return KP_ERROR_SEND_DATA_FAIL_14;

    return KP_SUCCESS;
}

static int get_image_size(kp_image_format_t format, int width, int height, uint32_t *image_size)
{
    switch (format)
    {
    case KP_IMAGE_FORMAT_RGB565:
    case KP_IMAGE_FORMAT_YUYV:
    case KP_IMAGE_FORMAT_YCBCR422_CRY1CBY0:
    case KP_IMAGE_FORMAT_YCBCR422_CBY1CRY0:
    case KP_IMAGE_FORMAT_YCBCR422_Y1CRY0CB:
    case KP_IMAGE_FORMAT_YCBCR422_Y1CBY0CR:
    case KP_IMAGE_FORMAT_YCBCR422_CRY0CBY1:
    case KP_IMAGE_FORMAT_YCBCR422_CBY0CRY1:
    case KP_IMAGE_FORMAT_YCBCR422_Y0CRY1CB:
    case KP_IMAGE_FORMAT_YCBCR422_Y0CBY1CR:
        *image_size = width * height * 2;
        return KP_SUCCESS;
    case KP_IMAGE_FORMAT_RGBA8888:
        *image_size = width * height * 4;
        return KP_SUCCESS;
    case KP_IMAGE_FORMAT_RAW8:
        *image_size = width * height;
        return KP_SUCCESS;
    case KP_IMAGE_FORMAT_YUV420:
        *image_size = width * height * 1.5;
        return KP_SUCCESS;
    default:
    case KP_IMAGE_FORMAT_UNKNOWN:
        *image_size = 0;
        return KP_ERROR_INVALID_PARAM_12;
    }
}

static bool check_model_id_is_exist_in_nef(_kp_devices_group_t *_devices_grp, uint32_t model_id)
{
    bool ret = false;

    for (int m = 0; m < _devices_grp->loaded_model_desc.num_models; m++)
    {
        if (_devices_grp->loaded_model_desc.models[m].id == model_id)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

static bool check_model_input_node_number_is_correct(_kp_devices_group_t *_devices_grp, uint32_t model_id, uint32_t num_input_node_data)
{
    bool ret = false;

    for (int m = 0; m < _devices_grp->loaded_model_desc.num_models; m++)
    {
        if ((_devices_grp->loaded_model_desc.models[m].id == model_id) &&
            (_devices_grp->loaded_model_desc.models[m].input_nodes_num == num_input_node_data))
        {
            ret = true;
            break;
        }
    }

    return ret;
}

static int verify_result_header_stamp(kp_inference_header_stamp_t *stamp, uint32_t check_total_size, uint32_t check_job_id)
{
    if ((stamp->magic_type != KDP2_MAGIC_TYPE_INFERENCE) && (stamp->magic_type != KDP2_MAGIC_TYPE_INFERENCE_V2))
    {
        dbg_print("%s, magic_type = 0x%x \n ",__func__, stamp->magic_type);
        dbg_print("%s, total_size = 0x%x \n ",__func__, stamp->total_size);
        dbg_print("%s, job_id = 0x%x \n ",__func__, stamp->job_id);
        dbg_print("%s, status_code = 0x%x \n ",__func__, stamp->status_code);
        return KP_ERROR_RECEIVE_INCORRECT_HEADER_STAMP_30;
    }

    if (stamp->status_code != KP_SUCCESS)
        return stamp->status_code; // FW report error

    if (check_job_id > 0 && stamp->job_id != check_job_id)
        return KP_ERROR_RECEIVE_JOB_ID_MISMATCH_32;

    if (check_total_size > 0 && stamp->total_size != check_total_size)
        return KP_ERROR_RECEIVE_SIZE_MISMATCH_31;

    return KP_SUCCESS;
}

static kp_channel_ordering_convert_t get_channel_ordering_convert_code(int product_id, kp_channel_ordering_t ordering)
{
    switch (product_id)
    {
    case KP_DEVICE_KL520:
        switch (ordering)
        {
        case KP_CHANNEL_ORDERING_CHW:
            return KP_CHANNEL_ORDERING_CVT_HCW2CHW;
        case KP_CHANNEL_ORDERING_HWC:
            return KP_CHANNEL_ORDERING_CVT_HCW2HWC;
        case KP_CHANNEL_ORDERING_DEFAULT:
        default:
            return KP_CHANNEL_ORDERING_CVT_NONE;
        }
        break;
    case KP_DEVICE_KL720:
    case KP_DEVICE_KL630:
        switch (ordering)
        {
        case KP_CHANNEL_ORDERING_HCW:
            return KP_CHANNEL_ORDERING_CVT_CHW2HCW;
        case KP_CHANNEL_ORDERING_HWC:
            return KP_CHANNEL_ORDERING_CVT_CHW2HWC;
        case KP_CHANNEL_ORDERING_DEFAULT:
        default:
            return KP_CHANNEL_ORDERING_CVT_NONE;
        }
        break;
    case KP_DEVICE_KL830:
    case KP_DEVICE_KL730:
        switch (ordering)
        {
        case KP_CHANNEL_ORDERING_HCW:
            return KP_CHANNEL_ORDERING_CVT_CHW2HCW;
        case KP_CHANNEL_ORDERING_HWC:
            return KP_CHANNEL_ORDERING_CVT_CHW2HWC;
        case KP_CHANNEL_ORDERING_DEFAULT:
        default:
            return KP_CHANNEL_ORDERING_CVT_NONE;
        }
        break;
    default:
        return KP_CHANNEL_ORDERING_CVT_NONE;
    }
}

static kp_fixed_point_dtype_t get_fixed_point_dtype(kp_model_tensor_data_layout_t data_layout)
{
    switch (data_layout)
    {
    case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B:
    case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B:
    case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B_CH_COMPACT:
    case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B:
    case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B:
        return KP_FIXED_POINT_DTYPE_INT8;
        break;
    case KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B:
    case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL:
    case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL:
    case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL_CH_COMPACT:
    case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL:
    case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B:
        return KP_FIXED_POINT_DTYPE_INT16;
        break;
    default:
        return KP_FIXED_POINT_DTYPE_UNKNOWN;
    }
}

static float pow2(int exp)
{
    if (0 <= exp) {
        return (float)(0x1ULL << exp);
    } else {
        return (float)1 / (float)(0x1ULL << abs(exp));
    }
}

static int get_quantization_parameters_v1_information(kp_quantization_parameters_v1_t* quantization_parameters_v1, int quantized_fixed_point_descriptor_idx, int32_t *radix, float *scale)
{
    if ((NULL == quantization_parameters_v1) ||
        (NULL == radix) ||
        (NULL == scale)) {
        printf("error: NULL pointer input parameter\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    if (quantized_fixed_point_descriptor_idx >= quantization_parameters_v1->quantized_fixed_point_descriptor_num) {
        printf("error: index of quantized_fixed_point_descriptor out of range\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    kp_quantized_fixed_point_descriptor_t *quantized_fixed_point_descriptor = &(quantization_parameters_v1->quantized_fixed_point_descriptor[quantized_fixed_point_descriptor_idx]);

    *radix = quantized_fixed_point_descriptor->radix;

    switch (quantized_fixed_point_descriptor->scale_dtype)
    {
    case KP_DTYPE_INT8:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_int8;
        break;
    case KP_DTYPE_INT16:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_int16;
        break;
    case KP_DTYPE_INT32:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_int32;
        break;
    case KP_DTYPE_UINT8:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_uint8;
        break;
    case KP_DTYPE_UINT16:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_uint16;
        break;
    case KP_DTYPE_UINT32:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_uint32;
        break;
    case KP_DTYPE_FLOAT32:
        *scale = (float)quantized_fixed_point_descriptor->scale.scale_float32;
        break;
    default:
        printf("error: get invalide KneronKNE_DataType_enum_t ...\n");
        return KP_ERROR_INVALID_MODEL_21;
    }

    return KP_SUCCESS;
}

inline static int get_quantization_parameters_factor(kp_quantization_parameters_v1_t* quantization_parameters_v1, bool is_channel_wise_quantization, int onnx_data_buf_offset, int quantized_axis_stride, int *quantized_fixed_point_descriptor_idx, float *quantization_factor)
{
    int status  = KP_SUCCESS;
    int radix   = 0;
    float scale = 0;

    if ((NULL == quantization_parameters_v1) ||
        (NULL == quantized_fixed_point_descriptor_idx) ||
        (NULL == quantization_factor)) {
        printf("error: NULL pointer input parameter\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    if (is_channel_wise_quantization) {
        if (0 == (onnx_data_buf_offset % quantized_axis_stride)) {
            status = get_quantization_parameters_v1_information(quantization_parameters_v1, *quantized_fixed_point_descriptor_idx, &radix, &scale);
            if (KP_SUCCESS != status) {
                printf("error: get invalide KneronKNE_DataType_enum_t ...\n");
                goto FUNC_OUT;
            }

            #ifdef OPTIMIZED_FIXED_TO_FLOAT
            {
                *quantization_factor = (float)1 / (float)(scale * pow2(radix));
            }
            #else
            {
                *quantization_factor = (float)(scale * pow2(radix));
            }
            #endif

            (*quantized_fixed_point_descriptor_idx)++;
        }
    } else {
        if (0 == onnx_data_buf_offset) {
            radix = 0;
            scale = 0;

            status = get_quantization_parameters_v1_information(quantization_parameters_v1, 0, &radix, &scale);
            if (KP_SUCCESS != status) {
                printf("error: get invalide KneronKNE_DataType_enum_t ...\n");
                goto FUNC_OUT;
            }

            #ifdef OPTIMIZED_FIXED_TO_FLOAT
            {
                *quantization_factor = (float)1 / (float)(scale * pow2(radix));
            }
            #else
            {
                *quantization_factor = (float)(scale * pow2(radix));
            }
            #endif
        }
    }

FUNC_OUT:
    return status;
}

int kp_inference_configure(kp_device_group_t devices, kp_inf_configuration_t *conf)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    int timeout = _devices_grp->timeout;
    kp_usb_control_t kctrl = {0};
    int ret = KP_SUCCESS;

    kctrl.command = KDP2_CONTROL_FIFOQ_ENABLE_DROPPABLE;

    kctrl.arg1 = (conf->enable_frame_drop) ? 1 : 0;

    for (int i = 0; i < _devices_grp->num_device; i++)
    {
        kp_usb_device_t *ll_dev = _devices_grp->ll_device[i];
        ret = kp_usb_control(ll_dev, &kctrl, timeout);

        if (KP_SUCCESS != ret) {
            break;
        }
    }

    return ret;
}

int kp_generic_image_inference_send(kp_device_group_t devices, kp_generic_image_inference_desc_t *inf_data)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_send++];

    if (_devices_grp->cur_send >= _devices_grp->num_device)
        _devices_grp->cur_send = 0;

    int timeout = _devices_grp->timeout;

    uint32_t image_size = 0;

    int ret = 0;
    int num_input_node_image = inf_data->num_input_node_image;

    if ((KP_DEVICE_KL730 == _devices_grp->product_id) ||
        (KP_DEVICE_KL830 == _devices_grp->product_id)) {
        if (KP_MAX_INPUT_NODE_COUNT_V2 < num_input_node_image) {
            return KP_ERROR_INVALID_INPUT_NODE_DATA_NUMBER_48;
        }
    } else {
        if (KP_MAX_INPUT_NODE_COUNT_V1 < num_input_node_image) {
            return KP_ERROR_INVALID_INPUT_NODE_DATA_NUMBER_48;
        }
    };

    if (false == check_model_input_node_number_is_correct(_devices_grp, inf_data->model_id, num_input_node_image)) {
        return KP_ERROR_INVALID_PARAM_12;
    } else if (_devices_grp->ddr_attr.input_buffer_count < num_input_node_image) {
        return KP_ERROR_FIFOQ_INPUT_BUFF_COUNT_NOT_ENOUGH_42;
    }

    if (false == check_model_id_is_exist_in_nef(_devices_grp, inf_data->model_id)) {
        dbg_print("[%s] model id [%d] not exist in nef\n", __func__, inf_data->model_id);
        return KP_ERROR_MODEL_NOT_LOADED_35;
    }

    for (int i = 0; i < num_input_node_image; i++) {
        ret = get_image_size(inf_data->input_node_image_list[i].image_format, inf_data->input_node_image_list[i].width, inf_data->input_node_image_list[i].height, &image_size);
        if (ret != KP_SUCCESS)
            return ret;

        kdp2_ipc_generic_raw_inf_header_t raw_inf_header;

        raw_inf_header.header_stamp.magic_type = KDP2_MAGIC_TYPE_INFERENCE;
        raw_inf_header.header_stamp.total_size = sizeof(raw_inf_header) + image_size;
        raw_inf_header.header_stamp.job_id = KDP2_INF_ID_GENERIC_RAW;
        raw_inf_header.header_stamp.total_image = num_input_node_image;
        raw_inf_header.header_stamp.image_index = i;

        if (raw_inf_header.header_stamp.total_size > _devices_grp->ddr_attr.input_buffer_size)
        {
            dbg_print("[%s] image buffer size is not enough in firmware\n", __func__);
            return KP_ERROR_SEND_DATA_TOO_LARGE_15;
        }

        raw_inf_header.inference_number = inf_data->inference_number;
        raw_inf_header.model_id = inf_data->model_id;

        memcpy((void *)&raw_inf_header.image_header, &inf_data->input_node_image_list[i], sizeof(kdp2_ipc_generic_raw_inf_image_header_t));

        ret = kp_usb_write_data(ll_dev, (void *)&raw_inf_header, sizeof(raw_inf_header), timeout);
        int status = check_inf_desc_error(ret);
        if (status != KP_SUCCESS)
            return status;

        ret = kp_usb_write_data(ll_dev, (void *)inf_data->input_node_image_list[i].image_buffer, image_size, timeout);
        status = check_send_image_error(ret);
        if (status != KP_SUCCESS)
            return status;
    }

    return KP_SUCCESS;
}

int kp_generic_image_inference_receive(kp_device_group_t devices, kp_generic_image_inference_result_header_t *output_desc, uint8_t *raw_out_buffer, uint32_t buf_size)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_recv];

    int timeout = _devices_grp->timeout;

    // if return < 0 means libusb error, otherwise return  received size
    int usb_ret = kp_usb_read_data(ll_dev, (void *)raw_out_buffer, buf_size, timeout);
    if (usb_ret < 0)
        return usb_ret;

    // parsing result buffer

    kp_inference_header_stamp_t *header_stamp = (kp_inference_header_stamp_t *)raw_out_buffer;
    int status = verify_result_header_stamp(header_stamp, 0, KDP2_INF_ID_GENERIC_RAW);

    if (status != KP_SUCCESS) {
        return status;
    }

    if (KDP2_MAGIC_TYPE_INFERENCE == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t *ipc_result = (kdp2_ipc_generic_raw_result_t *)raw_out_buffer;

        output_desc->inference_number = ipc_result->inf_number;
        output_desc->crop_number = ipc_result->crop_number;
        output_desc->product_id = ipc_result->product_id;

        switch (ipc_result->product_id)
        {
        case KP_DEVICE_KL520:
        {
            output_desc->num_output_node = *(uint32_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t));
            break;
        }
        case KP_DEVICE_KL720:
        {
            _720_raw_cnn_res_t *raw_cnn_res = (_720_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t));
            output_desc->num_output_node = raw_cnn_res->total_nodes;
            break;
        }
        case KP_DEVICE_KL830:
        case KP_DEVICE_KL730:
        case KP_DEVICE_KL630:
        {
            _630_raw_cnn_res_t *raw_cnn_res = (_630_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t));
            output_desc->num_output_node = raw_cnn_res->total_nodes;
            break;
        }
        default:
            break;
        }

        output_desc->num_pre_proc_info = ipc_result->num_of_pre_proc_info;

        memcpy(output_desc->pre_proc_info, ipc_result->pre_proc_info, output_desc->num_pre_proc_info * sizeof(kp_hw_pre_proc_info_t));

        if (ipc_result->is_last_crop == 1)
            _devices_grp->cur_recv++;
    } else if (KDP2_MAGIC_TYPE_INFERENCE_V2 == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t_v2 *ipc_result = (kdp2_ipc_generic_raw_result_t_v2 *)raw_out_buffer;

        if (KP_MAX_INPUT_NODE_COUNT < ipc_result->num_of_pre_proc_info) {
            return KP_ERROR_INVALID_INPUT_NODE_DATA_NUMBER_48;
        }

        output_desc->inference_number = ipc_result->inf_number;
        output_desc->crop_number = ipc_result->crop_number;
        output_desc->product_id = ipc_result->product_id;

        switch (ipc_result->product_id)
        {
        case KP_DEVICE_KL830:
        case KP_DEVICE_KL730:
        {
            npu_data_header_t *raw_cnn_res = (npu_data_header_t *)(ipc_result->mix_data + ipc_result->raw_data_offset);
            output_desc->num_output_node = raw_cnn_res->npu_data_node_num;
            break;
        }
        default:
            break;
        }

        output_desc->num_pre_proc_info = ipc_result->num_of_pre_proc_info;

        kp_hw_pre_proc_info_t *ipc_pre_proc_info = (kp_hw_pre_proc_info_t *)(ipc_result->mix_data + ipc_result->pre_proc_info_offset);

        memcpy(output_desc->pre_proc_info, ipc_pre_proc_info, output_desc->num_pre_proc_info * sizeof(kp_hw_pre_proc_info_t));

        if (ipc_result->is_last_crop == 1)
            _devices_grp->cur_recv++;
    }

    if (_devices_grp->cur_recv >= _devices_grp->num_device)
        _devices_grp->cur_recv = 0;

    return KP_SUCCESS;
}

int kp_generic_data_inference_send(kp_device_group_t devices, kp_generic_data_inference_desc_t *inf_data)
{
    int num_input_node_data = inf_data->num_input_node_data;
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_send++];

    if (_devices_grp->cur_send >= _devices_grp->num_device)
        _devices_grp->cur_send = 0;

    if ((KP_DEVICE_KL730 == _devices_grp->product_id) ||
        (KP_DEVICE_KL830 == _devices_grp->product_id)) {
        if (KP_MAX_INPUT_NODE_COUNT_V2 < num_input_node_data) {
            return KP_ERROR_INVALID_INPUT_NODE_DATA_NUMBER_48;
        }
    } else {
        if (KP_MAX_INPUT_NODE_COUNT_V1 < num_input_node_data) {
            return KP_ERROR_INVALID_INPUT_NODE_DATA_NUMBER_48;
        }
    }

    if (false == check_model_input_node_number_is_correct(_devices_grp, inf_data->model_id, num_input_node_data)) {
        return KP_ERROR_INVALID_PARAM_12;
    } else if (_devices_grp->ddr_attr.input_buffer_count < num_input_node_data) {
        return KP_ERROR_FIFOQ_INPUT_BUFF_COUNT_NOT_ENOUGH_42;
    }

    int timeout = _devices_grp->timeout;

    if (false == check_model_id_is_exist_in_nef(_devices_grp, inf_data->model_id)) {
        dbg_print("[%s] model id [%d] not exist in nef\n", __func__, inf_data->model_id);
        return KP_ERROR_MODEL_NOT_LOADED_35;
    }

    for (int i = 0; i < num_input_node_data; i++) {
        uint32_t buffer_size = inf_data->input_node_data_list[i].buffer_size;

        int ret = 0;

        kdp2_ipc_generic_raw_inf_bypass_pre_proc_header_t raw_inf_header;

        raw_inf_header.header_stamp.magic_type = KDP2_MAGIC_TYPE_INFERENCE;
        raw_inf_header.header_stamp.total_size = sizeof(raw_inf_header) + buffer_size;
        raw_inf_header.header_stamp.job_id = KDP2_INF_ID_GENERIC_RAW_BYPASS_PRE_PROC;
        raw_inf_header.header_stamp.total_image = num_input_node_data;
        raw_inf_header.header_stamp.image_index = i;

        if (raw_inf_header.header_stamp.total_size > _devices_grp->ddr_attr.input_buffer_size)
        {
            dbg_print("[%s] image buffer size is not enough in firmware\n", __func__);
            return KP_ERROR_SEND_DATA_TOO_LARGE_15;
        }

        raw_inf_header.inference_number = inf_data->inference_number;
        raw_inf_header.model_id = inf_data->model_id;
        raw_inf_header.image_buffer_size = buffer_size;

        ret = kp_usb_write_data(ll_dev, (void *)&raw_inf_header, sizeof(raw_inf_header), timeout);
        int status = check_inf_desc_error(ret);
        if (status != KP_SUCCESS)
            return status;

        ret = kp_usb_write_data(ll_dev, (void *)inf_data->input_node_data_list[i].buffer, buffer_size, timeout);
        status = check_send_image_error(ret);
        if (status != KP_SUCCESS)
            return status;
    }

    return KP_SUCCESS;
}

int kp_generic_data_inference_receive(kp_device_group_t devices, kp_generic_data_inference_result_header_t *output_desc, uint8_t *raw_out_buffer, uint32_t buf_size)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_recv];

    int timeout = _devices_grp->timeout;

    // if return < 0 means libusb error, otherwise return  received size
    int usb_ret = kp_usb_read_data(ll_dev, (void *)raw_out_buffer, buf_size, timeout);
    if (usb_ret < 0)
        return usb_ret;

    kp_inference_header_stamp_t *header_stamp = (kp_inference_header_stamp_t *)raw_out_buffer;
    int status = verify_result_header_stamp(header_stamp, 0, KDP2_INF_ID_GENERIC_RAW_BYPASS_PRE_PROC);

    if (status != KP_SUCCESS) {
        return status;
    }

    // parsing result buffer

    if (KDP2_MAGIC_TYPE_INFERENCE == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_bypass_pre_proc_result_t *ipc_result = (kdp2_ipc_generic_raw_bypass_pre_proc_result_t *)raw_out_buffer;

        output_desc->inference_number = ipc_result->inf_number;
        output_desc->crop_number = ipc_result->crop_number;
        output_desc->product_id = ipc_result->product_id;

        switch (ipc_result->product_id)
        {
        case KP_DEVICE_KL520:
        {
            output_desc->num_output_node = *(uint32_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_bypass_pre_proc_result_t));
            break;
        }
        case KP_DEVICE_KL720:
        {
            _720_raw_cnn_res_t *raw_cnn_res = (_720_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_bypass_pre_proc_result_t));
            output_desc->num_output_node = raw_cnn_res->total_nodes;
            break;
        }
        case KP_DEVICE_KL830:
        case KP_DEVICE_KL730:
        case KP_DEVICE_KL630:
        {
            _630_raw_cnn_res_t *raw_cnn_res = (_630_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_bypass_pre_proc_result_t));
            output_desc->num_output_node = raw_cnn_res->total_nodes;
            break;
        }
        default:
            break;
        }

        if (ipc_result->is_last_crop == 1)
            _devices_grp->cur_recv++;
    } else if (KDP2_MAGIC_TYPE_INFERENCE_V2 == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_bypass_pre_proc_result_t_v2 *ipc_result = (kdp2_ipc_generic_raw_bypass_pre_proc_result_t_v2 *)raw_out_buffer;

        output_desc->inference_number = ipc_result->inf_number;
        output_desc->crop_number = ipc_result->crop_number;
        output_desc->product_id = ipc_result->product_id;

        switch (ipc_result->product_id)
        {
        case KP_DEVICE_KL830:
        case KP_DEVICE_KL730:
        {
            npu_data_header_t *raw_cnn_res = (npu_data_header_t *)(ipc_result->mix_data + ipc_result->raw_data_offset);
            output_desc->num_output_node = raw_cnn_res->npu_data_node_num;
            break;
        }
        default:
            break;
        }

        if (ipc_result->is_last_crop == 1)
            _devices_grp->cur_recv++;
    }

    if (_devices_grp->cur_recv >= _devices_grp->num_device)
        _devices_grp->cur_recv = 0;

    return KP_SUCCESS;
}

#define KDP_COL_MIN_8       8
#define KDP_COL_MIN_16      16
#define KDP_CHANNEL_MIN_16  16
uint32_t round_up(uint32_t num, uint32_t round_num)
{
    return ((num + (round_num - 1)) & ~(round_num - 1));
}

uint32_t get_data_type_size(int32_t data_type)
{
    uint32_t data_type_size = 0;

    switch (data_type)
    {
    case KP_DTYPE_INT8:
        data_type_size = sizeof(int8_t);
        break;
    case KP_DTYPE_INT16:
        data_type_size = sizeof(int16_t);
        break;
    case KP_DTYPE_INT32:
        data_type_size = sizeof(int32_t);
        break;
    case KP_DTYPE_INT64:
        data_type_size = sizeof(int64_t);
        break;
    case KP_DTYPE_UINT8:
        data_type_size = sizeof(uint8_t);
        break;
    case KP_DTYPE_UINT16:
        data_type_size = sizeof(uint16_t);
        break;
    case KP_DTYPE_UINT32:
        data_type_size = sizeof(uint32_t);
        break;
    case KP_DTYPE_UINT64:
        data_type_size = sizeof(uint64_t);
        break;
    case KP_DTYPE_FLOAT32:
        data_type_size = sizeof(float);
        break;
    case KP_DTYPE_BFLOAT16:
        data_type_size = sizeof(uint16_t);
        break;
    case KP_DTYPE_DOUBLE64:
        data_type_size = sizeof(double);
        break;
    default:
        err_print("Invalid Data Type %u\n", data_type);
        break;
    }

    return data_type_size;
}

int fill_quantized_fix_point_descripter(kp_quantized_fixed_point_descriptor_t *quantized_fixed_point_descriptor_list, uint32_t quantized_parameters_len,
                                        uint32_t scale_type, uint32_t radix_type, void *scale_p, void *radix_p)
{
    int status = KP_SUCCESS;

    for (uint32_t idx = 0; idx < quantized_parameters_len; idx++) {
        kp_quantized_fixed_point_descriptor_t *quantized_fixed_point_descriptor = &quantized_fixed_point_descriptor_list[idx];

        quantized_fixed_point_descriptor->scale_dtype   = scale_type;

        switch (scale_type)
        {
        case KP_DTYPE_INT8:
            quantized_fixed_point_descriptor->scale.scale_int8      = ((int8_t *)scale_p)[idx];
            break;
        case KP_DTYPE_INT16:
            quantized_fixed_point_descriptor->scale.scale_int16     = ((int16_t *)scale_p)[idx];
            break;
        case KP_DTYPE_INT32:
            quantized_fixed_point_descriptor->scale.scale_int32     = ((int32_t *)scale_p)[idx];
            break;
        case KP_DTYPE_INT64:
            quantized_fixed_point_descriptor->scale.scale_int64     = ((int64_t *)scale_p)[idx];
            break;
        case KP_DTYPE_UINT8:
            quantized_fixed_point_descriptor->scale.scale_uint8     = ((uint8_t *)scale_p)[idx];
            break;
        case KP_DTYPE_UINT16:
            quantized_fixed_point_descriptor->scale.scale_uint16    = ((uint16_t *)scale_p)[idx];
            break;
        case KP_DTYPE_UINT32:
            quantized_fixed_point_descriptor->scale.scale_uint32    = ((uint32_t *)scale_p)[idx];
            break;
        case KP_DTYPE_UINT64:
            quantized_fixed_point_descriptor->scale.scale_uint64    = ((uint64_t *)scale_p)[idx];
            break;
        case KP_DTYPE_FLOAT32:
            quantized_fixed_point_descriptor->scale.scale_float32   = ((float *)scale_p)[idx];
            break;
        case KP_DTYPE_BFLOAT16:
            quantized_fixed_point_descriptor->scale.scale_bfloat16  = ((uint16_t *)scale_p)[idx];
            break;
        case KP_DTYPE_DOUBLE64:
            quantized_fixed_point_descriptor->scale.scale_double64  = ((double *)scale_p)[idx];
            break;
        default:
            err_print("construct nef single tensor information quantization parameters in model_descriptor fail: invalid KneronKNE_DataType_enum_t\n");
            status = KP_ERROR_INVALID_MODEL_21;
            goto FUNC_OUT;
        }

        switch (radix_type)
        {
        case KP_DTYPE_INT8:
            quantized_fixed_point_descriptor->radix = ((int8_t *)radix_p)[idx];
            break;
        case KP_DTYPE_INT16:
            quantized_fixed_point_descriptor->radix = ((int16_t *)radix_p)[idx];
            break;
        case KP_DTYPE_INT32:
            quantized_fixed_point_descriptor->radix = ((int32_t *)radix_p)[idx];
            break;
        default:
            err_print("construct nef single tensor information quantization parameters in model_descriptor fail: invalid KneronKNE_DataType_enum_t\n");
            status = KP_ERROR_INVALID_MODEL_21;
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}

kp_inf_raw_fixed_node_output_t *kp_generic_inference_retrieve_raw_fixed_node(uint32_t node_idx, uint8_t *raw_out_buffer)
{
    kp_inference_header_stamp_t *header_stamp                   = (kp_inference_header_stamp_t *)raw_out_buffer;
    kdp2_ipc_generic_raw_result_t_v1 *raw_result_v1             = NULL;
    kdp2_ipc_generic_raw_result_t_v2 *raw_result_v2             = NULL;
    kp_inf_raw_fixed_node_output_t *node_output                 = NULL;
    _kl520_output_node_metadata_t *kl520_node_desc              = NULL;

    kp_tensor_descriptor_t *tensor_descriptor                   = NULL;
    kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
    kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = NULL;
    kp_tensor_shape_info_v2_t *tensor_shape_info_v2             = NULL;
    kp_quantization_parameters_t *quantization_parameters       = NULL;
    kp_quantization_parameters_v1_t *quantization_parameters_v1 = NULL;

    uint32_t raw_offset                                         = 0;
    uint8_t *data_start                                         = 0;
    uint32_t out_node_num                                       = 0;
    _720_raw_cnn_res_t *pRawHead_720                            = NULL;
    _630_raw_cnn_res_t *pRawHead_630                            = NULL;
    npu_data_header_t *npu_data_heade                           = NULL;
    npu_data_single_node_header_v2_t *output_node_header        = NULL;
    int32_t *radix_p                                            = NULL;
    void *scale_p                                               = NULL;

    (void)tensor_shape_info_v2;

    if (KDP2_MAGIC_TYPE_INFERENCE == header_stamp->magic_type) {
        raw_result_v1 = (kdp2_ipc_generic_raw_result_t *)raw_out_buffer;

        switch (raw_result_v1->product_id)
        {
        case KP_DEVICE_KL520:
        {
            data_start      = raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t);
            out_node_num    = *(uint32_t *)data_start;
            if (node_idx > out_node_num - 1) {
                printf("%s, invalid node index.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output = (kp_inf_raw_fixed_node_output_t *)calloc(1, sizeof(kp_inf_raw_fixed_node_output_t));
            if (NULL == node_output) {
                printf("%s, memory is insufficient to allocate buffer for raw fixed node.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            kl520_node_desc = (_kl520_output_node_metadata_t *)(data_start + 4);

            raw_offset = 4 + out_node_num * sizeof(_kl520_output_node_metadata_t);
            for (int i = 0; i < node_idx; i++)
                raw_offset += kl520_node_desc[i].height * kl520_node_desc[i].channel * round_up(kl520_node_desc[i].width, KDP_COL_MIN_16); // Note: Currently, kl520 output is only support 16W1C8B npu data layout.

            // Copy fixed node output metadata to align with KL720
            // memcpy(&node_output->metadata, data_start + 4 + node_idx * sizeof(kp_inf_raw_fixed_node_metadata_t), sizeof(kp_inf_raw_fixed_node_metadata_t));
            node_output->num_data                               = kl520_node_desc[node_idx].height * kl520_node_desc[node_idx].channel * round_up(kl520_node_desc[node_idx].width, KDP_COL_MIN_16);
            node_output->data                                   = (int8_t *)(data_start + raw_offset);

            tensor_descriptor                                   = &(node_output->metadata.tensor_descriptor);
            tensor_descriptor->index                            = node_idx;
            tensor_descriptor->name                             = strcpy_dst_realloc(tensor_descriptor->name, "");
            tensor_descriptor->data_layout                      = convert_data_format_to_kp_tensor_format(kl520_node_desc[node_idx].data_layout, KP_MODEL_TARGET_CHIP_KL520);

            tensor_shape_info                                   = &(tensor_descriptor->tensor_shape_info);
            tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

            tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
            tensor_shape_info_v1->shape_npu_len                 = 4;
            tensor_shape_info_v1->shape_onnx_len                = 4;
            tensor_shape_info_v1->axis_permutation_len          = 4;
            tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
            tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
            tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

            quantization_parameters                                             = &(tensor_descriptor->quantization_parameters);
            quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

            quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
            quantization_parameters_v1->quantized_axis                          = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

            if (KP_SUCCESS != is_tensor_info_reallocted(tensor_descriptor)) {
                printf("%s, memory is insufficient to allocate buffer for tensor information.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            tensor_shape_info_v1->shape_npu[0]  = 1;
            tensor_shape_info_v1->shape_npu[1]  = kl520_node_desc[node_idx].channel;
            tensor_shape_info_v1->shape_npu[2]  = kl520_node_desc[node_idx].height;
            tensor_shape_info_v1->shape_npu[3]  = kl520_node_desc[node_idx].width;

            memcpy(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len * sizeof(int32_t));

            for (int axis_permutation_idx = 0; axis_permutation_idx < tensor_shape_info_v1->axis_permutation_len; axis_permutation_idx++) {
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[axis_permutation_idx] = axis_permutation_idx;
            }

            quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = kl520_node_desc[node_idx].radix;
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = kl520_node_desc[node_idx].scale;
        }
        break;

        case KP_DEVICE_KL720:
        {
            pRawHead_720 = (_720_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t));
            if (node_idx > pRawHead_720->total_nodes - 1) {
                printf("%s, invalid node index.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output = (kp_inf_raw_fixed_node_output_t *)calloc(1, sizeof(kp_inf_raw_fixed_node_output_t));
            if (NULL == node_output) {
                printf("%s, memory is insufficient to allocate buffer for raw fixed node.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output->num_data                               = pRawHead_720->total_raw_len;
            node_output->data                                   = (int8_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t) + sizeof(_720_raw_cnn_res_t) + pRawHead_720->onode_a[node_idx].start_offset);

            tensor_descriptor                                   = &(node_output->metadata.tensor_descriptor);
            tensor_descriptor->index                            = pRawHead_720->onode_a[node_idx].output_index;
            tensor_descriptor->name                             = strcpy_dst_realloc(tensor_descriptor->name, "");
            tensor_descriptor->data_layout                      = convert_data_format_to_kp_tensor_format(pRawHead_720->onode_a[node_idx].data_format, KP_MODEL_TARGET_CHIP_KL720);

            tensor_shape_info                                   = &(tensor_descriptor->tensor_shape_info);
            tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

            tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
            tensor_shape_info_v1->shape_npu_len                 = 4;
            tensor_shape_info_v1->shape_onnx_len                = 4;
            tensor_shape_info_v1->axis_permutation_len          = 4;
            tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
            tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
            tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

            quantization_parameters                                             = &(tensor_descriptor->quantization_parameters);
            quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

            quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
            quantization_parameters_v1->quantized_axis                          = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

            if (KP_SUCCESS != is_tensor_info_reallocted(tensor_descriptor)) {
                printf("%s, memory is insufficient to allocate buffer for tensor information.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            tensor_shape_info_v1->shape_npu[0] = 1;
            tensor_shape_info_v1->shape_npu[1] = pRawHead_720->onode_a[node_idx].ch_length;
            tensor_shape_info_v1->shape_npu[2] = pRawHead_720->onode_a[node_idx].row_length;
            tensor_shape_info_v1->shape_npu[3] = pRawHead_720->onode_a[node_idx].col_length;

            memcpy(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len * sizeof(int32_t));

            for (int axis_permutation_idx = 0; axis_permutation_idx < tensor_shape_info_v1->axis_permutation_len; axis_permutation_idx++) {
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[axis_permutation_idx] = axis_permutation_idx;
            }

            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = *(float *)(&pRawHead_720->onode_a[node_idx].output_scale);
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = (int32_t)pRawHead_720->onode_a[node_idx].output_radix;
        }
        break;

        case KP_DEVICE_KL630:
        {
            pRawHead_630 = (_630_raw_cnn_res_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t));
            if (node_idx > (uint32_t)(pRawHead_630->total_nodes - 1)) {
                printf("%s, invalid node index.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output = (kp_inf_raw_fixed_node_output_t *)calloc(1, sizeof(kp_inf_raw_fixed_node_output_t));
            if (NULL == node_output) {
                printf("%s, memory is insufficient to allocate buffer for raw fixed node.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output->num_data                               = pRawHead_630->total_raw_len;
            node_output->data                                   = (int8_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t) + sizeof(_630_raw_cnn_res_t) + pRawHead_630->onode_a[node_idx].start_offset);

            tensor_descriptor                                   = &(node_output->metadata.tensor_descriptor);
            tensor_descriptor->index                            = pRawHead_630->onode_a[node_idx].idx;
            tensor_descriptor->name                             = strcpy_dst_realloc(tensor_descriptor->name, "");
            tensor_descriptor->data_layout                      = convert_data_format_to_kp_tensor_format(pRawHead_630->onode_a[node_idx].fmt, KP_MODEL_TARGET_CHIP_KL630);

            tensor_shape_info                                   = &(tensor_descriptor->tensor_shape_info);
            tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

            tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
            tensor_shape_info_v1->shape_npu_len                 = 4;
            tensor_shape_info_v1->shape_onnx_len                = 4;
            tensor_shape_info_v1->axis_permutation_len          = 4;
            tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
            tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
            tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

            quantization_parameters                                             = &(tensor_descriptor->quantization_parameters);
            quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

            quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
            quantization_parameters_v1->quantized_axis                          = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

            if (KP_SUCCESS != is_tensor_info_reallocted(tensor_descriptor)) {
                printf("%s, memory is insufficient to allocate buffer for tensor information.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            tensor_shape_info_v1->shape_npu[0] = pRawHead_630->onode_a[node_idx].batch;
            tensor_shape_info_v1->shape_npu[1] = pRawHead_630->onode_a[node_idx].ch_length;
            tensor_shape_info_v1->shape_npu[2] = pRawHead_630->onode_a[node_idx].row_length;
            tensor_shape_info_v1->shape_npu[3] = pRawHead_630->onode_a[node_idx].col_length;

            memcpy(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len * sizeof(int32_t));

            for (int axis_permutation_idx = 0; axis_permutation_idx < tensor_shape_info_v1->axis_permutation_len; axis_permutation_idx++) {
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[axis_permutation_idx] = axis_permutation_idx;
            }

            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = *(float *)(&pRawHead_630->onode_a[node_idx].scale);
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = (int32_t)pRawHead_630->onode_a[node_idx].radix;
        }
        break;

        default:
            printf("%s, KP_DEVICE %d is not supported.\n", __func__, raw_result_v1->product_id);
            goto FUNC_OUT_ERROR;
        break;
        }
    } else if (KDP2_MAGIC_TYPE_INFERENCE_V2 == header_stamp->magic_type) {
        raw_result_v2 = (kdp2_ipc_generic_raw_result_t_v2 *)raw_out_buffer;

        switch (raw_result_v2->product_id)
        {
        case KP_DEVICE_KL830:
        case KP_DEVICE_KL730:
        {
            npu_data_heade = (npu_data_header_t *)(raw_out_buffer + sizeof(kdp2_ipc_generic_raw_result_t_v2) + (raw_result_v2->num_of_pre_proc_info * sizeof(kp_hw_pre_proc_info_t)));

            if (node_idx > (uint32_t)(npu_data_heade->npu_data_node_num - 1)) {
                printf("%s, invalid node index.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            output_node_header = &(((npu_data_single_node_header_v2_t *)npu_data_heade->data)[node_idx]);

            if ((KP_DTYPE_INT32 != output_node_header->shape_data_type) ||
                (KP_DTYPE_UINT32 != output_node_header->stride_npu_data_type) ||
                (KP_DTYPE_UINT32 != output_node_header->stride_onnx_data_type)) {
                printf("%s, unsupport IPC shape data type.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output = (kp_inf_raw_fixed_node_output_t *)calloc(1, sizeof(kp_inf_raw_fixed_node_output_t));
            if (NULL == node_output) {
                printf("%s, memory is insufficient to allocate buffer for raw fixed node.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            node_output->num_data               = output_node_header->npu_data_len;
            node_output->data                   = (int8_t *)(npu_data_heade->data + output_node_header->npu_data_start_offset);

            tensor_descriptor                   = &(node_output->metadata.tensor_descriptor);
            tensor_descriptor->index            = output_node_header->index;
            tensor_descriptor->name             = strcpy_dst_realloc(tensor_descriptor->name, (char *)(npu_data_heade->data + output_node_header->name_start_offset));
            tensor_descriptor->data_layout      = convert_data_format_to_kp_tensor_format(output_node_header->data_layout, KP_MODEL_TARGET_CHIP_KL730);

            tensor_shape_info                   = &(tensor_descriptor->tensor_shape_info);
            tensor_shape_info->version          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2;

            tensor_shape_info_v2                = &(tensor_shape_info->tensor_shape_info_data.v2);
            tensor_shape_info_v2->shape_len     = output_node_header->shape_len;
            tensor_shape_info_v2->shape         = realloc_tensor_shape_int32_t(tensor_shape_info_v2->shape, tensor_shape_info_v2->shape_len);
            tensor_shape_info_v2->stride_npu    = realloc_tensor_shape_uint32_t(tensor_shape_info_v2->stride_npu, tensor_shape_info_v2->shape_len);
            tensor_shape_info_v2->stride_onnx   = realloc_tensor_shape_uint32_t(tensor_shape_info_v2->stride_onnx, tensor_shape_info_v2->shape_len);

            quantization_parameters                                             = &(tensor_descriptor->quantization_parameters);
            quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

            quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
            quantization_parameters_v1->quantized_axis                          = output_node_header->quantized_axis;
            quantization_parameters_v1->quantized_fixed_point_descriptor_num    = output_node_header->quantized_parameters_len;
            quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

            if (KP_SUCCESS != is_tensor_info_reallocted(tensor_descriptor)) {
                printf("%s, memory is insufficient to allocate buffer for tensor information.\n", __func__);
                goto FUNC_OUT_ERROR;
            }

            memcpy(tensor_shape_info_v2->shape,
                   (void *)(npu_data_heade->data + output_node_header->shape_start_offset),
                   output_node_header->shape_len * get_data_type_size(output_node_header->shape_data_type));

            memcpy(tensor_shape_info_v2->stride_npu,
                   (uint32_t *)(npu_data_heade->data + output_node_header->stride_npu_start_offset),
                   output_node_header->shape_len * get_data_type_size(output_node_header->stride_npu_data_type));

            memcpy(tensor_shape_info_v2->stride_onnx,
                   (uint32_t *)(npu_data_heade->data + output_node_header->stride_onnx_start_offset),
                   output_node_header->shape_len * get_data_type_size(output_node_header->stride_onnx_data_type));

            radix_p = (int32_t *)(npu_data_heade->data + output_node_header->radix_start_offset);
            scale_p = (void *)(npu_data_heade->data + output_node_header->scale_start_offset);

            if (KP_SUCCESS != fill_quantized_fix_point_descripter(quantization_parameters_v1->quantized_fixed_point_descriptor, output_node_header->quantized_parameters_len, output_node_header->scale_data_type, output_node_header->radix_data_type, scale_p, radix_p)) {
                printf("%s, construct quantization parameters fail.\n", __func__);
                goto FUNC_OUT_ERROR;
            }
        }
        break;

        default:
            printf("%s, KP_DEVICE %d is not supported.\n", __func__, raw_result_v2->product_id);
            goto FUNC_OUT_ERROR;
        break;
        }
    }

    return node_output;

FUNC_OUT_ERROR:
    kp_release_raw_fixed_node_output(node_output);
    node_output = NULL;

    return node_output;
}

#define SIZE_OF_FIXED_NODE_DATA 4 // sizeof(int16_t) + padding size for align 4 (ref. kp_inf_fixed_node_output_t)

kp_inf_fixed_node_output_t *kp_generic_inference_retrieve_fixed_node(uint32_t node_idx, uint8_t *raw_out_buffer, kp_channel_ordering_t ordering)
{
    kp_inf_raw_fixed_node_output_t *raw_fixed_node_output       = kp_generic_inference_retrieve_raw_fixed_node(node_idx, raw_out_buffer);
    kp_inference_header_stamp_t *header_stamp                   = (kp_inference_header_stamp_t *)raw_out_buffer;
    kp_channel_ordering_convert_t channel_ordering_convert_code = KP_CHANNEL_ORDERING_CVT_NONE;
    uint32_t product_id                                         = KP_DEVICE_KL520;

    kp_inf_fixed_node_output_t *fixed_node_output               = NULL;
    kp_tensor_descriptor_t *tensor_descriptor                   = NULL;
    kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
    kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = NULL;
    kp_tensor_shape_info_v2_t *tensor_shape_info_v2             = NULL;
    kp_quantization_parameters_t *quantization_parameters_src   = NULL;
    kp_quantization_parameters_t *quantization_parameters_dst   = NULL;

    uint32_t fixed_point_dtype                                  = 0;
    uint32_t qunat_version                                      = 0;
    uint32_t shape_version                                      = 0;
    uint32_t shape_len                                          = 0;
    uint32_t num_data                                           = 0;
    int32_t *shape_p                                            = 0;
    uint32_t data_size                                          = 0;
    uint32_t data_layout                                        = 0;

    int width_aligned                                           = 0;
    int width                                                   = 0;
    int height                                                  = 0;
    int channel                                                 = 0;
    int n                                                       = 0;

    int channel_block_idx                                       = 0;
    int channel_offset_idx                                      = 0;
    int channel_block_size                                      = 0;

    int channel_idx                                             = 0;
    int npu_channel_group_stride_tmp                            = 0;
    int npu_channel_group_stride                                = 0;

    int32_t *onnx_data_shape_index                              = NULL;
    uint32_t onnx_data_buf_offset                               = 0;
    uint32_t npu_data_buf_offset                                = 0;

    uint16_t npu_data_element_16b                               = 0;
    uint16_t npu_data_high_bit_offset                           = 16;

    if (KDP2_MAGIC_TYPE_INFERENCE == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t *raw_result = (kdp2_ipc_generic_raw_result_t *)raw_out_buffer;

        product_id                      = raw_result->product_id;
        channel_ordering_convert_code   = get_channel_ordering_convert_code(product_id, ordering);
    } else if (KDP2_MAGIC_TYPE_INFERENCE_V2 == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t_v2 *raw_result = (kdp2_ipc_generic_raw_result_t_v2 *)raw_out_buffer;

        product_id                      = raw_result->product_id;
        channel_ordering_convert_code   = get_channel_ordering_convert_code(product_id, ordering);
    } else {
        printf("%s, invalid header stamp.\n", __func__);
        goto FUNC_OUT_ERROR;
    }

    if (NULL == raw_fixed_node_output) {
        printf("%s, parse raw fixed node fail.\n", __func__);
        goto FUNC_OUT_ERROR;
    }

    fixed_node_output           = NULL;
    tensor_descriptor           = &(raw_fixed_node_output->metadata.tensor_descriptor);
    tensor_shape_info           = &(tensor_descriptor->tensor_shape_info);
    tensor_shape_info_v1        = &(tensor_shape_info->tensor_shape_info_data.v1);
    tensor_shape_info_v2        = &(tensor_shape_info->tensor_shape_info_data.v2);
    quantization_parameters_src = &(tensor_descriptor->quantization_parameters);

    data_layout                 = raw_fixed_node_output->metadata.tensor_descriptor.data_layout;
    fixed_point_dtype           = get_fixed_point_dtype(data_layout);
    qunat_version               = quantization_parameters_src->version;
    shape_version               = tensor_shape_info->version;
    shape_len                   = 1;
    num_data                    = 1;
    shape_p                     = NULL;

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == shape_version) {
        shape_len   = tensor_shape_info_v1->shape_onnx_len;
        shape_p     = tensor_shape_info_v1->shape_onnx;
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == shape_version) {
        shape_len   = tensor_shape_info_v2->shape_len;
        shape_p     = tensor_shape_info_v2->shape;
    } else {
        printf("%s, invalid tensor shape version.\n", __func__);
        goto FUNC_OUT_ERROR;
    }

    for (uint32_t shape_idx = 0; shape_idx < shape_len; shape_idx++)
        num_data *= shape_p[shape_idx];

    data_size           = num_data * ((KP_FIXED_POINT_DTYPE_INT16 == fixed_point_dtype) ? sizeof(int16_t) : sizeof(int8_t));
    fixed_node_output   = (kp_inf_fixed_node_output_t *)malloc(sizeof(kp_inf_fixed_node_output_t) - SIZE_OF_FIXED_NODE_DATA + data_size);
    if (NULL == fixed_node_output) {
        printf("memory is insufficient to allocate buffer for node output.\n");
        goto FUNC_OUT_ERROR;
    }

    memset(fixed_node_output, 0, sizeof(kp_inf_fixed_node_output_t));

    fixed_node_output->fixed_point_dtype    = fixed_point_dtype;
    fixed_node_output->num_data             = num_data;
    fixed_node_output->shape_len            = shape_len;
    fixed_node_output->shape                = (int32_t *)calloc(shape_len, sizeof(int32_t));
    fixed_node_output->name                 = strcpy_dst_realloc(fixed_node_output->name, tensor_descriptor->name);
    if ((NULL == fixed_node_output->shape) ||
        (NULL == fixed_node_output->name)) {
        printf("memory is insufficient to allocate buffer for node output.\n");
        goto FUNC_OUT_ERROR;
    }

    memcpy(fixed_node_output->shape, shape_p, shape_len * sizeof(int32_t));

    if (KP_MODEL_QUANTIZATION_PARAMS_VERSION_1 == qunat_version) {
        quantization_parameters_dst = &(fixed_node_output->quantization_parameters);
        if (KP_SUCCESS != copy_single_tensor_info_quantization_parameters(quantization_parameters_dst, quantization_parameters_src)) {
            printf("%s, build quantization parameter fail.\n", __func__);
            goto FUNC_OUT_ERROR;
        }
    } else {
        printf("%s, invalid quantization parameters version.\n", __func__);
        goto FUNC_OUT_ERROR;
    }

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == shape_version) {
        width   = tensor_shape_info_v1->shape_npu[3];
        height  = tensor_shape_info_v1->shape_npu[2];
        channel = tensor_shape_info_v1->shape_npu[1];
        n       = 0;

        if (KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B == data_layout)
        {
            /* standard 16-bit fixed-point output */
            width_aligned = round_up(width, KDP_COL_MIN_8);

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
                for (int c = 0; c < channel; c++)
                {
                    for (int h = 0; h < height; h++)
                    {
                        for (int w = 0; w < width; w++)
                            fixed_node_output->data.int16[n++] = ((int16_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                            (c * width_aligned) +
                                                                                                            w];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        for (int w = 0; w < width; w++)
                            fixed_node_output->data.int16[n++] = ((int16_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                            (h * width_aligned) +
                                                                                                            w];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            fixed_node_output->data.int16[n++] = ((int16_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                            (c * width_aligned) +
                                                                                                            w];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            fixed_node_output->data.int16[n++] = ((int16_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                            (h * width_aligned) +
                                                                                                            w];
                    }
                }
                break;
            default:
                for (int i = 0; i < height * channel; i++)
                {
                    for (int j = 0; j < width; j++)
                        fixed_node_output->data.int16[n++] = ((int16_t *)(raw_fixed_node_output->data))[i * width_aligned + j];
                }
                break;
            }
        }
        else if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B == data_layout)
        {
            /* 8-bit fixed-point output */
            channel_block_idx   = 0;
            channel_offset_idx  = 0;
            channel_block_size  = height * width * KDP_CHANNEL_MIN_16;

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                /* KL520 not support 1W16C8B ouput NPU data layout format */
                printf("Invalid NPU data layout of HCW to CHW/HWC channel order conversion, NPU data layout = KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B.\n");
                goto FUNC_OUT_ERROR;
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        channel_block_idx   = c / KDP_CHANNEL_MIN_16;
                        channel_offset_idx  = c % KDP_CHANNEL_MIN_16;
                        for (int w = 0; w < width; w++)
                            fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                          (h * width * KDP_CHANNEL_MIN_16) +
                                                                                                          (w * KDP_CHANNEL_MIN_16) +
                                                                                                          (channel_offset_idx)];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                        {
                            channel_block_idx                   = c / KDP_CHANNEL_MIN_16;
                            channel_offset_idx                  = c % KDP_CHANNEL_MIN_16;
                            fixed_node_output->data.int8[n++]   = ((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                            (h * width * KDP_CHANNEL_MIN_16) +
                                                                                                            (w * KDP_CHANNEL_MIN_16) +
                                                                                                            (channel_offset_idx)];
                        }
                    }
                }
                break;
            default:
                for (int c = 0; c < channel; c++)
                {
                    channel_block_idx   = c / KDP_CHANNEL_MIN_16;
                    channel_offset_idx  = c % KDP_CHANNEL_MIN_16;
                    for (int i = 0; i < height * width; i++)
                        fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                      (i * KDP_CHANNEL_MIN_16) +
                                                                                                      (channel_offset_idx)];
                }
                break;
            }
        }
        else
        {
            /* standard 8-bit fixed-point output */
            width_aligned = round_up(width, KDP_COL_MIN_16);

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
                for (int c = 0; c < channel; c++)
                {
                    for (int h = 0; h < height; h++)
                    {
                        for (int w = 0; w < width; w++) {
                            fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                          (c * width_aligned) +
                                                                                                          w];
                        }
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        for (int w = 0; w < width; w++)
                            fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                          (h * width_aligned) +
                                                                                                          w];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                          (c * width_aligned) +
                                                                                                          w];
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                          (h * width_aligned) +
                                                                                                          w];
                    }
                }
                break;
            default:
                for (int i = 0; i < height * channel; i++)
                {
                    for (int j = 0; j < width; j++)
                        fixed_node_output->data.int8[n++] = ((int8_t *)(raw_fixed_node_output->data))[i * width_aligned + j];
                }
                break;
            }
        }
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == shape_version) {
        if (KP_CHANNEL_ORDERING_CVT_NONE != channel_ordering_convert_code) {
            err_print("Device 0x%X only support ordering 'KP_CHANNEL_ORDERING_DEFAULT'.\n", product_id);
            goto FUNC_OUT_ERROR;
        } else {
            /* convert NPU formatted data to ONNX sequential data */
            {
                onnx_data_shape_index = calloc(tensor_shape_info_v2->shape_len, sizeof(int32_t));

                if (NULL == onnx_data_shape_index) {
                    printf("error: malloc working buffer onnx_data_shape_index fail ...\n");
                    goto FUNC_OUT_ERROR;
                }

                switch (data_layout)
                {
                case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B_CH_COMPACT:
                case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B == data_layout) {
                            for (int axis = 0; axis < (int)tensor_shape_info_v2->shape_len; axis++) {
                                if (1 == tensor_shape_info_v2->stride_npu[axis]) {
                                    channel_idx = axis;
                                    continue;
                                }

                                npu_channel_group_stride_tmp = tensor_shape_info_v2->stride_npu[axis] * tensor_shape_info_v2->shape[axis];
                                if (npu_channel_group_stride_tmp > npu_channel_group_stride)
                                    npu_channel_group_stride = npu_channel_group_stride_tmp;
                            }

                            npu_channel_group_stride -= 16;

                            /* npu_data_buf_offset += (onnx_data_shape_index[channel_idx] / 16) * npu_channel_group_stride; */
                            npu_data_buf_offset += (onnx_data_shape_index[channel_idx] >> 4) * npu_channel_group_stride;
                        }

                        ((int8_t *)fixed_node_output->data.int8)[onnx_data_buf_offset] = ((int8_t *)raw_fixed_node_output->data)[npu_data_buf_offset];

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                case KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        npu_data_element_16b                                                = ((uint16_t *)raw_fixed_node_output->data)[npu_data_buf_offset];
                        ((uint16_t *)fixed_node_output->data.int16)[onnx_data_buf_offset]   = (npu_data_element_16b & 0xfffeu);

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL_CH_COMPACT:
                case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL == data_layout) {
                            for (int axis = 0; axis < (int)tensor_shape_info_v2->shape_len; axis++) {
                                if (1 == tensor_shape_info_v2->stride_npu[axis]) {
                                    channel_idx = axis;
                                    continue;
                                }

                                npu_channel_group_stride_tmp = tensor_shape_info_v2->stride_npu[axis] * tensor_shape_info_v2->shape[axis];
                                if (npu_channel_group_stride_tmp > npu_channel_group_stride)
                                    npu_channel_group_stride = npu_channel_group_stride_tmp;
                            }

                            npu_channel_group_stride -= 16;

                            /* npu_data_buf_offset += (onnx_data_shape_index[channel_idx] / 16) * npu_channel_group_stride; */
                            npu_data_buf_offset += (onnx_data_shape_index[channel_idx] >> 4) * npu_channel_group_stride;
                        }

                        /* npu_data_buf_offset = (npu_data_buf_offset / 16) * 32 + (npu_data_buf_offset % 16) */
                        npu_data_buf_offset = ((npu_data_buf_offset >> 4) << 5) + (npu_data_buf_offset & 15u);

                        fixed_node_output->data.int16[onnx_data_buf_offset] = (int16_t)(((((uint16_t)(((uint8_t *)raw_fixed_node_output->data)[npu_data_buf_offset])) & 0x007fu) +
                                                                                         (((uint16_t)(((uint8_t *)raw_fixed_node_output->data)[npu_data_buf_offset + npu_data_high_bit_offset])) << 7)) << 1);

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                default:
                    printf("error: get invalide data layout ...\n");
                    goto FUNC_OUT_ERROR;
                }
            }
        }
    }

    kp_release_raw_fixed_node_output(raw_fixed_node_output);

    if (NULL != onnx_data_shape_index)
        free(onnx_data_shape_index);

    return fixed_node_output;

FUNC_OUT_ERROR:
    kp_release_fixed_node_output(fixed_node_output);
    kp_release_raw_fixed_node_output(raw_fixed_node_output);
    fixed_node_output = NULL;

    if (NULL != onnx_data_shape_index)
        free(onnx_data_shape_index);

    return fixed_node_output;
}

kp_inf_float_node_output_t *kp_generic_inference_retrieve_float_node(uint32_t node_idx, uint8_t *raw_out_buffer, kp_channel_ordering_t ordering)
{
    kp_inf_raw_fixed_node_output_t *raw_fixed_node_output       = kp_generic_inference_retrieve_raw_fixed_node(node_idx, raw_out_buffer);
    kp_inference_header_stamp_t *header_stamp                   = (kp_inference_header_stamp_t *)raw_out_buffer;
    kp_channel_ordering_convert_t channel_ordering_convert_code = KP_CHANNEL_ORDERING_CVT_NONE;
    uint32_t product_id                                         = KP_DEVICE_KL520;

    kp_inf_float_node_output_t *float_node_output               = NULL;
    kp_tensor_descriptor_t *tensor_descriptor                   = NULL;
    kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
    kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = NULL;
    kp_tensor_shape_info_v2_t *tensor_shape_info_v2             = NULL;
    kp_quantization_parameters_t *quantization_parameters       = NULL;
    kp_quantization_parameters_v1_t *quantization_parameters_v1 = NULL;

    float quantization_factor                                   = 0;
    int quantized_axis_stride                                   = 0;
    int quantized_fixed_point_descriptor_idx                    = 0;
    bool is_channel_wise_quantization                           = false;

    uint32_t shape_version                                      = 0;
    uint32_t shape_len                                          = 0;
    uint32_t num_data                                           = 0;
    int32_t *shape_p                                            = 0;
    uint32_t data_layout                                        = 0;

    int width_aligned                                           = 0;
    int width                                                   = 0;
    int height                                                  = 0;
    int channel                                                 = 0;
    int n                                                       = 0;

    int channel_block_idx                                       = 0;
    int channel_offset_idx                                      = 0;
    int channel_block_size                                      = 0;

    int channel_idx                                             = 0;
    int npu_channel_group_stride_tmp                            = 0;
    int npu_channel_group_stride                                = 0;

    int32_t *onnx_data_shape_index                              = NULL;
    uint32_t onnx_data_buf_offset                               = 0;
    uint32_t npu_data_buf_offset                                = 0;

    uint16_t npu_data_element_16b                               = 0;
    uint16_t npu_data_high_bit_offset                           = 16;

    if (KDP2_MAGIC_TYPE_INFERENCE == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t *raw_result = (kdp2_ipc_generic_raw_result_t *)raw_out_buffer;

        product_id                      = raw_result->product_id;
        channel_ordering_convert_code   = get_channel_ordering_convert_code(product_id, ordering);
    } else if (KDP2_MAGIC_TYPE_INFERENCE_V2 == header_stamp->magic_type) {
        kdp2_ipc_generic_raw_result_t_v2 *raw_result = (kdp2_ipc_generic_raw_result_t_v2 *)raw_out_buffer;

        product_id                      = raw_result->product_id;
        channel_ordering_convert_code   = get_channel_ordering_convert_code(product_id, ordering);
    } else {
        goto FUNC_OUT_ERROR;
    }

    if (NULL == raw_fixed_node_output)
        goto FUNC_OUT_ERROR;

    float_node_output               = NULL;
    tensor_descriptor               = &(raw_fixed_node_output->metadata.tensor_descriptor);
    tensor_shape_info               = &(tensor_descriptor->tensor_shape_info);
    tensor_shape_info_v1            = &(tensor_shape_info->tensor_shape_info_data.v1);
    tensor_shape_info_v2            = &(tensor_shape_info->tensor_shape_info_data.v2);
    quantization_parameters         = &(tensor_descriptor->quantization_parameters);
    quantization_parameters_v1      = &(quantization_parameters->quantization_parameters_data.v1);
    is_channel_wise_quantization    = (1 < quantization_parameters_v1->quantized_fixed_point_descriptor_num);

    data_layout                     = raw_fixed_node_output->metadata.tensor_descriptor.data_layout;
    shape_version                   = tensor_shape_info->version;
    shape_len                       = 1;
    num_data                        = 1;
    shape_p                         = NULL;

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == shape_version) {
        shape_len   = tensor_shape_info_v1->shape_onnx_len;
        shape_p     = tensor_shape_info_v1->shape_onnx;
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == shape_version) {
        shape_len   = tensor_shape_info_v2->shape_len;
        shape_p     = tensor_shape_info_v2->shape;
    } else {
        goto FUNC_OUT_ERROR;
    }

    for (uint32_t shape_idx = 0; shape_idx < shape_len; shape_idx++)
        num_data *= shape_p[shape_idx];

    float_node_output = (kp_inf_float_node_output_t *)malloc(sizeof(kp_inf_float_node_output_t) + num_data * sizeof(float));
    if (NULL == float_node_output) {
        printf("memory is insufficient to allocate buffer for node output\n");
        goto FUNC_OUT_ERROR;
    }

    memset(float_node_output, 0, sizeof(kp_inf_float_node_output_t));

    float_node_output->num_data     = num_data;
    float_node_output->shape_len    = shape_len;
    float_node_output->shape        = (int32_t *)calloc(shape_len, sizeof(int32_t));
    float_node_output->name         = strcpy_dst_realloc(float_node_output->name, tensor_descriptor->name);
    if ((NULL == float_node_output->shape) ||
        (NULL == float_node_output->name)) {
        printf("memory is insufficient to allocate buffer for node output.\n");
        goto FUNC_OUT_ERROR;
    }

    memcpy(float_node_output->shape, shape_p, shape_len * sizeof(int32_t));

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == shape_version) {
        if (KP_SUCCESS != get_quantization_parameters_factor(quantization_parameters_v1,
                                                             false,
                                                             0,
                                                             0,
                                                             &quantized_fixed_point_descriptor_idx,
                                                             &quantization_factor)) {
            printf("error: get quantization parameters factor fail ...\n");
            goto FUNC_OUT_ERROR;
        }

        width   = tensor_shape_info_v1->shape_npu[3];
        height  = tensor_shape_info_v1->shape_npu[2];
        channel = tensor_shape_info_v1->shape_npu[1];
        n       = 0;

        if (KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B == data_layout)
        {
            /* standard 16-bit floating-point output */
            width_aligned = round_up(width, KDP_COL_MIN_8);

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
                for (int c = 0; c < channel; c++)
                {
                    for (int h = 0; h < height; h++)
                    {
                        for (int w = 0; w < width; w++)
                            float_node_output->data[n++] = (float)((int16_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                             (c * width_aligned) +
                                                                                                             w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        for (int w = 0; w < width; w++)
                            float_node_output->data[n++] = (float)((int16_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                             (h * width_aligned) +
                                                                                                             w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            float_node_output->data[n++] = (float)((int16_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                             (c * width_aligned) +
                                                                                                             w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            float_node_output->data[n++] = (float)((int16_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                             (h * width_aligned) +
                                                                                                             w] / quantization_factor;
                    }
                }
                break;
            default:
                for (int i = 0; i < height * channel; i++)
                {
                    for (int j = 0; j < width; j++)
                        float_node_output->data[n++] = (float)((int16_t *)(raw_fixed_node_output->data))[i * width_aligned + j] / quantization_factor;
                }
                break;
            }
        }
        else if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B == data_layout)
        {
            /* 8-bit fixed-point output */
            channel_block_idx   = 0;
            channel_offset_idx  = 0;
            channel_block_size  = height * width * KDP_CHANNEL_MIN_16;

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                /* KL520 not support 1W16C8B ouput NPU data layout format */
                printf("Invalid NPU data layout of HCW to CHW/HWC channel order conversion, NPU data layout = KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B.\n");
                goto FUNC_OUT_ERROR;
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        channel_block_idx = c / KDP_CHANNEL_MIN_16;
                        channel_offset_idx = c % KDP_CHANNEL_MIN_16;
                        for (int w = 0; w < width; w++)
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                            (h * width * KDP_CHANNEL_MIN_16) +
                                                                                                            (w * KDP_CHANNEL_MIN_16) +
                                                                                                            (channel_offset_idx)] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                        {
                            channel_block_idx = c / KDP_CHANNEL_MIN_16;
                            channel_offset_idx = c % KDP_CHANNEL_MIN_16;
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                            (h * width * KDP_CHANNEL_MIN_16) +
                                                                                                            (w * KDP_CHANNEL_MIN_16) +
                                                                                                            (channel_offset_idx)] / quantization_factor;
                        }
                    }
                }
                break;
            default:
                for (int c = 0; c < channel; c++)
                {
                    channel_block_idx = c / KDP_CHANNEL_MIN_16;
                    channel_offset_idx = c % KDP_CHANNEL_MIN_16;
                    for (int i = 0; i < height * width; i++)
                        float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(channel_block_idx * channel_block_size) +
                                                                                                        (i * KDP_CHANNEL_MIN_16) +
                                                                                                        (channel_offset_idx)] / quantization_factor;
                }
                break;
            }
        }
        else
        {
            /* standard 8-bit floating-point output */
            width_aligned = round_up(width, KDP_COL_MIN_16);

            switch (channel_ordering_convert_code)
            {
            case KP_CHANNEL_ORDERING_CVT_HCW2CHW:
                for (int c = 0; c < channel; c++)
                {
                    for (int h = 0; h < height; h++)
                    {
                        for (int w = 0; w < width; w++)
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                            (c * width_aligned) +
                                                                                                            w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HCW:
                for (int h = 0; h < height; h++)
                {
                    for (int c = 0; c < channel; c++)
                    {
                        for (int w = 0; w < width; w++)
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                            (h * width_aligned) +
                                                                                                            w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_HCW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(h * channel * width_aligned) +
                                                                                                            (c * width_aligned) +
                                                                                                            w] / quantization_factor;
                    }
                }
                break;
            case KP_CHANNEL_ORDERING_CVT_CHW2HWC:
                for (int h = 0; h < height; h++)
                {
                    for (int w = 0; w < width; w++)
                    {
                        for (int c = 0; c < channel; c++)
                            float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[(c * height * width_aligned) +
                                                                                                            (h * width_aligned) +
                                                                                                            w] / quantization_factor;
                    }
                }
                break;
            default:
                for (int i = 0; i < height * channel; i++)
                {
                    for (int j = 0; j < width; j++)
                        float_node_output->data[n++] = (float)((int8_t *)(raw_fixed_node_output->data))[i * width_aligned + j] / quantization_factor;
                }
                break;
            }
        }
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == shape_version) {
        if (KP_CHANNEL_ORDERING_CVT_NONE != channel_ordering_convert_code) {
            err_print("Device 0x%X only support ordering 'KP_CHANNEL_ORDERING_DEFAULT'\n", product_id);
            goto FUNC_OUT_ERROR;
        } else {
            /* get channel-wise quantization stride */
            {
                if (is_channel_wise_quantization) {
                    quantized_axis_stride = 1;
                    for (int axis = 0; axis < tensor_descriptor->tensor_shape_info.tensor_shape_info_data.v2.shape_len; axis++) {
                        if (axis != quantization_parameters_v1->quantized_axis) {
                            quantized_axis_stride *= tensor_descriptor->tensor_shape_info.tensor_shape_info_data.v2.shape[axis];
                        }
                    }
                }
            }

            /* convert NPU formatted data to ONNX sequential data */
            {
                onnx_data_shape_index = calloc(tensor_shape_info_v2->shape_len, sizeof(int32_t));

                if (NULL == onnx_data_shape_index) {
                    printf("error: malloc working buffer onnx_data_shape_index fail ...\n");
                    goto FUNC_OUT_ERROR;
                }

                switch (data_layout)
                {
                case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B_CH_COMPACT:
                case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B == data_layout) {
                            for (int axis = 0; axis < (int)tensor_shape_info_v2->shape_len; axis++) {
                                if (1 == tensor_shape_info_v2->stride_npu[axis]) {
                                    channel_idx = axis;
                                    continue;
                                }

                                npu_channel_group_stride_tmp = tensor_shape_info_v2->stride_npu[axis] * tensor_shape_info_v2->shape[axis];
                                if (npu_channel_group_stride_tmp > npu_channel_group_stride)
                                    npu_channel_group_stride = npu_channel_group_stride_tmp;
                            }

                            npu_channel_group_stride -= 16;

                            /* npu_data_buf_offset += (onnx_data_shape_index[channel_idx] / 16) * npu_channel_group_stride; */
                            npu_data_buf_offset += (onnx_data_shape_index[channel_idx] >> 4) * npu_channel_group_stride;
                        }

                        if (KP_SUCCESS != get_quantization_parameters_factor(quantization_parameters_v1,
                                                                             is_channel_wise_quantization,
                                                                             onnx_data_buf_offset,
                                                                             quantized_axis_stride,
                                                                             &quantized_fixed_point_descriptor_idx,
                                                                             &quantization_factor)) {
                            printf("error: get quantization parameters factor fail ...\n");
                            goto FUNC_OUT_ERROR;
                        }

                        float_node_output->data[onnx_data_buf_offset] = ((int8_t *)raw_fixed_node_output->data)[npu_data_buf_offset] / quantization_factor;

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                case KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B:
                case KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        if (KP_SUCCESS != get_quantization_parameters_factor(quantization_parameters_v1,
                                                                             is_channel_wise_quantization,
                                                                             onnx_data_buf_offset,
                                                                             quantized_axis_stride,
                                                                             &quantized_fixed_point_descriptor_idx,
                                                                             &quantization_factor)) {
                            printf("error: get quantization parameters factor fail ...\n");
                            goto FUNC_OUT_ERROR;
                        }

                        npu_data_element_16b                            = ((uint16_t *)raw_fixed_node_output->data)[npu_data_buf_offset];
                        float_node_output->data[onnx_data_buf_offset]   = (float)((int16_t)(npu_data_element_16b & 0xfffeu)) / quantization_factor;

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                case KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL:
                case KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL_CH_COMPACT:
                case KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL:
                    while (true) {
                        npu_data_buf_offset = 0;
                        for (int32_t axis = 0; axis < tensor_shape_info_v2->shape_len; axis++) {
                            npu_data_buf_offset += onnx_data_shape_index[axis] * tensor_shape_info_v2->stride_npu[axis];
                        }

                        if (KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL == data_layout) {
                            for (int axis = 0; axis < (int)tensor_shape_info_v2->shape_len; axis++) {
                                if (1 == tensor_shape_info_v2->stride_npu[axis]) {
                                    channel_idx = axis;
                                    continue;
                                }

                                npu_channel_group_stride_tmp = tensor_shape_info_v2->stride_npu[axis] * tensor_shape_info_v2->shape[axis];
                                if (npu_channel_group_stride_tmp > npu_channel_group_stride)
                                    npu_channel_group_stride = npu_channel_group_stride_tmp;
                            }

                            npu_channel_group_stride -= 16;

                            /* npu_data_buf_offset += (onnx_data_shape_index[channel_idx] / 16) * npu_channel_group_stride; */
                            npu_data_buf_offset += (onnx_data_shape_index[channel_idx] >> 4) * npu_channel_group_stride;
                        }

                        /* npu_data_buf_offset = (npu_data_buf_offset / 16) * 32 + (npu_data_buf_offset % 16) */
                        npu_data_buf_offset = ((npu_data_buf_offset >> 4) << 5) + (npu_data_buf_offset & 15u);

                        if (KP_SUCCESS != get_quantization_parameters_factor(quantization_parameters_v1,
                                                                             is_channel_wise_quantization,
                                                                             onnx_data_buf_offset,
                                                                             quantized_axis_stride,
                                                                             &quantized_fixed_point_descriptor_idx,
                                                                             &quantization_factor)) {
                            printf("error: get quantization parameters factor fail ...\n");
                            goto FUNC_OUT_ERROR;
                        }

                        float_node_output->data[onnx_data_buf_offset] = (float)((int16_t)(((((uint16_t)(((uint8_t *)raw_fixed_node_output->data)[npu_data_buf_offset])) & 0x007fu) +
                                                                                           (((uint16_t)(((uint8_t *)raw_fixed_node_output->data)[npu_data_buf_offset + npu_data_high_bit_offset])) << 7)) << 1)) / quantization_factor;

                        for (int32_t axis = tensor_shape_info_v2->shape_len - 1; axis >= 0; axis--) {
                            onnx_data_shape_index[axis]++;
                            if (onnx_data_shape_index[axis] == tensor_shape_info_v2->shape[axis]) {
                                if (axis == 0)
                                    break;

                                onnx_data_shape_index[axis] = 0;
                                continue;
                            } else {
                                break;
                            }
                        }

                        if (onnx_data_shape_index[0] == tensor_shape_info_v2->shape[0])
                            break;

                        onnx_data_buf_offset++;
                    }
                    break;
                default:
                    printf("error: get invalide data layout ...\n");
                    goto FUNC_OUT_ERROR;
                }
            }
        }
    }

    kp_release_raw_fixed_node_output(raw_fixed_node_output);

    if (NULL != onnx_data_shape_index)
        free(onnx_data_shape_index);

    return float_node_output;

FUNC_OUT_ERROR:
    kp_release_float_node_output(float_node_output);
    kp_release_raw_fixed_node_output(raw_fixed_node_output);
    float_node_output = NULL;

    if (NULL != onnx_data_shape_index)
        free(onnx_data_shape_index);

    return float_node_output;
}

int kp_customized_inference_send(kp_device_group_t devices, void *header, int header_size, uint8_t *image, int image_size)
{
    int ret;

    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_send++];

    if (_devices_grp->cur_send >= _devices_grp->num_device)
        _devices_grp->cur_send = 0;

    // help user to set up header stamp
    kp_inference_header_stamp_t *header_stamp = (kp_inference_header_stamp_t *)header;

    if (KP_MAX_INPUT_NODE_COUNT < header_stamp->total_image) {
        return KP_ERROR_FIFOQ_INPUT_BUFF_COUNT_NOT_ENOUGH_42;
    }

    if (header_stamp->image_index >= header_stamp->total_image) {
        return KP_ERROR_INVALID_PARAM_12;
    }

    header_stamp->magic_type = KDP2_MAGIC_TYPE_INFERENCE;
    header_stamp->total_size = header_size + image_size;

    if (header_stamp->total_size > _devices_grp->ddr_attr.input_buffer_size)
        return KP_ERROR_SEND_DATA_TOO_LARGE_15;

    ret = kp_usb_write_data(ll_dev, header, header_size, _devices_grp->timeout);
    int status = check_inf_desc_error(ret);
    if (status != KP_SUCCESS)
        return status;

    if (image) // sometimes image buffer could be null
    {
        ret = kp_usb_write_data(ll_dev, image, image_size, _devices_grp->timeout);
        status = check_send_image_error(ret);
        if (status != KP_SUCCESS)
            return status;
    }

    return KP_SUCCESS;
}

int kp_customized_inference_receive(kp_device_group_t devices, void *result_buffer, int buf_size, int *recv_size)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_recv++];

    if (_devices_grp->cur_recv >= _devices_grp->num_device)
        _devices_grp->cur_recv = 0;

    // if return < 0 means libusb error, otherwise return  received size
    int usb_ret = kp_usb_read_data(ll_dev, result_buffer, buf_size, _devices_grp->timeout);
    if (usb_ret < 0)
        return usb_ret;

    *recv_size = usb_ret;

    // verify result buffer
    int status = verify_result_header_stamp((kp_inference_header_stamp_t *)result_buffer, 0, 0);
    if (status != KP_SUCCESS)
        return status;

    return KP_SUCCESS;
}

int kp_customized_command_noack_send(kp_device_group_t devices, void *cmd, int cmd_size)
{
    int ret;

    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_send++];

    if (_devices_grp->cur_send >= _devices_grp->num_device)
        _devices_grp->cur_send = 0;

    // help user to set up header stamp
    kp_inference_header_stamp_t *header_stamp = (kp_inference_header_stamp_t *)cmd;
    header_stamp->magic_type = KDP2_MAGIC_TYPE_CUSTOMIZED;
    header_stamp->total_size = cmd_size;

    if (cmd_size > _devices_grp->ddr_attr.input_buffer_size)
        return KP_ERROR_SEND_DATA_TOO_LARGE_15;

    ret = kp_usb_write_data(ll_dev, cmd, cmd_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    return KP_SUCCESS;
}

int kp_customized_command_send(kp_device_group_t devices, void *cmd, int cmd_size, void *return_buf, int return_buf_size)
{
    int ret;

    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_send++];

    if (_devices_grp->cur_send >= _devices_grp->num_device)
        _devices_grp->cur_send = 0;

    // help user to set up header stamp
    kp_inference_header_stamp_t *header_stamp = (kp_inference_header_stamp_t *)cmd;
    header_stamp->magic_type = KDP2_MAGIC_TYPE_CUSTOMIZED;
    header_stamp->total_size = cmd_size;

    if (header_stamp->total_size > _devices_grp->ddr_attr.input_buffer_size)
        return KP_ERROR_SEND_DATA_TOO_LARGE_15;

    ret = kp_usb_write_data(ll_dev, cmd, cmd_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    ret = kp_usb_read_data(ll_dev, return_buf, return_buf_size, _devices_grp->timeout);
    if (ret < 0)
        return ret;

    header_stamp = (kp_inference_header_stamp_t *)return_buf;

    if (header_stamp->magic_type != KDP2_MAGIC_TYPE_CUSTOMIZED)
    {
        dbg_print("%s, magic_type = 0x%x \n ",__func__, header_stamp->magic_type);
        dbg_print("%s, total_size = 0x%x \n ",__func__, header_stamp->total_size);
        dbg_print("%s, job_id = 0x%x \n ",__func__, header_stamp->job_id);
        dbg_print("%s, status_code = 0x%x \n ",__func__, header_stamp->status_code);

        return KP_ERROR_RECEIVE_INCORRECT_HEADER_STAMP_30;
    }

    return KP_SUCCESS;
}

int kp_dbg_set_enable_checkpoints(kp_device_group_t devices, uint32_t checkpoint_flags, bool enable)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kdp2_ipc_cmd_set_dbg_checkpoint_t cmd;
    cmd.magic_type = KDP2_MAGIC_TYPE_COMMAND;
    cmd.total_size = sizeof(kdp2_ipc_cmd_set_dbg_checkpoint_t);
    cmd.command_id = KDP2_COMMAND_SET_DBG_CHECKPOINT;
    cmd.checkpoint_flags = checkpoint_flags;
    cmd.enable = enable;

    for (int i = 0; i < _devices_grp->num_device; i++)
    {
        kp_usb_device_t *ll_dev = _devices_grp->ll_device[i];
        int ret = kp_usb_write_data(ll_dev, (void *)&cmd, cmd.total_size, _devices_grp->timeout);
        if (ret != KP_SUCCESS)
            return ret;

        int return_code;
        ret = kp_usb_read_data(ll_dev, (void *)&return_code, sizeof(uint32_t), _devices_grp->timeout);
        if (ret < 0)
            return ret;
        else if (return_code != KP_SUCCESS)
            return return_code;
    }

    return KP_SUCCESS;
}

int kp_dbg_receive_checkpoint_data(kp_device_group_t devices, void **checkpoint_buf)
{
    static void *dbg_buf = NULL;
    int dbg_buf_size = 8 * 1024 * 1024;
    if (dbg_buf == NULL)
    {
        dbg_buf = malloc(dbg_buf_size);
        if (dbg_buf == NULL)
            return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;
    kp_usb_device_t *ll_dev = _devices_grp->ll_device[_devices_grp->cur_recv++];

    if (_devices_grp->cur_recv >= _devices_grp->num_device)
        _devices_grp->cur_recv = 0;

    // if return < 0 means libusb error, otherwise return received size
    int usb_ret = kp_usb_read_data(ll_dev, dbg_buf, dbg_buf_size, _devices_grp->timeout);
    if (usb_ret < 0)
        return usb_ret;

    kp_inference_header_stamp_t *hdr = (kp_inference_header_stamp_t *)dbg_buf;
    if ((KDP2_MAGIC_TYPE_CHECKPOINT_DATA != hdr->magic_type) &&
        (KDP2_MAGIC_TYPE_CHECKPOINT_DATA_V2 != hdr->magic_type))
        return KP_ERROR_INVALID_CHECKPOINT_DATA_36;

    if (usb_ret == sizeof(kp_inference_header_stamp_t))
        return KP_DBG_CHECKPOINT_END_37;

    // cast data layout to kp_model_tensor_data_layout_t
    uint32_t checkpoint_tag = ((dbg_ipc_checkpoint_data_after_inference_t *)dbg_buf)->checkpoint_tag;

    if (KP_DBG_CHECKPOINT_BEFORE_PREPROCESS == checkpoint_tag)
    {
        *checkpoint_buf = calloc(1, hdr->total_size);
        memcpy(*checkpoint_buf, dbg_buf, hdr->total_size);
    }
    else if (KP_DBG_CHECKPOINT_AFTER_PREPROCESS == checkpoint_tag)
    {
        *checkpoint_buf = calloc(1, hdr->total_size);
        memcpy(*checkpoint_buf, dbg_buf, hdr->total_size);
    }
    else if ((KP_DBG_CHECKPOINT_AFTER_INFERENCE == checkpoint_tag) ||
             (KP_DBG_CHECKPOINT_BEFORE_CPU_OP == checkpoint_tag) ||
             (KP_DBG_CHECKPOINT_AFTER_CPU_OP == checkpoint_tag))
    {
        if (KDP2_MAGIC_TYPE_CHECKPOINT_DATA == hdr->magic_type) {
            dbg_ipc_checkpoint_data_after_inference_t *dbg_ipc_data = (dbg_ipc_checkpoint_data_after_inference_t *)hdr;
            kp_dbg_checkpoint_data_after_inference_t *dbg_data = (kp_dbg_checkpoint_data_after_inference_t *)calloc(1, sizeof(kp_dbg_checkpoint_data_after_inference_t));
            kp_tensor_descriptor_t *tensor_desc = (kp_tensor_descriptor_t *)calloc(dbg_ipc_data->num_nodes, sizeof(kp_tensor_descriptor_t));
            uint8_t *raw_output = (uint8_t *)calloc(dbg_ipc_data->total_output_size, sizeof(uint8_t));
            *checkpoint_buf = dbg_data;

            dbg_data->checkpoint_tag = dbg_ipc_data->checkpoint_tag;
            dbg_data->target_inf_model = dbg_ipc_data->target_inf_model;
            dbg_data->num_nodes = dbg_ipc_data->num_nodes;
            dbg_data->node_metadata = tensor_desc;
            dbg_data->total_output_size = dbg_ipc_data->total_output_size;
            dbg_data->raw_output = raw_output;

            memcpy(&dbg_data->header_stamp, &dbg_ipc_data->header_stamp, sizeof(kp_inference_header_stamp_t));

            for (uint32_t i = 0; i < dbg_ipc_data->num_nodes; i++) {
                tensor_desc[i].data_layout = convert_data_format_to_kp_tensor_format(dbg_ipc_data->node_metadata[i].data_layout, _devices_grp->loaded_model_desc.target);
                tensor_desc[i].index = i;
                tensor_desc[i].name = NULL;
                {
                    kp_tensor_shape_info_v1_t *shape_info = &tensor_desc[i].tensor_shape_info.tensor_shape_info_data.v1;

                    tensor_desc[i].tensor_shape_info.version = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;
                    shape_info->shape_npu_len = 4;
                    shape_info->shape_onnx_len = 4;
                    shape_info->axis_permutation_len = 4;
                    shape_info->shape_npu = realloc_tensor_shape_int32_t(shape_info->shape_npu, shape_info->shape_npu_len);
                    shape_info->shape_onnx = realloc_tensor_shape_int32_t(shape_info->shape_onnx, shape_info->shape_onnx_len);
                    shape_info->axis_permutation_onnx_to_npu = realloc_tensor_shape_int32_t(shape_info->axis_permutation_onnx_to_npu, shape_info->axis_permutation_len);

                    shape_info->shape_npu[0] = 1;
                    shape_info->shape_npu[1] = dbg_ipc_data->node_metadata[i].channel;
                    shape_info->shape_npu[2] = dbg_ipc_data->node_metadata[i].height;
                    shape_info->shape_npu[3] = dbg_ipc_data->node_metadata[i].width;

                    memcpy(shape_info->shape_onnx, shape_info->shape_npu, shape_info->shape_npu_len * sizeof(int32_t));

                    for (int axis_permutation_idx = 0; axis_permutation_idx < shape_info->axis_permutation_len; axis_permutation_idx++) {
                        shape_info->axis_permutation_onnx_to_npu[axis_permutation_idx] = axis_permutation_idx;
                    }
                }
                {
                    kp_quantization_parameters_v1_t *quant_param = &tensor_desc[i].quantization_parameters.quantization_parameters_data.v1;
                    tensor_desc[i].quantization_parameters.version = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;
                    quant_param->quantized_axis = 1;
                    quant_param->quantized_fixed_point_descriptor_num = 1;
                    quant_param->quantized_fixed_point_descriptor = realloc_quantized_fixed_point_descriptor_list(quant_param->quantized_fixed_point_descriptor,
                                                                                                                  quant_param->quantized_fixed_point_descriptor_num);
                    quant_param->quantized_fixed_point_descriptor[0].radix = dbg_ipc_data->node_metadata[i].radix;
                    quant_param->quantized_fixed_point_descriptor[0].scale_dtype = KP_DTYPE_FLOAT32;
                    quant_param->quantized_fixed_point_descriptor[0].scale.scale_float32 = dbg_ipc_data->node_metadata[i].scale;
                }
            }

            memcpy(dbg_data->raw_output, dbg_ipc_data->raw_output, dbg_data->total_output_size);
        } else if (KDP2_MAGIC_TYPE_CHECKPOINT_DATA_V2 == hdr->magic_type) {
            uint32_t dst_raw_output_offset = 0;
            dbg_ipc_checkpoint_data_after_inference_t_v2 *dbg_ipc_data = (dbg_ipc_checkpoint_data_after_inference_t_v2 *)hdr;
            kp_dbg_checkpoint_data_after_inference_t *dbg_data = (kp_dbg_checkpoint_data_after_inference_t *)calloc(1, sizeof(kp_dbg_checkpoint_data_after_inference_t) + dbg_ipc_data->total_output_size);
            kp_tensor_descriptor_t *tensor_desc = (kp_tensor_descriptor_t *)calloc(dbg_ipc_data->num_nodes, sizeof(kp_tensor_descriptor_t));
            uint8_t *raw_output = (uint8_t *)calloc(dbg_ipc_data->total_output_size, sizeof(uint8_t));
            *checkpoint_buf = dbg_data;

            dbg_data->checkpoint_tag = dbg_ipc_data->checkpoint_tag;
            dbg_data->target_inf_model = dbg_ipc_data->target_inf_model;
            dbg_data->num_nodes = dbg_ipc_data->num_nodes;
            dbg_data->node_metadata = tensor_desc;
            dbg_data->total_output_size = dbg_ipc_data->total_output_size;
            dbg_data->raw_output = raw_output;

            memcpy(&dbg_data->header_stamp, &dbg_ipc_data->header_stamp, sizeof(kp_inference_header_stamp_t));

            for (uint32_t i = 0; i < dbg_ipc_data->num_nodes; i++) {
                npu_data_single_node_header_v2_t *ipc_node_header = (npu_data_single_node_header_v2_t *)(dbg_ipc_data->data + (i * sizeof(npu_data_single_node_header_v2_t)));
                tensor_desc[i].data_layout = convert_data_format_to_kp_tensor_format(ipc_node_header->data_layout, _devices_grp->loaded_model_desc.target);
                tensor_desc[i].index = i;
                tensor_desc[i].name = (char *)calloc(ipc_node_header->name_len + 1, sizeof(char));
                memcpy(tensor_desc[i].name, (dbg_ipc_data->data + ipc_node_header->name_start_offset), ipc_node_header->name_len * sizeof(char));
                {
                    kp_tensor_shape_info_v2_t *shape_info = &tensor_desc[i].tensor_shape_info.tensor_shape_info_data.v2;

                    tensor_desc[i].tensor_shape_info.version = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2;
                    shape_info->shape_len = ipc_node_header->shape_len;
                    shape_info->shape = realloc_tensor_shape_int32_t(shape_info->shape, shape_info->shape_len);
                    shape_info->stride_npu = realloc_tensor_shape_uint32_t(shape_info->stride_npu, shape_info->shape_len);
                    shape_info->stride_onnx = realloc_tensor_shape_uint32_t(shape_info->stride_onnx, shape_info->shape_len);
                    memcpy(shape_info->shape, (dbg_ipc_data->data + ipc_node_header->shape_start_offset), ipc_node_header->shape_len * sizeof(int32_t));
                    memcpy(shape_info->stride_npu, (dbg_ipc_data->data + ipc_node_header->stride_npu_start_offset), ipc_node_header->shape_len * sizeof(uint32_t));
                    memcpy(shape_info->stride_onnx, (dbg_ipc_data->data + ipc_node_header->stride_onnx_start_offset), ipc_node_header->shape_len * sizeof(uint32_t));
                }
                {
                    kp_quantization_parameters_v1_t *quant_param = &tensor_desc[i].quantization_parameters.quantization_parameters_data.v1;

                    tensor_desc[i].quantization_parameters.version = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;
                    quant_param->quantized_axis = ipc_node_header->quantized_axis;
                    quant_param->quantized_fixed_point_descriptor_num = ipc_node_header->quantized_parameters_len;
                    quant_param->quantized_fixed_point_descriptor = realloc_quantized_fixed_point_descriptor_list(quant_param->quantized_fixed_point_descriptor,
                                                                                                                  quant_param->quantized_fixed_point_descriptor_num);

                    int32_t *radix_p = (int32_t *)(dbg_ipc_data->data + ipc_node_header->radix_start_offset);
                    void *scale_p = (void *)(dbg_ipc_data->data + ipc_node_header->scale_start_offset);

                    fill_quantized_fix_point_descripter(quant_param->quantized_fixed_point_descriptor, ipc_node_header->quantized_parameters_len,
                                                        ipc_node_header->scale_data_type, ipc_node_header->radix_data_type, scale_p, radix_p);
                }

                uint8_t *src_raw_output = (dbg_ipc_data->data + ipc_node_header->npu_data_start_offset);
                uint8_t *dst_raw_output = (dbg_data->raw_output + dst_raw_output_offset);
                memcpy(dst_raw_output, src_raw_output, ipc_node_header->npu_data_len);

                dst_raw_output_offset += ipc_node_header->npu_data_len;
            }
        }
    }

    return KP_SUCCESS;
}

int kp_profile_set_enable(kp_device_group_t devices, bool enable)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[0]; // FIXME

    kdp2_ipc_cmd_set_profile_enable_t cmd_buf;
    cmd_buf.magic_type = KDP2_MAGIC_TYPE_COMMAND;
    cmd_buf.total_size = sizeof(kdp2_ipc_cmd_set_profile_enable_t);
    cmd_buf.command_id = KDP2_COMMAND_SET_PROFILE_ENABLE;
    cmd_buf.enable = enable;

    int ret = kp_usb_write_data(ll_dev, (void *)&cmd_buf, cmd_buf.total_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    int return_code;
    ret = kp_usb_read_data(ll_dev, (void *)&return_code, sizeof(uint32_t), _devices_grp->timeout);
    if (ret < 0)
        return ret;
    else if (return_code != KP_SUCCESS)
        return return_code;

    return KP_SUCCESS;
}

int kp_profile_get_statistics(kp_device_group_t devices, kp_profile_data_t *profile_data)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[0]; // FIXME

    kdp2_ipc_cmd_get_profile_statics_t cmd_buf;
    cmd_buf.magic_type = KDP2_MAGIC_TYPE_COMMAND;
    cmd_buf.total_size = sizeof(kdp2_ipc_cmd_get_profile_statics_t);
    cmd_buf.command_id = KDP2_COMMAND_GET_PROFILE_STATISTICS;

    int ret = kp_usb_write_data(ll_dev, (void *)&cmd_buf, cmd_buf.total_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    ret = kp_usb_read_data(ll_dev, (void *)profile_data, sizeof(kp_profile_data_t), _devices_grp->timeout);
    if (ret < 0)
        return ret;

    return KP_SUCCESS;
}

int kp_performance_monitor_set_enable(kp_device_group_t devices, bool enable)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[0]; // FIXME

    kdp2_ipc_cmd_set_performance_monitor_enable_t cmd_buf;
    cmd_buf.magic_type = KDP2_MAGIC_TYPE_COMMAND;
    cmd_buf.total_size = sizeof(kdp2_ipc_cmd_set_performance_monitor_enable_t);
    cmd_buf.command_id = KDP2_COMMAND_SET_PERFORMANCE_MONITOR_ENABLE;
    cmd_buf.enable = enable;

    int ret = kp_usb_write_data(ll_dev, (void *)&cmd_buf, cmd_buf.total_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    int return_code;
    ret = kp_usb_read_data(ll_dev, (void *)&return_code, sizeof(uint32_t), _devices_grp->timeout);
    if (ret < 0)
        return ret;
    else if (return_code != KP_SUCCESS)
        return return_code;

    return KP_SUCCESS;
}

int kp_performance_monitor_get_statistics(kp_device_group_t devices, kp_performance_monitor_data_t *performance_monitor_data)
{
    _kp_devices_group_t *_devices_grp = (_kp_devices_group_t *)devices;

    kp_usb_device_t *ll_dev = _devices_grp->ll_device[0]; // FIXME

    kdp2_ipc_cmd_get_performance_monitor_statics_t cmd_buf;
    cmd_buf.magic_type = KDP2_MAGIC_TYPE_COMMAND;
    cmd_buf.total_size = sizeof(kdp2_ipc_cmd_get_performance_monitor_statics_t);
    cmd_buf.command_id = KDP2_COMMAND_GET_PERFORMANCE_MONITOR_STATISTICS;

    int ret = kp_usb_write_data(ll_dev, (void *)&cmd_buf, cmd_buf.total_size, _devices_grp->timeout);
    if (ret != KP_SUCCESS)
        return ret;

    ret = kp_usb_read_data(ll_dev, (void *)performance_monitor_data, sizeof(kp_performance_monitor_data_t), _devices_grp->timeout);
    if (ret < 0)
        return ret;

    return KP_SUCCESS;
}

void kp_release_raw_fixed_node_output(kp_inf_raw_fixed_node_output_t *raw_fixed_node_output)
{
    if (NULL == raw_fixed_node_output)
        return;

    kp_tensor_descriptor_t *tensor_descriptor                   = &(raw_fixed_node_output->metadata.tensor_descriptor);
    kp_tensor_shape_info_t *tensor_shape_info                   = &(tensor_descriptor->tensor_shape_info);
    kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = &(tensor_shape_info->tensor_shape_info_data.v1);
    kp_tensor_shape_info_v2_t *tensor_shape_info_v2             = &(tensor_shape_info->tensor_shape_info_data.v2);
    kp_quantization_parameters_t *quantization_parameters       = &(raw_fixed_node_output->metadata.tensor_descriptor.quantization_parameters);
    kp_quantization_parameters_v1_t *quantization_parameters_v1 = &(quantization_parameters->quantization_parameters_data.v1);

    if (NULL != tensor_descriptor->name)
        free(tensor_descriptor->name);

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == tensor_shape_info->version) {
        if (NULL != tensor_shape_info_v1->shape_npu)
            free(tensor_shape_info_v1->shape_npu);

        if (NULL != tensor_shape_info_v1->shape_onnx)
            free(tensor_shape_info_v1->shape_onnx);

        if (NULL != tensor_shape_info_v1->axis_permutation_onnx_to_npu)
            free(tensor_shape_info_v1->axis_permutation_onnx_to_npu);
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == tensor_shape_info->version) {
        if (NULL != tensor_shape_info_v2->shape)
            free(tensor_shape_info_v2->shape);

        if (NULL != tensor_shape_info_v2->stride_npu)
            free(tensor_shape_info_v2->stride_npu);

        if (NULL != tensor_shape_info_v2->stride_onnx)
            free(tensor_shape_info_v2->stride_onnx);
    } else {
        printf("%s, invalid tensor shape version.\n", __func__);
        return;
    }

    if (KP_MODEL_QUANTIZATION_PARAMS_VERSION_1 == quantization_parameters->version) {
        if (NULL != quantization_parameters_v1->quantized_fixed_point_descriptor)
            free(quantization_parameters_v1->quantized_fixed_point_descriptor);
    } else {
        printf("%s, invalid quantization parameters version.\n", __func__);
        return;
    }

    free(raw_fixed_node_output);
}

void kp_release_fixed_node_output(kp_inf_fixed_node_output_t *fixed_node_output)
{
    if (NULL == fixed_node_output)
        return;

    if (NULL != fixed_node_output->name)
        free(fixed_node_output->name);

    if (NULL != fixed_node_output->shape)
        free(fixed_node_output->shape);

    kp_quantization_parameters_t *quantization_parameters       = &(fixed_node_output->quantization_parameters);
    kp_quantization_parameters_v1_t *quantization_parameters_v1 = &(quantization_parameters->quantization_parameters_data.v1);

    if (KP_MODEL_QUANTIZATION_PARAMS_VERSION_1 == quantization_parameters->version) {
        if (NULL != quantization_parameters_v1->quantized_fixed_point_descriptor)
            free(quantization_parameters_v1->quantized_fixed_point_descriptor);
    } else {
        printf("%s, invalid quantization parameters version.\n", __func__);
        return;
    }

    free(fixed_node_output);
}

void kp_release_float_node_output(kp_inf_float_node_output_t *float_node_output)
{
    if (NULL == float_node_output)
        return;

    if (NULL != float_node_output->name)
        free(float_node_output->name);

    if (NULL != float_node_output->shape)
        free(float_node_output->shape);

    free(float_node_output);
}

int kp_release_dbg_checkpoint_data(void *checkpoint_buf)
{
    kp_dbg_checkpoint_data_after_inference_t *after_inf = (kp_dbg_checkpoint_data_after_inference_t *)checkpoint_buf;

    if ((KDP2_MAGIC_TYPE_CHECKPOINT_DATA != after_inf->header_stamp.magic_type) &&
        (KDP2_MAGIC_TYPE_CHECKPOINT_DATA_V2 != after_inf->header_stamp.magic_type))
        return KP_ERROR_INVALID_CHECKPOINT_DATA_36;

    if ((KP_DBG_CHECKPOINT_BEFORE_PREPROCESS == after_inf->checkpoint_tag) ||
        (KP_DBG_CHECKPOINT_AFTER_PREPROCESS == after_inf->checkpoint_tag)) {
        free(checkpoint_buf);
    } else if ((KP_DBG_CHECKPOINT_AFTER_INFERENCE == after_inf->checkpoint_tag) ||
               (KP_DBG_CHECKPOINT_BEFORE_CPU_OP == after_inf->checkpoint_tag) ||
               (KP_DBG_CHECKPOINT_AFTER_CPU_OP == after_inf->checkpoint_tag)) {
        if (NULL != after_inf->node_metadata) {
            for (int node_idx = 0; node_idx < after_inf->num_nodes; node_idx++) {
                if (KP_SUCCESS != deconstruct_tensor_descriptor(&(after_inf->node_metadata[node_idx]))) {
                    return KP_ERROR_MEMORY_FREE_FAILURE_39;
                }
            }
            free(after_inf->node_metadata);
        }

        free(after_inf->raw_output);
        free(after_inf);
    }

    return KP_SUCCESS;
}
