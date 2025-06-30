/**
 * @file        setup_reader.c
 * @brief       NEF model related functions - read setup binary
 * @version     0.1
 * @date        2022-04-05
 *
 * @copyright   Copyright (c) 2022 Kneron Inc. All rights reserved.
 */

// #define DEBUG_PRINT

#include "internal_func.h"
#include "kneron_setup_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_PRINT
#define dbg_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }
#else
#define dbg_print(format, ...)
#endif

#define err_print(format, ...) { printf(format, ##__VA_ARGS__); fflush(stdout); }

#define SETUP_LEGACY_MAGIC_NUM      0x8EB5A462
#define SETUP_FLATBUFF_MAGIC_NUM    0x012EBD34

/******************************************************************
 * [Setup bin reader legacy]
 * read information by pre-defined struct
 *
 * [Setup bin reader flatbuffer]
 * read information by pre-defined flatbuffer schema (setup_reader.h)
 * more information please ref: kneron_npu_executable_format/setup_bin
 ******************************************************************/

/******************************************************************
 * setup legacy reader
 ******************************************************************/

/**
 * setup.bin schema: model node type
 * note: from kl520 firmware kdpio.h to parse setup.bin
 */
enum
{
    NODE_TYPE_IN,
    NODE_TYPE_CPU,
    NODE_TYPE_OUTPUT,
    NODE_TYPE_DATA,
    NODE_TYPE_SUPER,
    NODE_TYPE_INPUT,
};

/**
 * setup.bin schema: kl520 model cnn header
 * note: from firmware kdpio.h to parse setup.bin
 */
typedef struct
{
    uint32_t crc;
    uint32_t version;
    uint32_t reserved;
    uint32_t model_type;
    uint32_t app_type;
    uint32_t dram_start;
    uint32_t dram_size;
    uint32_t input_row;
    uint32_t input_col;
    uint32_t input_channel;
    uint32_t cmd_start;
    uint32_t cmd_size;
    uint32_t weight_start;
    uint32_t weight_size;
    uint32_t input_start;
    uint32_t input_size;
    uint32_t input_radix;
    uint32_t output_nums;
} __attribute__((packed, aligned(4))) _520_cnn_header_t;

/**
 * setup.bin schema: kl720 model cnn header
 * note: from firmware kdpio.h to parse setup.bin
 */
typedef struct
{
    uint32_t crc;
    uint32_t version;
    uint32_t reamaining_models;
    uint32_t model_type;
    uint32_t application_type;
    uint32_t dram_start;
    uint32_t dram_size;
    uint32_t cmd_start;
    uint32_t cmd_size;
    uint32_t weight_start;
    uint32_t weight_size;
    uint32_t input_start;
    uint32_t input_size;
    uint32_t input_num;
    uint32_t output_num;
} __attribute__((packed, aligned(4))) _720_cnn_header_t;

/**
 * setup.bin schema: kl720 model cnn netinput node (node_id = 5) header
 * note: from firmware kdpio.h 'NetInput_Node' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t input_index;
    uint32_t input_format;
    uint32_t input_row;
    uint32_t input_col;
    uint32_t input_channel;
    uint32_t input_start;
    uint32_t input_size;
    uint32_t input_radix;
} __attribute__((packed, aligned(4))) _720_net_input_node_t;

/**
 * setup.bin schema:model cnn super node (node_id = 4) header
 * note: from firmware kdpio.h 'super_node_s' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t addr;
    uint32_t row_start;
    uint32_t col_start;
    uint32_t ch_start;
    uint32_t row_length;
    uint32_t col_length;
    uint32_t ch_length;
} __attribute__((packed, aligned(4))) _super_node_t;

/**
 * setup.bin schema:model cnn output node (node_id = 2) header
 * note: from firmware kdpio.h 'out_node_s' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t supernum;
    uint32_t data_format;
    uint32_t row_start;
    uint32_t col_start;
    uint32_t ch_start;
    uint32_t row_length;
    uint32_t col_length;
    uint32_t ch_length;
    uint32_t output_index;
    uint32_t output_radix;
    uint32_t output_scale;
    _super_node_t* node_list;
} __attribute__((packed, aligned(4))) _out_node_t;

/**
 * setup.bin schema:model cnn data node (node_id = 3) header
 * note: from firmware kdpio.h 'data_node_s' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t supernum;
    uint32_t data_format;
    uint32_t data_radix;
    uint32_t data_scale;
    uint32_t row_start;
    uint32_t col_start;
    uint32_t ch_start;
    uint32_t row_length;
    uint32_t col_length;
    uint32_t ch_length;
    _super_node_t* node_list;
} __attribute__((packed, aligned(4))) _data_node_t;

/**
 * setup.bin schema:model cnn cpu node (node_id = 1) header
 * note: from firmware kdpio.h 'cpu_node_s' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t input_datanode_num;
    uint32_t op_type;
    uint32_t in_num_row;
    uint32_t in_num_col;
    uint32_t in_num_ch;
    uint32_t out_num_row;
    uint32_t out_num_col;
    uint32_t out_num_ch;
    uint32_t h_pad;
    uint32_t w_pad;
    uint32_t kernel_h;
    uint32_t kernel_w;
    uint32_t stride_h;
    uint32_t stride_w;
    _data_node_t* output_datanode;
    _data_node_t* input_datanodes;
} __attribute__((packed, aligned(4))) _cpu_node_t;

/**
 * setup.bin schema:model cnn cpu node (node_id = 0) header
 * note: from firmware kdpio.h 'in_node_s' to parse setup.bin
 */
typedef struct
{
    uint32_t node_id;
    uint32_t next_npu;
} __attribute__((packed, aligned(4))) _in_node_t;

int construct_single_setup_info_legacy(uintptr_t setup_buff, size_t setup_buff_size, kp_single_model_descriptor_t *single_model_descriptor) {
    if (NULL == (void *)setup_buff ||
        0 == setup_buff_size ||
        NULL == single_model_descriptor) {
        err_print("construct nef single model information in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    size_t setup_buff_offset    = 0;
    size_t setup_node_boundary  = 0;

    if (KP_MODEL_TARGET_CHIP_KL520 == single_model_descriptor->target) {            // kl520 setup
        _520_cnn_header_t *cnnh_520 = (_520_cnn_header_t *)setup_buff;
        setup_buff_offset += sizeof(_520_cnn_header_t);
        if ((sizeof(_520_cnn_header_t) > cnnh_520->reserved) || (setup_buff_size < cnnh_520->reserved)) {
            setup_node_boundary = setup_buff_size;
        } else {
            setup_node_boundary = cnnh_520->reserved;
        }

        single_model_descriptor->input_nodes_num    = 1;
        single_model_descriptor->output_nodes_num   = cnnh_520->output_nums;
    } else if (KP_MODEL_TARGET_CHIP_KL720 == single_model_descriptor->target) {     // kl720 setup
        _720_cnn_header_t *cnnh_720 = (_720_cnn_header_t *)setup_buff;
        setup_buff_offset           += sizeof(_720_cnn_header_t);

        if ((sizeof(_720_cnn_header_t) > cnnh_720->reamaining_models) || (setup_buff_size < cnnh_720->reamaining_models)) {
            setup_node_boundary = setup_buff_size;
        } else {
            setup_node_boundary = cnnh_720->reamaining_models;
        }

        single_model_descriptor->input_nodes_num    = cnnh_720->input_num;
        single_model_descriptor->output_nodes_num   = cnnh_720->output_num;
    } else {                                                                        // unknwon setup
        err_print("construct nef info in model_descriptor fail: invalid target %u ...\n", single_model_descriptor->target);
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    single_model_descriptor->input_nodes    = realloc_tensor_list(single_model_descriptor->input_nodes, single_model_descriptor->input_nodes_num);
    single_model_descriptor->output_nodes   = realloc_tensor_list(single_model_descriptor->output_nodes, single_model_descriptor->output_nodes_num);

    if ((0 < single_model_descriptor->input_nodes_num &&
        NULL == single_model_descriptor->input_nodes) ||
        (0 < single_model_descriptor->output_nodes_num &&
        NULL == single_model_descriptor->output_nodes)) {
        err_print("construct nef info in input/output node tensor info fail: realloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    dbg_print("input_nodes_num %u\n", single_model_descriptor->input_nodes_num);
    dbg_print("output_nodes_num %u\n", single_model_descriptor->output_nodes_num);

    if (KP_MODEL_TARGET_CHIP_KL520 == single_model_descriptor->target) { // kl520 input node
        kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = NULL;
        kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
        kp_tensor_descriptor_t *tensor_info                         = NULL;
        kp_quantization_parameters_v1_t *quantization_parameters_v1 = NULL;
        kp_quantization_parameters_t *quantization_parameters       = NULL;
        _520_cnn_header_t *cnnh_520                                 = (_520_cnn_header_t *)setup_buff;

        tensor_info                                         = &(single_model_descriptor->input_nodes[0]);
        tensor_info->index                                  = 0;
        tensor_info->name                                   = strcpy_dst_realloc(tensor_info->name, "");
        tensor_info->data_layout                            = convert_data_format_to_kp_tensor_format(DATA_FMT_KL520_4W4C8B, KP_MODEL_TARGET_CHIP_KL520);

        tensor_shape_info                                   = &(tensor_info->tensor_shape_info);
        tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

        tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
        tensor_shape_info_v1->shape_npu_len                 = 4;
        tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
        tensor_shape_info_v1->shape_onnx_len                = 4;
        tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
        tensor_shape_info_v1->axis_permutation_len          = 4;
        tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

        quantization_parameters                                             = &(tensor_info->quantization_parameters);
        quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

        quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
        quantization_parameters_v1->quantized_axis                          = 1;
        quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
        quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

        status = is_tensor_info_reallocted(tensor_info);

        if (KP_SUCCESS == status) {
            /**
             * input tensor shape
             *
             * old setup.bin only support batch_size = 1
             */
            tensor_shape_info_v1->shape_npu[0]                      = 1;                         // batch size
            tensor_shape_info_v1->shape_npu[1]                      = cnnh_520->input_channel;   // channel
            tensor_shape_info_v1->shape_npu[2]                      = cnnh_520->input_row;       // height
            tensor_shape_info_v1->shape_npu[3]                      = cnnh_520->input_col;       // width

            tensor_shape_info_v1->shape_onnx[0]                     = 1;                         // batch size
            tensor_shape_info_v1->shape_onnx[1]                     = cnnh_520->input_channel;   // channel
            tensor_shape_info_v1->shape_onnx[2]                     = cnnh_520->input_row;       // height
            tensor_shape_info_v1->shape_onnx[3]                     = cnnh_520->input_col;       // width

            tensor_shape_info_v1->axis_permutation_onnx_to_npu[0]   = 0;
            tensor_shape_info_v1->axis_permutation_onnx_to_npu[1]   = 1;
            tensor_shape_info_v1->axis_permutation_onnx_to_npu[2]   = 2;
            tensor_shape_info_v1->axis_permutation_onnx_to_npu[3]   = 3;

            /* quantization parameters */
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = *(int32_t *)&(cnnh_520->input_radix);
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
            quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = 1.0f;
        } else {
            goto FUNC_OUT;
        }
    }

    // construct tensor info (e.g. intput_nodes, output_nodes)
    while (setup_buff_offset < setup_node_boundary) {
        uintptr_t node_buff                                         = (uintptr_t)setup_buff + setup_buff_offset;
        uint32_t node_id                                            = *(uint32_t *)node_buff;
        uint32_t node_offset                                        = 0;
        kp_tensor_shape_info_v1_t *tensor_shape_info_v1             = NULL;
        kp_tensor_shape_info_t *tensor_shape_info                   = NULL;
        kp_tensor_descriptor_t *tensor_info                         = NULL;
        kp_quantization_parameters_v1_t *quantization_parameters_v1 = NULL;
        kp_quantization_parameters_t *quantization_parameters       = NULL;
        _out_node_t *out_node                                       = NULL;

        switch (node_id) {
        case NODE_TYPE_IN:
            // NPU IN Signal NODE
            dbg_print("current node is an NPU IN Signal NODE\n");
            node_offset = sizeof(_in_node_t);
            break;
        case NODE_TYPE_CPU:
            // CPU NODE
            dbg_print("current node is a CPU NODE\n");

            if (KP_MODEL_TARGET_CHIP_KL520 == single_model_descriptor->target)
                node_offset = sizeof(_cpu_node_t) - (2 * sizeof(_data_node_t*)) + (2 * (sizeof(_data_node_t) - sizeof(_super_node_t*) + sizeof(_super_node_t)));
            else if (KP_MODEL_TARGET_CHIP_KL720 == single_model_descriptor->target)
                node_offset = sizeof(_cpu_node_t) - (2 * sizeof(_data_node_t*));
            else {                                        // unknwon setup
                err_print("construct nef info in model_descriptor fail: invalid target %u ...\n", single_model_descriptor->target);
                status = KP_ERROR_INVALID_MODEL_21;
            }
            break;
        case NODE_TYPE_OUTPUT:
            // OUTPUT NODE
            dbg_print("current node is a output NODE\n");
            out_node    = (_out_node_t *)node_buff;
            node_offset = sizeof(_out_node_t) - (sizeof(_super_node_t*));

            tensor_info                                         = &(single_model_descriptor->output_nodes[out_node->output_index]);
            tensor_info->index                                  = out_node->output_index;
            tensor_info->name                                   = strcpy_dst_realloc(tensor_info->name, "");
            tensor_info->data_layout                            = convert_data_format_to_kp_tensor_format(out_node->data_format, single_model_descriptor->target);

            tensor_shape_info                                   = &(tensor_info->tensor_shape_info);
            tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;
    
            tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
            tensor_shape_info_v1->shape_npu_len                 = 4;
            tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
            tensor_shape_info_v1->shape_onnx_len                = 4;
            tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
            tensor_shape_info_v1->axis_permutation_len          = 4;
            tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

            quantization_parameters                                             = &(tensor_info->quantization_parameters);
            quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

            quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
            quantization_parameters_v1->quantized_axis                          = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
            quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

            status = is_tensor_info_reallocted(tensor_info);

            if (KP_SUCCESS == status) {
                /**
                 * output tensor shape
                 *
                 * old setup.bin only support batch_size = 1
                 */
                tensor_shape_info_v1->shape_npu[0]                      = 1;                    // batch size
                tensor_shape_info_v1->shape_npu[1]                      = out_node->ch_length;  // channel
                tensor_shape_info_v1->shape_npu[2]                      = out_node->row_length; // height
                tensor_shape_info_v1->shape_npu[3]                      = out_node->col_length; // width

                tensor_shape_info_v1->shape_onnx[0]                     = 1;                    // batch size
                tensor_shape_info_v1->shape_onnx[1]                     = out_node->ch_length;  // channel
                tensor_shape_info_v1->shape_onnx[2]                     = out_node->row_length; // height
                tensor_shape_info_v1->shape_onnx[3]                     = out_node->col_length; // width

                tensor_shape_info_v1->axis_permutation_onnx_to_npu[0]   = 0;
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[1]   = 1;
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[2]   = 2;
                tensor_shape_info_v1->axis_permutation_onnx_to_npu[3]   = 3;

                /* quantization parameters */
                quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = *(int32_t *)&(out_node->output_radix);
                quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
                quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = *(float *)&(out_node->output_scale);
            }
            break;
        case NODE_TYPE_INPUT:
            // NPU INPUT NODE
            dbg_print("current node is an network input NODE\n");
            if (KP_MODEL_TARGET_CHIP_KL720 == single_model_descriptor->target) { // kl720 input node
                _720_net_input_node_t *net_input_node   = (_720_net_input_node_t *)node_buff;
                node_offset                             = sizeof(_720_net_input_node_t);

                tensor_info                                         = &(single_model_descriptor->input_nodes[net_input_node->input_index]);
                tensor_info->index                                  = net_input_node->input_index;
                tensor_info->name                                   = strcpy_dst_realloc(tensor_info->name, "");
                tensor_info->data_layout                            = convert_data_format_to_kp_tensor_format(net_input_node->input_format, single_model_descriptor->target);

                tensor_shape_info                                   = &(tensor_info->tensor_shape_info);
                tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

                tensor_shape_info_v1                                = &(tensor_shape_info->tensor_shape_info_data.v1);
                tensor_shape_info_v1->shape_npu_len                 = 4;
                tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);
                tensor_shape_info_v1->shape_onnx_len                = 4;
                tensor_shape_info_v1->shape_onnx                    = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
                tensor_shape_info_v1->axis_permutation_len          = 4;
                tensor_shape_info_v1->axis_permutation_onnx_to_npu  = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);

                quantization_parameters                                             = &(tensor_info->quantization_parameters);
                quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;

                quantization_parameters_v1                                          = &(quantization_parameters->quantization_parameters_data.v1);
                quantization_parameters_v1->quantized_axis                          = 1;
                quantization_parameters_v1->quantized_fixed_point_descriptor_num    = 1;
                quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

                status = is_tensor_info_reallocted(tensor_info);

                if (KP_SUCCESS == status) {
                    /**
                     * input tensor shape
                     *
                     * old setup.bin only support batch_size = 1
                     */
                    tensor_shape_info_v1->shape_npu[0]                     = 1;                                // batch size
                    tensor_shape_info_v1->shape_npu[1]                     = net_input_node->input_channel;    // channel
                    tensor_shape_info_v1->shape_npu[2]                     = net_input_node->input_row;        // height
                    tensor_shape_info_v1->shape_npu[3]                     = net_input_node->input_col;        // width

                    tensor_shape_info_v1->shape_onnx[0]                    = 1;                                // batch size
                    tensor_shape_info_v1->shape_onnx[1]                    = net_input_node->input_channel;    // channel
                    tensor_shape_info_v1->shape_onnx[2]                    = net_input_node->input_row;        // height
                    tensor_shape_info_v1->shape_onnx[3]                    = net_input_node->input_col;        // width

                    tensor_shape_info_v1->axis_permutation_onnx_to_npu[0]  = 0;
                    tensor_shape_info_v1->axis_permutation_onnx_to_npu[1]  = 1;
                    tensor_shape_info_v1->axis_permutation_onnx_to_npu[2]  = 2;
                    tensor_shape_info_v1->axis_permutation_onnx_to_npu[3]  = 3;

                    /* quantization parameters */
                    quantization_parameters_v1->quantized_fixed_point_descriptor[0].radix               = *(int32_t *)&(net_input_node->input_radix);
                    quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale_dtype         = KP_DTYPE_FLOAT32;
                    quantization_parameters_v1->quantized_fixed_point_descriptor[0].scale.scale_float32 = 1.0f;
                }
            } else {                                        // unknwon setup
                err_print("construct nef info in model_descriptor fail: invalid target %u ...\n", single_model_descriptor->target);
                status = KP_ERROR_INVALID_MODEL_21;
            }
            break;
        case NODE_TYPE_DATA:
            // NPU DATA NODE
            dbg_print("current node is an network data NODE\n");
            node_offset = sizeof(_data_node_t) - sizeof(_super_node_t*);
            break;
        case NODE_TYPE_SUPER:
            // NPU SUPER NODE
            dbg_print("current node is an network super NODE\n");
            node_offset = sizeof(_super_node_t);
            break;
        default:
            // Unknown NODE
            err_print("unknown node type: %d\n", node_id);
            status = KP_ERROR_INVALID_MODEL_21;
        }

        if (KP_SUCCESS != status)
            goto FUNC_OUT;

        setup_buff_offset += node_offset;
    }

FUNC_OUT:

    return status;
}

/******************************************************************
 * setup flatbuff reader
 ******************************************************************/

int construct_single_setup_info_header_flatbuffer(KneronSetup_INFContent_table_t root, kp_single_model_descriptor_t *single_model_descriptor) {
    if (NULL == root ||
        NULL == single_model_descriptor) {
        err_print("construct nef single model information header in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    KneronSetup_Header_table_t header = KneronSetup_INFContent_header(root);

    if (NULL == header) {
        err_print("construct nef single model information header in model_descriptor fail: invalid flatbuffer ...\n");
        return KP_ERROR_INVALID_MODEL_21;
    }

    single_model_descriptor->target                                 = KneronSetup_Header_target(header);

    single_model_descriptor->setup_bin_schema_version.major         = KneronSetup_SchemaVersion_major_num(          KneronSetup_Header_schema_version(header));
    single_model_descriptor->setup_bin_schema_version.minor         = KneronSetup_SchemaVersion_minor_num(          KneronSetup_Header_schema_version(header));
    single_model_descriptor->setup_bin_schema_version.revision      = KneronSetup_SchemaVersion_revision_num(       KneronSetup_Header_schema_version(header));

    single_model_descriptor->file_schema_version.major              = KneronSetup_FileSchemaVersion_major_num(      KneronSetup_Header_file_schema_version(header));
    single_model_descriptor->file_schema_version.minor              = KneronSetup_FileSchemaVersion_minor_num(      KneronSetup_Header_file_schema_version(header));
    single_model_descriptor->file_schema_version.revision           = KneronSetup_FileSchemaVersion_revision_num(   KneronSetup_Header_file_schema_version(header));

    return KP_SUCCESS;
}

int construct_single_setup_info_quantization_parameters_flatbuffer(KneronSetup_QuantizationParameters_table_t quantization_parameters_flatbuffer, kp_quantization_parameters_t *quantization_parameters) {
    if (NULL == quantization_parameters_flatbuffer ||
        NULL == quantization_parameters) {
        err_print("construct nef single model information quantization parameters in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    KneronSetup_FxpInfo_vec_t fxp_info_vec_flatbuffer                   = KneronSetup_QuantizationParameters_fxp_info(quantization_parameters_flatbuffer);
    kp_quantization_parameters_v1_t *quantization_parameters_v1         = &(quantization_parameters->quantization_parameters_data.v1);

    quantization_parameters->version                                    = KP_MODEL_QUANTIZATION_PARAMS_VERSION_1;
    quantization_parameters_v1->quantized_axis                          = 1;
    quantization_parameters_v1->quantized_fixed_point_descriptor_num    = KneronSetup_FxpInfo_vec_len(fxp_info_vec_flatbuffer);
    quantization_parameters_v1->quantized_fixed_point_descriptor        = realloc_quantized_fixed_point_descriptor_list(quantization_parameters_v1->quantized_fixed_point_descriptor, quantization_parameters_v1->quantized_fixed_point_descriptor_num);

    if (0 < quantization_parameters_v1->quantized_fixed_point_descriptor_num &&
        NULL == quantization_parameters_v1->quantized_fixed_point_descriptor) {
        err_print("construct nef single model information quantization parameters in model_descriptor fail: alloc memory fail ...\n");
        return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    KneronSetup_FxpInfo_table_t fxp_info_flatbuffer;
    for (int i = 0; i < quantization_parameters_v1->quantized_fixed_point_descriptor_num; i++) {
        fxp_info_flatbuffer = KneronSetup_FxpInfo_vec_at(fxp_info_vec_flatbuffer, i);

        quantization_parameters_v1->quantized_fixed_point_descriptor[i].radix               = KneronSetup_FxpInfo_radix(fxp_info_flatbuffer);
        quantization_parameters_v1->quantized_fixed_point_descriptor[i].scale_dtype         = KP_DTYPE_FLOAT32;
        quantization_parameters_v1->quantized_fixed_point_descriptor[i].scale.scale_float32 = KneronSetup_FxpInfo_scale(fxp_info_flatbuffer);
    }

    return KP_SUCCESS;
}

int construct_single_setup_info_tensor_flatbuffer(KneronSetup_Tensor_table_t tensor, uint32_t target_chip, kp_tensor_descriptor_t *tensor_info) {
    if (NULL == tensor ||
        NULL == tensor_info) {
        err_print("construct nef single model information tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status                                          = KP_SUCCESS;

    tensor_info->name                                   = strcpy_dst_realloc(tensor_info->name, (char*)KneronSetup_Tensor_name(tensor));
    tensor_info->data_layout                            = convert_data_format_to_kp_tensor_format(KneronSetup_Tensor_format(tensor), target_chip);

    kp_tensor_shape_info_t* tensor_shape_info           = &(tensor_info->tensor_shape_info);
    tensor_shape_info->version                          = KP_MODEL_TENSOR_SHAPE_INFO_VERSION_1;

    kp_tensor_shape_info_v1_t *tensor_shape_info_v1     = &(tensor_shape_info->tensor_shape_info_data.v1);

    /* shape npu parse - malloc */
    flatbuffers_int32_vec_t shape_npu                   = KneronSetup_Tensor_shape(tensor);
    tensor_shape_info_v1->shape_npu_len                 = flatbuffers_int32_vec_len(shape_npu);
    tensor_shape_info_v1->shape_npu                     = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_npu_len);

    /* shape onnx parse - malloc */
    flatbuffers_int32_vec_t shape_onnx                  = KneronSetup_Tensor_raw_shape(tensor);
    tensor_shape_info_v1->shape_onnx_len                = flatbuffers_int32_vec_len(shape_onnx);

    if (0 < tensor_shape_info_v1->shape_onnx_len) {
        tensor_shape_info_v1->shape_onnx = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_onnx_len);
    } else {
        tensor_shape_info_v1->shape_onnx = realloc_tensor_shape_int32_t(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_npu_len);
    }

    /* shape interpreter parse - malloc */
    flatbuffers_int32_vec_t shape_intrp_dim             = KneronSetup_Tensor_shape_intrp_dim(tensor);
    tensor_shape_info_v1->axis_permutation_len          = flatbuffers_int32_vec_len(shape_intrp_dim);

    if (0 < tensor_shape_info_v1->axis_permutation_len) {
        tensor_shape_info_v1->axis_permutation_onnx_to_npu = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->axis_permutation_len);
    } else {
        tensor_shape_info_v1->axis_permutation_onnx_to_npu = realloc_tensor_shape_int32_t(tensor_shape_info_v1->axis_permutation_onnx_to_npu, tensor_shape_info_v1->shape_npu_len);
    }

    /* build quantization parameters */
    kp_quantization_parameters_t *quantization_parameters                           = &(tensor_info->quantization_parameters);
    KneronSetup_QuantizationParameters_table_t quantization_parameters_flatbuffer   = KneronSetup_Tensor_quantization(tensor);
    status = construct_single_setup_info_quantization_parameters_flatbuffer(quantization_parameters_flatbuffer, quantization_parameters);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = is_tensor_info_reallocted(tensor_info);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    /* shape npu parse - build */
    memcpy(tensor_shape_info_v1->shape_npu, shape_npu, tensor_shape_info_v1->shape_npu_len * flatbuffers_int32__size());

    /* shape onnx parse - build */
    if (0 < tensor_shape_info_v1->shape_onnx_len) {
        memcpy(tensor_shape_info_v1->shape_onnx, shape_onnx, tensor_shape_info_v1->shape_onnx_len * flatbuffers_int32__size());
    } else {
        tensor_shape_info_v1->shape_onnx_len = tensor_shape_info_v1->shape_npu_len;
        memcpy(tensor_shape_info_v1->shape_onnx, tensor_shape_info_v1->shape_npu, tensor_shape_info_v1->shape_onnx_len * flatbuffers_int32__size());
    }

    /* shape interprepter parse - build */
    if (0 < tensor_shape_info_v1->axis_permutation_len) {
        memcpy(tensor_shape_info_v1->axis_permutation_onnx_to_npu, shape_intrp_dim, tensor_shape_info_v1->axis_permutation_len * flatbuffers_int32__size());
    } else {
        tensor_shape_info_v1->axis_permutation_len = tensor_shape_info_v1->shape_npu_len;
        for (int axis_permutation_idx = 0; axis_permutation_idx < tensor_shape_info_v1->axis_permutation_len; axis_permutation_idx++) {
            tensor_shape_info_v1->axis_permutation_onnx_to_npu[axis_permutation_idx] = axis_permutation_idx;
        }
    }

FUNC_OUT:

    return status;
}

int construct_single_setup_info_inputs_tensor_flatbuffer(KneronSetup_INFContent_table_t root, kp_single_model_descriptor_t *single_model_descriptor) {
    if (NULL == root ||
        NULL == single_model_descriptor) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    KneronSetup_Tensor_vec_t tensor_vec = KneronSetup_INFContent_inputs(root);
    KneronSetup_Tensor_table_t tensor;
    kp_tensor_descriptor_t *tensor_info = NULL;

    if (NULL == tensor_vec) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: invalid flatbuffer ...\n");
        return KP_ERROR_INVALID_MODEL_21;
    }

    single_model_descriptor->input_nodes_num    = KneronSetup_Tensor_vec_len(tensor_vec);
    single_model_descriptor->input_nodes        = realloc_tensor_list(single_model_descriptor->input_nodes, single_model_descriptor->input_nodes_num);

    if (0 < single_model_descriptor->input_nodes_num &&
        NULL == single_model_descriptor->input_nodes) {
        err_print("construct nef single model information inputs tensor in model_descriptor fail: alloc memory fail ...\n");
        status = KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
        goto FUNC_OUT;
    }

    for (int i = 0; i < single_model_descriptor->input_nodes_num; i++) {
        tensor              = KneronSetup_Tensor_vec_at(tensor_vec, i);
        tensor_info         = &(single_model_descriptor->input_nodes[i]);

        tensor_info->index  = i;
        status              = construct_single_setup_info_tensor_flatbuffer(tensor, single_model_descriptor->target, tensor_info);

        if (KP_SUCCESS != status) {
            err_print("construct nef single model information inputs tensor in model_descriptor fail: constuct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}

int construct_single_setup_info_outputs_tensor_flatbuffer(KneronSetup_INFContent_table_t root, kp_single_model_descriptor_t *single_model_descriptor) {
    if (NULL == root ||
        NULL == single_model_descriptor) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    KneronSetup_Tensor_vec_t tensor_vec = KneronSetup_INFContent_outputs(root);

    if (NULL == tensor_vec) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: invalid flatbuffer ...\n");
        return KP_ERROR_INVALID_MODEL_21;
    }

    single_model_descriptor->output_nodes_num   = KneronSetup_Tensor_vec_len(tensor_vec);
    single_model_descriptor->output_nodes       = realloc_tensor_list(single_model_descriptor->output_nodes, single_model_descriptor->output_nodes_num);

    if (0 < single_model_descriptor->output_nodes_num &&
        NULL == single_model_descriptor->output_nodes) {
        err_print("construct nef single model information outputs tensor in model_descriptor fail: alloc memory fail ...\n");
        return KP_ERROR_MEMORY_ALLOCATION_FAILURE_9;
    }

    KneronSetup_Tensor_table_t tensor;
    kp_tensor_descriptor_t *tensor_info = NULL;
    for (int i = 0; i < single_model_descriptor->output_nodes_num; i++) {
        tensor              = KneronSetup_Tensor_vec_at(tensor_vec, i);
        tensor_info         = &(single_model_descriptor->output_nodes[i]);

        tensor_info->index  = i;
        status              = construct_single_setup_info_tensor_flatbuffer(tensor, single_model_descriptor->target, tensor_info);

        if (KP_SUCCESS != status) {
            err_print("construct nef single model information outputs tensor in model_descriptor fail: constuct tensor fail ...\n");
            goto FUNC_OUT;
        }
    }

FUNC_OUT:

    return status;
}

int construct_single_setup_info_flatbuffer(uintptr_t setup_buff, kp_single_model_descriptor_t *single_model_descriptor) {
    if (NULL == (void *)setup_buff ||
        NULL == single_model_descriptor) {
        err_print("construct nef single model information in model_descriptor fail: NULL pointer input parameters ...\n");
        return KP_ERROR_INVALID_PARAM_12;
    }

    int status = KP_SUCCESS;

    KneronSetup_INFContent_table_t root = KneronSetup_INFContent_as_root((void *)setup_buff);

    if (NULL == root) {
        err_print("construct nef single model information in model_descriptor fail: invalid model binary data ...\n");
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    if (SETUP_FLATBUFF_MAGIC_NUM != KneronSetup_INFContent_magic(root)) {
        err_print("construct nef single model information in model_descriptor fail: invalid setup flatbuffer magic number ... %d\n", KneronSetup_INFContent_magic(root));
        status = KP_ERROR_INVALID_MODEL_21;
        goto FUNC_OUT;
    }

    status = construct_single_setup_info_header_flatbuffer(root, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = construct_single_setup_info_inputs_tensor_flatbuffer(root, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

    status = construct_single_setup_info_outputs_tensor_flatbuffer(root, single_model_descriptor);
    if (KP_SUCCESS != status)
        goto FUNC_OUT;

FUNC_OUT:

    return status;
}

/******************************************************************
 * setup reader
 ******************************************************************/

int construct_single_setup_info(uintptr_t setup_buff, size_t setup_buff_size, kp_single_model_descriptor_t *single_model_descriptor) {
    int ret;

    if (SETUP_LEGACY_MAGIC_NUM == *((uint32_t*)setup_buff))
        ret = construct_single_setup_info_legacy(setup_buff, setup_buff_size, single_model_descriptor);
    else
        ret = construct_single_setup_info_flatbuffer(setup_buff, single_model_descriptor);

    return ret;
}
