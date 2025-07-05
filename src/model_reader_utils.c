#include <stdio.h>
#include "internal_func.h"
#include "kp_struct.h"

#define err_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }

/******************************************************************
 * utils
 ******************************************************************/

int is_tensor_info_reallocted(kp_tensor_descriptor_t* tensor) {
    if (NULL == tensor->name) {
        err_print("construct nef info in node tensor info fail: realloc memory fail ...\n");
        return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    if (KP_MODEL_QUANTIZATION_PARAMS_VERSION_1 == tensor->quantization_parameters.version) {
        kp_quantization_parameters_v1_t *quantization_parameters_v1 = &(tensor->quantization_parameters.quantization_parameters_data.v1);
        if ((0 < quantization_parameters_v1->quantized_fixed_point_descriptor_num) &&
            (NULL == quantization_parameters_v1->quantized_fixed_point_descriptor)) {
            err_print("construct nef info in node tensor info fail: realloc memory fail ...\n");
            return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        }
    } else {
        err_print("construct nef info in node tensor info fail: invalid source quantization parameters version ...\n");
        return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == tensor->tensor_shape_info.version) {
        kp_tensor_shape_info_v1_t *tensor_shape_info_v1 = &(tensor->tensor_shape_info.tensor_shape_info_data.v1);
        if (((0 < tensor_shape_info_v1->axis_permutation_len) && (NULL == tensor_shape_info_v1->axis_permutation_onnx_to_npu)) ||
            ((0 < tensor_shape_info_v1->shape_npu_len)        && (NULL == tensor_shape_info_v1->shape_npu)) ||
            ((0 < tensor_shape_info_v1->shape_onnx_len)       && (NULL == tensor_shape_info_v1->shape_onnx))) {
            err_print("construct nef info in node tensor info fail: realloc memory fail ...\n");
            return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        }
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == tensor->tensor_shape_info.version) {
        kp_tensor_shape_info_v2_t *tensor_shape_info_v2 = &(tensor->tensor_shape_info.tensor_shape_info_data.v2);
        if (0 < tensor_shape_info_v2->shape_len && (NULL == tensor_shape_info_v2->shape ||
                                                    NULL == tensor_shape_info_v2->stride_npu ||
                                                    NULL == tensor_shape_info_v2->stride_onnx)) {
            err_print("construct nef info in node tensor info fail: realloc memory fail ...\n");
            return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        }
    } else {
        err_print("construct nef info in node tensor info fail: invalid source tensor shape version ...\n");
        return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    return KP_SUCCESS;
}

uint32_t convert_data_format_to_kp_tensor_format(uint32_t data_format, uint32_t target_chip) {
    if (KP_MODEL_TARGET_CHIP_KL520 == target_chip) {
        switch (data_format)
        {
        case DATA_FMT_KL520_4W4C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B;
            break;
        case DATA_FMT_KL520_16W1C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B;
            break;
        }
    } else if (KP_MODEL_TARGET_CHIP_KL720 == target_chip) {
        switch (data_format)
        {
        case DATA_FMT_KL720_4W4C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B;
            break;
        case DATA_FMT_KL720_16W1C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B;
            break;
        case DATA_FMT_KL720_1W16C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B;
            break;
        case DATA_FMT_KL720_1W16C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL;
            break;
        case DATA_FMT_KL720_8W1C16B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B;
            break;
        case DATA_FMT_KL720_RAW8:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B;
            break;
        case DATA_FMT_KL720_RAW16:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B;
            break;
        case DATA_FMT_KL720_RAW_FLOAT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_FLOAT;
            break;
        }
    } else if (KP_MODEL_TARGET_CHIP_KL530 == target_chip) {
        switch (data_format)
        {
        case DATA_FMT_KL530_4W4C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B;
            break;
        case DATA_FMT_KL530_4W4C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL;
            break;
        case DATA_FMT_KL530_16W1C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B;
            break;
        case DATA_FMT_KL530_16W1C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL;
            break;
        case DATA_FMT_KL530_1W16C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B;
            break;
        case DATA_FMT_KL530_1W16C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL;
            break;
        case DATA_FMT_KL530_8W1C16B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B;
            break;
        case DATA_FMT_KL530_RAW8:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B;
            break;
        case DATA_FMT_KL530_RAW16:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B;
            break;
        case DATA_FMT_KL530_RAW_FLOAT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_FLOAT;
            break;
        }
    } else if (KP_MODEL_TARGET_CHIP_KL630 == target_chip) {
        switch (data_format)
        {
        case DATA_FMT_KL630_4W4C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B;
            break;
        case DATA_FMT_KL630_4W4C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL;
            break;
        case DATA_FMT_KL630_16W1C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B;
            break;
        case DATA_FMT_KL630_16W1C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL;
            break;
        case DATA_FMT_KL630_1W16C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B;
            break;
        case DATA_FMT_KL630_1W16C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL;
            break;
        case DATA_FMT_KL630_8W1C16B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B;
            break;
        case DATA_FMT_KL630_RAW8:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B;
            break;
        case DATA_FMT_KL630_RAW16:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B;
            break;
        case DATA_FMT_KL630_RAW_FLOAT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_FLOAT;
            break;
        }
    } else if (KP_MODEL_TARGET_CHIP_KL730 == target_chip) {
        switch (data_format)
        {
        case DATA_FMT_KL730_4W4C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8B;
            break;
        case DATA_FMT_KL730_4W4C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_4W4C8BHL;
            break;
        case DATA_FMT_KL730_16W1C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8B;
            break;
        case DATA_FMT_KL730_16W1C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_16W1C8BHL;
            break;
        case DATA_FMT_KL730_1W16C8B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B;
            break;
        case DATA_FMT_KL730_1W16C8BHL:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL;
            break;
        case DATA_FMT_KL730_1W16C8B_CH_COMPACT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8B_CH_COMPACT;
            break;
        case DATA_FMT_KL730_1W16C8BHL_CH_COMPACT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_1W16C8BHL_CH_COMPACT;
            break;
        case DATA_FMT_KL730_8W1C16B:
            return KP_MODEL_TENSOR_DATA_LAYOUT_8W1C16B;
            break;
        case DATA_FMT_KL730_RAW8:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_8B;
            break;
        case DATA_FMT_KL730_RAW16:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_16B;
            break;
        case DATA_FMT_KL730_RAW_FLOAT:
            return KP_MODEL_TENSOR_DATA_LAYOUT_RAW_FLOAT;
            break;
        }
    }

    return KP_MODEL_TENSOR_DATA_LAYOUT_UNKNOWN;
}
