/**
 * @file        kneron_kne_reader.c
 * @brief       NEF model related functions - read NEF
 * @version     0.1
 * @date        2021-03-22
 *
 * @copyright   Copyright (c) 2021 Kneron Inc. All rights reserved.
 */

#include "internal_func.h"
#include "kneron_kne_c_reader.h"
#include "kp_struct.h"
#include <stdio.h>
#include <stdint.h>

#define err_print(format, ...)  { printf(format, ##__VA_ARGS__); fflush(stdout); }
#define MAX(a, b)               (((a)>(b))?(a):(b))

/******************************************************************
 * [private] KNE model constructor utils
 ******************************************************************/

int construct_kne_data_type_size_flatbuffer(KneronKNE_DataType_enum_t kne_data_type_enum, uint32_t *data_type_size) {
    int status = KP_SUCCESS;

    if (NULL == data_type_size) {
        err_print("construct KNE data type size in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    switch (kne_data_type_enum)
    {
    case KneronKNE_DataType_Int8:
        *data_type_size = sizeof(int8_t);
        break;
    case KneronKNE_DataType_Int16:
        *data_type_size = sizeof(int16_t);
        break;
    case KneronKNE_DataType_Int32:
        *data_type_size = sizeof(int32_t);
        break;
    case KneronKNE_DataType_Int64:
        *data_type_size = sizeof(int64_t);
        break;
    case KneronKNE_DataType_UInt8:
        *data_type_size = sizeof(uint8_t);
        break;
    case KneronKNE_DataType_UInt16:
        *data_type_size = sizeof(uint16_t);
        break;
    case KneronKNE_DataType_Uint32:
        *data_type_size = sizeof(uint32_t);
        break;
    case KneronKNE_DataType_UInt64:
        *data_type_size = sizeof(uint64_t);
        break;
    case KneronKNE_DataType_Float:
        *data_type_size = sizeof(float);
        break;
    case KneronKNE_DataType_Bfloat16:
        *data_type_size = sizeof(uint16_t);
        break;
    case KneronKNE_DataType_Double:
        *data_type_size = sizeof(double);
        break;
    default:
        err_print("construct nef single tensor information quantization parameters in model_descriptor fail: invalid KneronKNE_DataType_enum_t\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

FUNC_OUT:
    return status;
}

int construct_kne_data_type_flatbuffer(KneronKNE_DataType_enum_t kne_data_type_enum, kp_dtype_t *kp_data_type_enum) {
    int status = KP_SUCCESS;

    if (NULL == kp_data_type_enum) {
        err_print("construct KNE data type in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    switch (kne_data_type_enum)
    {
    case KneronKNE_DataType_Int8:
        *kp_data_type_enum = KP_DTYPE_INT8;
        break;
    case KneronKNE_DataType_Int16:
        *kp_data_type_enum = KP_DTYPE_INT16;
        break;
    case KneronKNE_DataType_Int32:
        *kp_data_type_enum = KP_DTYPE_INT32;
        break;
    case KneronKNE_DataType_Int64:
        *kp_data_type_enum = KP_DTYPE_INT64;
        break;
    case KneronKNE_DataType_UInt8:
        *kp_data_type_enum = KP_DTYPE_UINT8;
        break;
    case KneronKNE_DataType_UInt16:
        *kp_data_type_enum = KP_DTYPE_UINT16;
        break;
    case KneronKNE_DataType_Uint32:
        *kp_data_type_enum = KP_DTYPE_UINT32;
        break;
    case KneronKNE_DataType_UInt64:
        *kp_data_type_enum = KP_DTYPE_UINT64;
        break;
    case KneronKNE_DataType_Float:
        *kp_data_type_enum = KP_DTYPE_FLOAT32;
        break;
    case KneronKNE_DataType_Bfloat16:
        *kp_data_type_enum = KP_DTYPE_BFLOAT16;
        break;
    case KneronKNE_DataType_Double:
        *kp_data_type_enum = KP_DTYPE_DOUBLE64;
        break;
    default:
        err_print("construct KNE data type in model_descriptor fail: invalid KneronKNE_DataType_enum_t\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

FUNC_OUT:
    return status;
}

int construct_kne_single_tensor_info_quantization_parameters_flatbuffer(KneronKNE_QuantizationParameters_table_t quantization_parameters_flatbuffer, kp_quantization_parameters_t *quantization_parameters) {
    int status = KP_SUCCESS;
    KneronKNE_DataType_enum_t kne_data_type_enum;
    kp_dtype_t kp_data_type_enum;
    uint32_t data_type_size;
    flatbuffers_int8_vec_t radix_vec;
    flatbuffers_uint8_vec_t scale_vec;
    flatbuffers_uint8_vec_t scale_pointer;
    kp_quantization_parameters_v1_t *quantization_parameters_v1;
    kp_quantized_fixed_point_descriptor_t *quantized_fixed_point_descriptor;

    if (NULL == quantization_parameters_flatbuffer ||
        NULL == quantization_parameters) {
        err_print("construct nef single tensor information quantization parameters in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    /* get scale data type and data size */
    kne_data_type_enum  = KneronKNE_QuantizationParameters_scale_type(quantization_parameters_flatbuffer);
    status              = construct_kne_data_type_flatbuffer(kne_data_type_enum, &kp_data_type_enum);
    if (KP_SUCCESS != status)
    {
        err_print("construct nef single tensor information quantization parameters in model_descriptor fail: the scale data type is %d ...\n", kne_data_type_enum);
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    status = construct_kne_data_type_size_flatbuffer(kne_data_type_enum, &data_type_size);
    if (KP_SUCCESS != status) {
        err_print("construct nef single tensor information quantization parameters in model_descriptor fail: the scale data type is %d ...\n", kne_data_type_enum);
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;
    quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);

    /* get radix/scale length */
    radix_vec = KneronKNE_QuantizationParameters_radix(quantization_parameters_flatbuffer);
    scale_vec = KneronKNE_QuantizationParameters_scale(quantization_parameters_flatbuffer);

    quantization_parameters_v1->quantized_fixed_point_descriptor_num    = MAX(KneronKNE_QuantizationParameters_scale_count(quantization_parameters_flatbuffer), flatbuffers_int8_vec_len(radix_vec));
    quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

    if ((0 < quantization_parameters_v1->quantized_fixed_point_descriptor_num) &&
        (NULL == quantization_parameters_v1->quantized_fixed_point_descriptor)) {
        err_print("construct nef single tensor information quantization parameters in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    scale_pointer = scale_vec;
    for (int idx = 0; idx < quantization_parameters_v1->quantized_fixed_point_descriptor_num; idx++) {
        quantized_fixed_point_descriptor                = &(quantization_parameters_v1->quantized_fixed_point_descriptor[idx]);
        quantized_fixed_point_descriptor->radix         = (1 == flatbuffers_int8_vec_len(radix_vec)) ? (int32_t)radix_vec[0] : (int32_t)radix_vec[idx];
        quantized_fixed_point_descriptor->scale_dtype   = kp_data_type_enum;
        switch (kp_data_type_enum)
        {
        case KP_DTYPE_INT8:
            quantized_fixed_point_descriptor->scale.scale_int8      = *(int8_t *)scale_pointer;
            break;
        case KP_DTYPE_INT16:
            quantized_fixed_point_descriptor->scale.scale_int16     = *(int16_t *)scale_pointer;
            break;
        case KP_DTYPE_INT32:
            quantized_fixed_point_descriptor->scale.scale_int32     = *(int32_t *)scale_pointer;
            break;
        case KP_DTYPE_INT64:
            quantized_fixed_point_descriptor->scale.scale_int64     = *(int64_t *)scale_pointer;
            break;
        case KP_DTYPE_UINT8:
            quantized_fixed_point_descriptor->scale.scale_uint8     = *(uint8_t *)scale_pointer;
            break;
        case KP_DTYPE_UINT16:
            quantized_fixed_point_descriptor->scale.scale_uint16    = *(uint16_t *)scale_pointer;
            break;
        case KP_DTYPE_UINT32:
            quantized_fixed_point_descriptor->scale.scale_uint32    = *(uint32_t *)scale_pointer;
            break;
        case KP_DTYPE_UINT64:
            quantized_fixed_point_descriptor->scale.scale_uint64    = *(uint64_t *)scale_pointer;
            break;
        case KP_DTYPE_FLOAT32:
            quantized_fixed_point_descriptor->scale.scale_float32   = *(float *)scale_pointer;
            break;
        case KP_DTYPE_BFLOAT16:
            quantized_fixed_point_descriptor->scale.scale_bfloat16  = *(uint16_t *)scale_pointer;
            break;
        case KP_DTYPE_DOUBLE64:
            quantized_fixed_point_descriptor->scale.scale_double64  = *(double *)scale_pointer;
            break;
        default:
            err_print("construct nef single tensor information quantization parameters in model_descriptor fail: invalid KneronKNE_DataType_enum_t\n");
            status = KP_ERROR_INVALID_MODEL_21;
            goto FUNC_OUT;
        }

        if (1 < KneronKNE_QuantizationParameters_scale_count(quantization_parameters_flatbuffer))
            scale_pointer += data_type_size;
    }

FUNC_OUT:
    return status;
}

int construct_kne_single_model_tensors_info(KneronKNE_Tensor_table_t tensor, uint32_t target_chip, kp_tensor_descriptor_t *tensor_descriptor)
{
    int status                                                  = KP_SUCCESS;
    kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
    kp_tensor_shape_info_v2_t *tensor_shape_info_v2             = NULL;
    kp_quantization_parameters_t *quantization_parameters       = NULL;
    kp_quantization_parameters_v1_t *quantization_parameters_v1 = NULL;
    flatbuffers_int32_vec_t shape_onnx;
    flatbuffers_uint32_vec_t stride_npu;
    KneronKNE_QuantizationParameters_table_t quantization_parameters_flatbuffer;

    if ((NULL == tensor) ||
        (NULL == tensor_descriptor)) {
        err_print("construct nef single model tensors information in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    tensor_descriptor->name                         = strcpy_dst_realloc(tensor_descriptor->name, (char*)KneronKNE_Tensor_name(tensor));
    tensor_descriptor->data_layout                  = convert_data_format_to_kp_tensor_format(KneronKNE_Tensor_format(tensor), target_chip);

    tensor_shape_info                               = &(tensor_descriptor->tensor_shape_info);
    tensor_shape_info->version                      = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_2;

    tensor_shape_info_v2                            = &(tensor_descriptor->tensor_shape_info.tensor_shape_info_data.v2);

    /* shape onnx parse - malloc */
    shape_onnx                                      = KneronKNE_Tensor_shape(tensor);

    tensor_shape_info_v2->shape_len                 = flatbuffers_int32_vec_len(shape_onnx);
    tensor_shape_info_v2->shape                     = realloc_tensor_shape_int32_t(tensor_shape_info_v2->shape, tensor_shape_info_v2->shape_len);

    /* build npu stride - malloc */
    stride_npu                                      = KneronKNE_Tensor_stride_aligned(tensor);

    tensor_shape_info_v2->stride_npu                = realloc_tensor_shape_uint32_t(tensor_shape_info_v2->stride_npu, tensor_shape_info_v2->shape_len);

    /* build onnx stride - malloc */
    tensor_shape_info_v2->stride_onnx               = realloc_tensor_shape_uint32_t(tensor_shape_info_v2->stride_onnx, tensor_shape_info_v2->shape_len);

    /* quantization information parse */
    quantization_parameters                         = &(tensor_descriptor->quantization_parameters);
    quantization_parameters_v1                      = &(quantization_parameters->quantization_parameters_data.v1);
    quantization_parameters_flatbuffer              = KneronKNE_Tensor_quantization(tensor);
    quantization_parameters_v1->quantized_axis      = KneronKNE_Tensor_ch_dim(tensor);
    status                                          = construct_kne_single_tensor_info_quantization_parameters_flatbuffer(quantization_parameters_flatbuffer, quantization_parameters);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status                                          = is_tensor_info_reallocted(tensor_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    /* shape onnx parse - build */
    memcpy(tensor_shape_info_v2->shape, shape_onnx, tensor_shape_info_v2->shape_len * flatbuffers_int32__size());

    /* build npu stride - build */
    memcpy(tensor_shape_info_v2->stride_npu, stride_npu, tensor_shape_info_v2->shape_len * flatbuffers_uint32__size());

    /* build onnx stride - build */
    for (int dimension = 0; dimension < tensor_shape_info_v2->shape_len; dimension++) {
        tensor_shape_info_v2->stride_onnx[dimension] = 1;
    }

    for (int dimension = tensor_shape_info_v2->shape_len - 2; dimension > -1; dimension--) {
        tensor_shape_info_v2->stride_onnx[dimension] *= (tensor_shape_info_v2->stride_onnx[dimension + 1] * tensor_shape_info_v2->shape[dimension + 1]);
    }

FUNC_OUT:
    return status;
}

int construct_kne_single_model_input_tensor_info(KneronKNE_ModelHeader_table_t model_header, kp_single_model_descriptor_t *single_model_descriptor)
{
    int status                                  = KP_SUCCESS;
    kp_tensor_descriptor_t *tensor_descriptor   = NULL;
    KneronKNE_Tensor_vec_t tensor_vec;
    KneronKNE_Tensor_table_t tensor;

    if ((NULL == model_header) ||
        (NULL == single_model_descriptor)) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    tensor_vec = KneronKNE_ModelHeader_inputs(model_header);
    if (NULL == tensor_vec) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: invalid flatbuffer ...\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    single_model_descriptor->input_nodes_num    = KneronKNE_Tensor_vec_len(tensor_vec);
    single_model_descriptor->input_nodes        = realloc_tensor_list(single_model_descriptor->input_nodes, single_model_descriptor->input_nodes_num);

    if (0 < single_model_descriptor->input_nodes_num &&
        NULL == single_model_descriptor->input_nodes) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int idx = 0; idx < single_model_descriptor->input_nodes_num; idx++) {
        tensor                      = KneronKNE_Tensor_vec_at(tensor_vec, idx);
        tensor_descriptor           = &(single_model_descriptor->input_nodes[idx]);
        tensor_descriptor->index    = idx;

        status = construct_kne_single_model_tensors_info(tensor, single_model_descriptor->target, tensor_descriptor);
        if (KP_SUCCESS != status) {
            err_print("construct nef single model information inputs tensor in model_descriptor fail: construct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:
    return status;
}

int construct_kne_single_model_output_tensor_info(KneronKNE_ModelHeader_table_t model_header, kp_single_model_descriptor_t *single_model_descriptor)
{
    int status                                  = KP_SUCCESS;
    kp_tensor_descriptor_t *tensor_descriptor   = NULL;
    KneronKNE_Tensor_vec_t tensor_vec;
    KneronKNE_Tensor_table_t tensor;

    if ((NULL == model_header) ||
        (NULL == single_model_descriptor)) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    tensor_vec = KneronKNE_ModelHeader_outputs(model_header);
    if (NULL == tensor_vec) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: invalid flatbuffer ...\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    single_model_descriptor->output_nodes_num   = KneronKNE_Tensor_vec_len(tensor_vec);
    single_model_descriptor->output_nodes       = realloc_tensor_list(single_model_descriptor->output_nodes, single_model_descriptor->output_nodes_num);

    if (0 < single_model_descriptor->output_nodes_num &&
        NULL == single_model_descriptor->output_nodes) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int idx = 0; idx < single_model_descriptor->output_nodes_num; idx++) {
        tensor                      = KneronKNE_Tensor_vec_at(tensor_vec, idx);
        tensor_descriptor           = &(single_model_descriptor->output_nodes[idx]);
        tensor_descriptor->index    = idx;

        status = construct_kne_single_model_tensors_info(tensor, single_model_descriptor->target, tensor_descriptor);
        if (KP_SUCCESS != status) {
            err_print("construct nef single model information outputs tensor in model_descriptor fail: construct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:
    return status;
}

int construct_kne_single_model_schema_info(KneronKNE_ModelHeader_table_t model_header, kp_single_model_descriptor_t *single_model_descriptor)
{
    int status = KP_SUCCESS;
    KneronKNE_SchemaVersion_table_t schema_version = NULL;

    if ((NULL == model_header) ||
        (NULL == single_model_descriptor)) {
        err_print("construct nef single model schema information in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    schema_version = KneronKNE_ModelHeader_schema_version(model_header);
    if (NULL == schema_version) {
        err_print("construct nef single model schema information in model_descriptor fail: invalid flatbuffer ...\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    single_model_descriptor->setup_bin_schema_version.major     = KneronKNE_SchemaVersion_major_num(schema_version);
    single_model_descriptor->setup_bin_schema_version.minor     = KneronKNE_SchemaVersion_minor_num(schema_version);
    single_model_descriptor->setup_bin_schema_version.revision  = KneronKNE_SchemaVersion_revision_num(schema_version);

FUNC_OUT:
    return status;
}

int construct_kne_single_model_info(KneronKNE_Model_table_t kne_model, kp_single_model_descriptor_t *single_model_descriptor)
{
    int status = KP_SUCCESS;
    KneronKNE_ModelHeader_table_t model_header = NULL;

    if ((NULL == kne_model) ||
        (NULL == single_model_descriptor)) {
        err_print("construct nef single model information in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    model_header = KneronKNE_Model_header(kne_model);
    if (NULL == model_header) {
        err_print("construct nef single model information in model_descriptor fail: invalid flatbuffer ...\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    single_model_descriptor->id = KneronKNE_ModelHeader_id(model_header);

    status = construct_kne_single_model_schema_info(model_header, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = construct_kne_single_model_input_tensor_info(model_header, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = construct_kne_single_model_output_tensor_info(model_header, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

FUNC_OUT:
    return status;
}

/******************************************************************
 * [private] KNE reader utils
 ******************************************************************/

int get_kne_header(KneronKNE_KNEContent_table_t* table_p, kp_kne_info_t *kne_info)
{
    if (table_p == NULL) {
        return KP_ERROR_INVALID_PARAM_12;
    }

    KneronKNE_KNEHeader_table_t nef_header = KneronKNE_KNEContent_header(*table_p);
    if (NULL == nef_header || NULL == kne_info) {
        return KP_ERROR_INVALID_MODEL_21;
    }

    kne_info->target        = KneronKNE_KNEHeader_target(nef_header);
    kne_info->kne_header    = (uintptr_t)nef_header;

    return KP_SUCCESS;
}

int get_kne_models(KneronKNE_KNEContent_table_t* table_p, kp_kne_info_t *kne_info)
{
    if (table_p == NULL) {
        return KP_ERROR_INVALID_PARAM_12;
    }

    KneronKNE_Model_vec_t kne_model_vec = KneronKNE_KNEContent_models(*table_p);
    if (NULL == kne_model_vec || NULL == kne_info) {
        return KP_ERROR_INVALID_MODEL_21;
    }

    kne_info->kne_model_vec = (uintptr_t)kne_model_vec;

    return KP_SUCCESS;
}

/******************************************************************
 * [public] KNE reader
 ******************************************************************/

int read_kne(uintptr_t kne_data, kp_kne_info_t *kne_info)
{
    int status                          = KP_SUCCESS;
    KneronKNE_KNEContent_table_t table  = KneronKNE_KNEContent_as_root((char*)kne_data);
    if (table == NULL)
        return KP_ERROR_INVALID_MODEL_21;

    status = get_kne_header(&table, kne_info);
    if (status != 0)
        return KP_ERROR_INVALID_MODEL_21;

    status = get_kne_models(&table, kne_info);
    if (status != 0)
        return KP_ERROR_INVALID_MODEL_21;

    return status;
}

int get_kne_single_model_output_buffer_size(uintptr_t kne_model_vec_ptr, uint32_t model_id, size_t *output_buffer_size)
{
    int status = KP_SUCCESS;
    KneronKNE_Model_vec_t kne_model_vec;
    KneronKNE_Model_table_t kne_model;
    KneronKNE_ModelHeader_table_t kne_model_header;
    KneronKNE_BufferInfo_vec_t buffer_info_vec;
    KneronKNE_BufferInfo_table_t buffer_info;
    if ((NULL == (void *)kne_model_vec_ptr) ||
        (NULL == output_buffer_size)) {
        err_print("get kne single model output buffer size fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    kne_model_vec = (KneronKNE_Model_vec_t)kne_model_vec_ptr;
    for (int model_idx = 0; model_idx < KneronKNE_Model_vec_len(kne_model_vec); model_idx++) {
        kne_model = KneronKNE_Model_vec_at(kne_model_vec, model_idx);
        if (NULL == kne_model) {
            err_print("get kne single model output buffer size fail: invalid flatbuffer ...\n");
            status = KP_ERROR_INVALID_MODEL_21;
            goto FUNC_OUT;
        }

        kne_model_header = KneronKNE_Model_header(kne_model);
        if (NULL == kne_model_header) {
            err_print("get kne single model output buffer size fail: invalid flatbuffer ...\n");
            status = KP_ERROR_INVALID_MODEL_21;
            goto FUNC_OUT;
        }

        if (model_id == KneronKNE_ModelHeader_id(kne_model_header)) {
            buffer_info_vec = KneronKNE_ModelHeader_buffer_info(kne_model_header);
            if (NULL == buffer_info_vec) {
                err_print("get kne single model output buffer size fail: invalid flatbuffer ...\n");
                status = KP_ERROR_INVALID_MODEL_21;
                goto FUNC_OUT;
            }

            for (int buff_info_idx = 0; buff_info_idx < KneronKNE_BufferInfo_vec_len(buffer_info_vec); buff_info_idx++) {
                buffer_info = KneronKNE_BufferInfo_vec_at(buffer_info_vec, buff_info_idx);
                if (NULL == buffer_info) {
                    err_print("get kne single model output buffer size fail: invalid flatbuffer ...\n");
                    status = KP_ERROR_INVALID_MODEL_21;
                    goto FUNC_OUT;
                }

                if (KneronKNE_Location_OUTPUT_BUFFER == KneronKNE_BufferInfo_buffer(buffer_info)) {
                    *output_buffer_size = KneronKNE_BufferInfo_len(buffer_info);
                    goto FUNC_OUT;
                }
            }
        }
    }

    status = KP_ERROR_MODEL_NOT_LOADED_35;

FUNC_OUT:
    return status;
}

int construct_kne_models_info(uintptr_t kne_model_vec_ptr, kp_model_nef_descriptor_t* loaded_model_desc)
{
    int status = KP_SUCCESS;
    KneronKNE_Model_vec_t kne_model_vec;
    if ((NULL == (void *)kne_model_vec_ptr) ||
        (NULL == loaded_model_desc)) {
        err_print("construct nef models information in model_descriptor fail: NULL pointer input parameters ...\n");
        status = KP_ERROR_INVALID_PARAM_12;
        goto FUNC_OUT;
    }

    kne_model_vec                   = (KneronKNE_Model_vec_t)kne_model_vec_ptr;
    loaded_model_desc->num_models   = KneronKNE_Model_vec_len(kne_model_vec);
    loaded_model_desc->models       = realloc_model_descriptor_list(loaded_model_desc->models, loaded_model_desc->num_models);

    if ((0 < loaded_model_desc->num_models) &&
        (NULL == loaded_model_desc->models)) {
        err_print("construct nef models model_descriptor fail: realloc single model descriptor fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int idx = 0; idx < loaded_model_desc->num_models; idx++) {
        KneronKNE_Model_table_t kne_model                       = KneronKNE_Model_vec_at(kne_model_vec, idx);
        kp_single_model_descriptor_t *single_model_descriptor   = &(loaded_model_desc->models[idx]);

        /* setting target chip from NEF metadata */
        single_model_descriptor->target                         = loaded_model_desc->target;

        status = construct_kne_single_model_info(kne_model, single_model_descriptor);
        if (KP_SUCCESS != status) {
            status = KP_ERROR_INVALID_PARAM_12;
            goto FUNC_OUT;
        }
    }

FUNC_OUT:
    return status;
}
