/**
 * @file        model_descriptor_copier.c
 * @brief       NEF model related functions - copy model information from kp_model_nef_descriptor_t
 * @version     0.1
 * @date        2024-01-19
 *
 * @copyright   Copyright (c) 2024 Kneron Inc. All rights reserved.
 */

// #define DEBUG_PRINT

#include "internal_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_PRINT
#define dbg_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }
#else
#define dbg_print(format, ...)
#endif

#define err_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }

/******************************************************************
 * nef info copier utils
 ******************************************************************/

int copy_single_tensor_info_quantization_parameters(kp_quantization_parameters_t *quantization_parameters_dst, kp_quantization_parameters_t *quantization_parameters_src) {
    if (NULL == quantization_parameters_dst ||
        NULL == quantization_parameters_src) {
        err_print("copy nef single model information quantization parameters in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    quantization_parameters_dst->version = quantization_parameters_src->version;

    if (KP_MODEL_QUANTIZATION_PARAMS_VERSION_1 == quantization_parameters_src->version) {
        kp_quantization_parameters_v1_t* quantization_parameters_dst_v1         = &(quantization_parameters_dst->quantization_parameters_data.v1);
        kp_quantization_parameters_v1_t* quantization_parameters_src_v1         = &(quantization_parameters_src->quantization_parameters_data.v1);

        quantization_parameters_dst_v1->quantized_axis                          = quantization_parameters_src_v1->quantized_axis;
        quantization_parameters_dst_v1->quantized_fixed_point_descriptor_num    = quantization_parameters_src_v1->quantized_fixed_point_descriptor_num;
        quantization_parameters_dst_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_dst_v1->quantized_fixed_point_descriptor, quantization_parameters_dst_v1->quantized_fixed_point_descriptor_num);

        if (0 < quantization_parameters_dst_v1->quantized_fixed_point_descriptor_num &&
            NULL == quantization_parameters_dst_v1->quantized_fixed_point_descriptor) {
            err_print("cpoy nef single model information quantization parameters in model_descriptor fail: alloc memory fail ...\n");
            return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        }

        memcpy((void *)quantization_parameters_dst_v1->quantized_fixed_point_descriptor, (void *)quantization_parameters_src_v1->quantized_fixed_point_descriptor, quantization_parameters_dst_v1->quantized_fixed_point_descriptor_num * sizeof(kp_quantized_fixed_point_descriptor_t));
    } else {
        err_print("copy nef single model information quantization parameters in model_descriptor fail: invalide quantization parameters version ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    return KP_SUCCESS;
}

int copy_single_model_descriptor_tensor(kp_tensor_descriptor_t *tensor_info_dst, kp_tensor_descriptor_t *tensor_info_src) {
    if (NULL == tensor_info_dst ||
        NULL == tensor_info_src) {
        err_print("copy nef single model information tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    tensor_info_dst->index                      = tensor_info_src->index;
    tensor_info_dst->name                       = strcpy_dst_realloc(tensor_info_dst->name, tensor_info_src->name);
    tensor_info_dst->data_layout                = tensor_info_src->data_layout;
    tensor_info_dst->tensor_shape_info.version  = tensor_info_src->tensor_shape_info.version;

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == tensor_info_dst->tensor_shape_info.version) {
        kp_tensor_shape_info_v1_t *tensor_shape_info_dst    = &(tensor_info_dst->tensor_shape_info.tensor_shape_info_data.v1);
        kp_tensor_shape_info_v1_t *tensor_shape_info_src    = &(tensor_info_src->tensor_shape_info.tensor_shape_info_data.v1);

        tensor_shape_info_dst->shape_npu_len                = tensor_shape_info_src->shape_npu_len;
        tensor_shape_info_dst->shape_npu                    = realloc_tensor_shape_int32_t(tensor_shape_info_dst->shape_npu, tensor_shape_info_dst->shape_npu_len);

        tensor_shape_info_dst->shape_onnx_len               = tensor_shape_info_src->shape_onnx_len;
        tensor_shape_info_dst->shape_onnx                   = realloc_tensor_shape_int32_t(tensor_shape_info_dst->shape_onnx, tensor_shape_info_dst->shape_onnx_len);

        tensor_shape_info_dst->axis_permutation_len         = tensor_shape_info_src->axis_permutation_len;
        tensor_shape_info_dst->axis_permutation_onnx_to_npu = realloc_tensor_shape_int32_t(tensor_shape_info_dst->axis_permutation_onnx_to_npu, tensor_shape_info_dst->axis_permutation_len);
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == tensor_info_dst->tensor_shape_info.version) {
        kp_tensor_shape_info_v2_t *tensor_shape_info_dst    = &(tensor_info_dst->tensor_shape_info.tensor_shape_info_data.v2);
        kp_tensor_shape_info_v2_t *tensor_shape_info_src    = &(tensor_info_src->tensor_shape_info.tensor_shape_info_data.v2);

        tensor_shape_info_dst->shape_len                    = tensor_shape_info_src->shape_len;
        tensor_shape_info_dst->shape                        = realloc_tensor_shape_int32_t(tensor_shape_info_dst->shape, tensor_shape_info_dst->shape_len);
        tensor_shape_info_dst->stride_onnx                  = realloc_tensor_shape_uint32_t(tensor_shape_info_dst->stride_onnx, tensor_shape_info_dst->shape_len);
        tensor_shape_info_dst->stride_npu                   = realloc_tensor_shape_uint32_t(tensor_shape_info_dst->stride_npu, tensor_shape_info_dst->shape_len);
    } else {
        err_print("copy nef single model information tensor in model_descriptor fail: invalide tensor shape information version ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    kp_quantization_parameters_t *quantization_parameters_dst = &(tensor_info_dst->quantization_parameters);
    kp_quantization_parameters_t *quantization_parameters_src = &(tensor_info_src->quantization_parameters);
    status = copy_single_tensor_info_quantization_parameters(quantization_parameters_dst, quantization_parameters_src);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = is_tensor_info_reallocted(tensor_info_dst);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1 == tensor_info_dst->tensor_shape_info.version) {
        kp_tensor_shape_info_v1_t *tensor_shape_info_dst = &(tensor_info_dst->tensor_shape_info.tensor_shape_info_data.v1);
        kp_tensor_shape_info_v1_t *tensor_shape_info_src = &(tensor_info_src->tensor_shape_info.tensor_shape_info_data.v1);

        memcpy(tensor_shape_info_dst->shape_npu, tensor_shape_info_src->shape_npu, tensor_shape_info_dst->shape_npu_len * sizeof(int32_t));
        memcpy(tensor_shape_info_dst->shape_onnx, tensor_shape_info_src->shape_onnx, tensor_shape_info_dst->shape_onnx_len * sizeof(int32_t));
        memcpy(tensor_shape_info_dst->axis_permutation_onnx_to_npu, tensor_shape_info_src->axis_permutation_onnx_to_npu, tensor_shape_info_dst->axis_permutation_len * sizeof(int32_t));
    } else if (KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2 == tensor_info_dst->tensor_shape_info.version) {
        kp_tensor_shape_info_v2_t *tensor_shape_info_dst = &(tensor_info_dst->tensor_shape_info.tensor_shape_info_data.v2);
        kp_tensor_shape_info_v2_t *tensor_shape_info_src = &(tensor_info_src->tensor_shape_info.tensor_shape_info_data.v2);

        memcpy(tensor_shape_info_dst->shape, tensor_shape_info_src->shape, tensor_shape_info_dst->shape_len * sizeof(int32_t));
        memcpy(tensor_shape_info_dst->stride_onnx, tensor_shape_info_src->stride_onnx, tensor_shape_info_dst->shape_len * sizeof(uint32_t));
        memcpy(tensor_shape_info_dst->stride_npu, tensor_shape_info_src->stride_npu, tensor_shape_info_dst->shape_len * sizeof(uint32_t));
    } else {
        err_print("copy nef single model information tensor in model_descriptor fail: invalide tensor shape information version ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

FUNC_OUT:

    return status;
}

int copy_single_model_descriptor_inputs_tensor(kp_single_model_descriptor_t *single_model_descriptor_dst, kp_single_model_descriptor_t *single_model_descriptor_src) {
    if (NULL == single_model_descriptor_dst ||
        NULL == single_model_descriptor_src) {
        err_print("copy nef single model information inputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status                                      = KP_SUCCESS;

    kp_tensor_descriptor_t *tensor_info_dst         = NULL;
    kp_tensor_descriptor_t *tensor_info_src         = NULL;

    single_model_descriptor_dst->input_nodes_num    = single_model_descriptor_src->input_nodes_num;
    single_model_descriptor_dst->input_nodes        = realloc_tensor_list(single_model_descriptor_dst->input_nodes, single_model_descriptor_dst->input_nodes_num);

    if (0 < single_model_descriptor_dst->input_nodes_num &&
        NULL == single_model_descriptor_dst->input_nodes) {
        err_print("copy nef single model information inputs tensor in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int i = 0; i < single_model_descriptor_dst->input_nodes_num; i++) {
        tensor_info_dst = &(single_model_descriptor_dst->input_nodes[i]);
        tensor_info_src = &(single_model_descriptor_src->input_nodes[i]);

        status = copy_single_model_descriptor_tensor(tensor_info_dst, tensor_info_src);
        if (KP_SUCCESS != status) {
            err_print("copy nef single model information inputs tensor in model_descriptor fail: constuct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}

int copy_single_model_descriptor_outputs_tensor(kp_single_model_descriptor_t *single_model_descriptor_dst, kp_single_model_descriptor_t *single_model_descriptor_src) {
    if (NULL == single_model_descriptor_dst ||
        NULL == single_model_descriptor_src) {
        err_print("copy nef single model information outputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status                                      = KP_SUCCESS;

    kp_tensor_descriptor_t *tensor_info_dst         = NULL;
    kp_tensor_descriptor_t *tensor_info_src         = NULL;

    single_model_descriptor_dst->output_nodes_num   = single_model_descriptor_src->output_nodes_num;
    single_model_descriptor_dst->output_nodes       = realloc_tensor_list(single_model_descriptor_dst->output_nodes, single_model_descriptor_dst->output_nodes_num);

    if (0 < single_model_descriptor_dst->output_nodes_num &&
        NULL == single_model_descriptor_dst->output_nodes) {
        err_print("copy nef single model information outputs tensor in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int i = 0; i < single_model_descriptor_dst->output_nodes_num; i++) {
        tensor_info_dst = &(single_model_descriptor_dst->output_nodes[i]);
        tensor_info_src = &(single_model_descriptor_src->output_nodes[i]);

        status = copy_single_model_descriptor_tensor(tensor_info_dst, tensor_info_src);
        if (KP_SUCCESS != status) {
            err_print("copy nef single model information outputs tensor in model_descriptor fail: constuct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}

int copy_single_model_descriptor(kp_single_model_descriptor_t *single_model_descriptor_dst, kp_single_model_descriptor_t *single_model_descriptor_src) {
    if (NULL == single_model_descriptor_dst ||
        NULL == single_model_descriptor_src) {
        err_print("copy nef single model information in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    single_model_descriptor_dst->target                     = single_model_descriptor_src->target;
    single_model_descriptor_dst->version                    = single_model_descriptor_src->version;
    single_model_descriptor_dst->id                         = single_model_descriptor_src->id;
    single_model_descriptor_dst->setup_bin_schema_version   = single_model_descriptor_src->setup_bin_schema_version;
    single_model_descriptor_dst->file_schema_version        = single_model_descriptor_src->file_schema_version;
    single_model_descriptor_dst->max_raw_out_size           = single_model_descriptor_src->max_raw_out_size;

    status = copy_single_model_descriptor_inputs_tensor(single_model_descriptor_dst, single_model_descriptor_src);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = copy_single_model_descriptor_outputs_tensor(single_model_descriptor_dst, single_model_descriptor_src);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

FUNC_OUT:

    return status;
}


int copy_model_des_nef_metadata(kp_model_nef_descriptor_t *loaded_model_desc_dst /* output */, kp_model_nef_descriptor_t *loaded_model_desc_src) {
    if (NULL == loaded_model_desc_dst ||
        NULL == loaded_model_desc_src) {
        err_print("copy nef metadata in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    kp_model_nef_metadata_t *loaded_model_nef_metadata_dst = &(loaded_model_desc_dst->metadata);
    kp_model_nef_metadata_t *loaded_model_nef_metadata_src = &(loaded_model_desc_src->metadata);

    loaded_model_nef_metadata_dst->kn_num =                         loaded_model_nef_metadata_src->kn_num;
    loaded_model_nef_metadata_dst->compiler_version =               strcpy_dst_realloc(loaded_model_nef_metadata_dst->compiler_version,     loaded_model_nef_metadata_src->compiler_version);
    loaded_model_nef_metadata_dst->toolchain_version =              strcpy_dst_realloc(loaded_model_nef_metadata_dst->toolchain_version,    loaded_model_nef_metadata_src->toolchain_version);
    loaded_model_nef_metadata_dst->platform =                       strcpy_dst_realloc(loaded_model_nef_metadata_dst->platform,             loaded_model_nef_metadata_src->platform);
    loaded_model_nef_metadata_dst->nef_schema_version.major =       loaded_model_nef_metadata_src->nef_schema_version.major;
    loaded_model_nef_metadata_dst->nef_schema_version.minor =       loaded_model_nef_metadata_src->nef_schema_version.minor;
    loaded_model_nef_metadata_dst->nef_schema_version.revision =    loaded_model_nef_metadata_src->nef_schema_version.revision;

    if (NULL == loaded_model_nef_metadata_dst->compiler_version ||
        NULL == loaded_model_nef_metadata_dst->toolchain_version||
        NULL == loaded_model_nef_metadata_dst->platform) {
        err_print("copy nef metadata in model_descriptor fail: copy nef meta data fail ...\n");
        return KP_ERROR_INVALID_MODEL_21;
    }

    return KP_SUCCESS;
}

/******************************************************************
 * nef info copier
 ******************************************************************/

int copy_model_nef_descriptor(kp_model_nef_descriptor_t *loaded_model_desc_dst /* output */, kp_model_nef_descriptor_t *loaded_model_desc_src)
{
    if ((NULL == loaded_model_desc_dst) || (NULL == loaded_model_desc_src))
    {
        err_print("invalid parameters, null pointer ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status                                                  = KP_SUCCESS;
    kp_single_model_descriptor_t *single_model_descriptor_dst   = NULL;
    kp_single_model_descriptor_t *single_model_descriptor_src   = NULL;

    status = deconstruct_model_nef_descriptor(loaded_model_desc_dst);
    if (KP_SUCCESS != status)
    {
        err_print("copy model nef descriptor failed: %d...\n", status);
        goto FUNC_OUT;
    }

    // copy nef metadata
    status = copy_model_des_nef_metadata(loaded_model_desc_dst, loaded_model_desc_src);
    if (KP_SUCCESS != status)
    {
        err_print("copy model nef metadata failed: %d...\n", status);
        goto FUNC_OUT;
    }

    // copy nef header info and malloc models information memeory
    loaded_model_desc_dst->magic        = loaded_model_desc_src->magic;
    loaded_model_desc_dst->target       = loaded_model_desc_src->target;
    loaded_model_desc_dst->crc          = loaded_model_desc_src->crc;
    loaded_model_desc_dst->num_models   = loaded_model_desc_src->num_models;
    loaded_model_desc_dst->models       = realloc_model_descriptor_list(loaded_model_desc_dst->models, loaded_model_desc_dst->num_models);
    if (0 < loaded_model_desc_dst->num_models &&
        NULL == loaded_model_desc_dst->models) {
        err_print("copy nef model_descriptor fail: remalloc single model descriptor fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int i = 0; i < loaded_model_desc_src->num_models; i++) {
        single_model_descriptor_dst = &(loaded_model_desc_dst->models[i]);
        single_model_descriptor_src = &(loaded_model_desc_src->models[i]);

        status = copy_single_model_descriptor(single_model_descriptor_dst, single_model_descriptor_src);
        if (KP_SUCCESS != status)
        {
            err_print("copy model nef metadata failed: %d...\n", status);
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}
