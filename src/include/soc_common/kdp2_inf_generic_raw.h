#pragma once

#include "kp_internal.h"

#define KDP2_INF_ID_GENERIC_RAW 10
#define KDP2_INF_ID_GENERIC_RAW_BYPASS_PRE_PROC 17

// FIXME ?
// Parsing KL720 raw output
// Copied from beethoven_sw\firmware\platform\kl720\common\model_res.h
typedef struct
{
    uint32_t start_offset;
    uint32_t buf_len;
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
} _720_raw_onode_t;

typedef struct
{
    uint32_t total_raw_len;
    int32_t total_nodes;
    _720_raw_onode_t onode_a[40];
    uint8_t data[];
} _720_raw_cnn_res_t;

typedef struct
{
    uint32_t idx;  //sequence number of out sub-nodes
    uint32_t fmt;
    uint32_t batch;
    uint32_t ch_length;
    uint32_t row_length;
    uint32_t col_length;
    uint32_t buf_addr;
    uint32_t buf_len;
    uint32_t scale;
    uint32_t radix;
    uint32_t start_offset;
    uint32_t buf_aligned_len;
    uint32_t quant_vect_len;
} _630_raw_onode_t;

typedef struct
{
    uint32_t total_raw_len;
    int32_t total_nodes;
    _630_raw_onode_t onode_a[40];
    uint8_t data[];
} _630_raw_cnn_res_t;

typedef struct
{
    uint32_t idx;  //sequence number of out sub-nodes
    uint32_t fmt;
    uint32_t batch;
    uint32_t ch_length;
    uint32_t row_length;
    uint32_t col_length;
    uint64_t buf_addr;      //KL730: 64bit ARM
    uint32_t buf_len;
    uint32_t scale;
    uint32_t radix;
    uint32_t start_offset;
    uint32_t buf_aligned_len;
    uint32_t quant_vect_len;
} _730_raw_onode_t;

typedef struct
{
    uint32_t total_raw_len;
    int32_t total_nodes;
    _730_raw_onode_t onode_a[40];
    uint8_t data[];
} _730_raw_cnn_res_t;

typedef struct
{
    uint32_t idx;  //sequence number of out sub-nodes
    uint32_t fmt;
    uint32_t batch;
    uint32_t ch_length;
    uint32_t row_length;
    uint32_t col_length;
    uint64_t buf_addr;      //KL830: 64bit ARM
    uint32_t buf_len;
    uint32_t scale;
    uint32_t radix;
    uint32_t start_offset;
    uint32_t buf_aligned_len;
    uint32_t quant_vect_len;
} _830_raw_onode_t;

typedef struct
{
    uint32_t total_raw_len;
    int32_t total_nodes;
    _830_raw_onode_t onode_a[40];
    uint8_t data[];
} _830_raw_cnn_res_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t resize_mode;
    uint32_t padding_mode;
    uint32_t image_format;
    uint32_t normalize_mode;
    uint32_t crop_count;
    kp_inf_crop_box_t inf_crop[MAX_CROP_BOX];
} __attribute__((aligned(4))) kdp2_ipc_generic_raw_inf_image_header_t;

/**
 * @brief schema version version for NPU data representation.
 */
typedef enum
{
    NPU_DATA_SCHEMA_VERSION_UNKNOWN = 0,                            /**< unknow version */
    NPU_DATA_SCHEMA_VERSION_1       = 1,                            /**< version 1 - for KL520, KL720 and KL630 */
    NPU_DATA_SCHEMA_VERSION_2       = 2                             /**< version 2 - for KL730 */
} npu_data_schema_version_t;

/**
 * @brief a basic descriptor for a NPU data access information of tensor (version 1)
 */
typedef struct {
    uint32_t                        index;                          /**< index of node */
    uint32_t                        name_len;                       /**< length of node name */
    uint32_t                        name_start_offset;              /**< base address offset of node name */
    uint32_t                        data_layout;                    /**< npu memory layout */

    uint32_t                        shape_npu_len;                  /**< length of NPU shape (Default value: 4) */
    int32_t                         shape_npu_data_type;            /**< data type of NPU shape (Default dimension order: BxCxHxW) */
    uint32_t                        shape_npu_start_offset;         /**< base address offset of NPU shape (Default dimension order: BxCxHxW) */
    uint32_t                        shape_onnx_len;                 /**< length of ONNX shape */
    int32_t                         shape_onnx_data_type;           /**< data type of ONNX shape */
    uint32_t                        shape_onnx_start_offset;        /**< base address offset of ONNX shape */
    uint32_t                        axis_permutation_len;           /**< length of remap axis permutation from onnx to npu shape (shape_intrp_dim) */
    int32_t                         axis_permutation_data_type;     /**< data type of remap axis permutation from onnx to npu shape (shape_intrp_dim) */
    uint32_t                        axis_permutation_start_offset;  /**< base address offset of remap axis permutation from onnx to npu shape (shape_intrp_dim) */

    uint32_t                        quantized_axis;                 /**< the axis along which the fixed-point quantization information performed */
    uint32_t                        quantized_parameters_len;       /**< numbers of fixed-point quantization information */
    int32_t                         radix_data_type;                /**< radix data type */
    uint32_t                        radix_start_offset;             /**< base address offset of radix data */
    int32_t                         scale_data_type;                /**< scale data type */
    uint32_t                        scale_start_offset;             /**< base address offset of scale data */

    uint32_t                        npu_data_len;                   /**< length of NPU tensor raw data (in bytes) */
    uint32_t                        npu_data_start_offset;          /**< base address offset of NPU raw data */
} npu_data_single_node_header_v1_t;

/**
 * @brief a basic descriptor for a NPU data access information of tensor (version 2)
 */
typedef struct {
    uint32_t                        index;                          /**< index of node */
    uint32_t                        name_len;                       /**< length of node name */
    uint32_t                        name_start_offset;              /**< base address offset of node name */
    uint32_t                        data_layout;                    /**< npu memory layout */

    uint32_t                        shape_len;                      /**< length of shape */
    int32_t                         shape_data_type;                /**< data type of shape */
    uint32_t                        shape_start_offset;             /**< base address offset of shape */
    int32_t                         stride_onnx_data_type;          /**< data type of ONNX data access stride (in scalar) */
    uint32_t                        stride_onnx_start_offset;       /**< base address offset of ONNX data access stride (in scalar) */
    int32_t                         stride_npu_data_type;           /**< data type of NPU data access stride (in scalar) */
    uint32_t                        stride_npu_start_offset;        /**< base address offset of NPU data access stride (in scalar) */

    uint32_t                        quantized_axis;                 /**< the axis along which the fixed-point quantization information performed */
    uint32_t                        quantized_parameters_len;       /**< numbers of fixed-point quantization information */
    int32_t                         radix_data_type;                /**< radix data type */
    uint32_t                        radix_start_offset;             /**< base address offset of radix data */
    int32_t                         scale_data_type;                /**< scale data type */
    uint32_t                        scale_start_offset;             /**< base address offset of scale data */

    uint32_t                        npu_data_len;                   /**< length of NPU tensor raw data (in bytes) */
    uint32_t                        npu_data_start_offset;          /**< base address offset of NPU raw data */
} npu_data_single_node_header_v2_t;

/**
 * @brief a basic descriptor for a NPU data access information of tensors
 */
typedef struct {
    uint32_t                        npu_data_schema_version;        /**< NPU data schema version (ref. npu_data_schema_version_t) */
    uint32_t                        data_size;                      /**< total size of NPU raw data information (in bytes) */
    uint32_t                        npu_data_node_num;              /**< number of NPU raw data entry */
    uint8_t                         data[];                         /**< data buffer for store npu_data_single_node_header_t and NPU raw data (|npu_data_single_node_header_t[0]|npu_data_single_node_header_t[1]|...|npu_data_single_node_header_t[N]|raw_data ...|) */
} npu_data_header_t;

// input header for 'Generic RAW inference'
typedef struct
{
    /* header stamp is necessary for data transfer between host and device */
    kp_inference_header_stamp_t header_stamp;
    uint32_t inference_number;
    uint32_t model_id;
    kdp2_ipc_generic_raw_inf_image_header_t image_header;
} __attribute__((aligned(4))) kdp2_ipc_generic_raw_inf_header_t;

// result header for 'Generic RAW inference'
typedef struct
{
    /* header stamp is necessary for data transfer between host and device */
    kp_inference_header_stamp_t header_stamp;
    uint32_t num_of_pre_proc_info;
    kp_hw_pre_proc_info_t pre_proc_info[KP_MAX_INPUT_NODE_COUNT_V1];
    uint32_t product_id;   // enum kp_product_id_t.
    uint32_t inf_number;
    uint32_t crop_number;
    uint32_t is_last_crop; // 0: not last crop box, 1: last crop box
    uint8_t raw_data[];    // just imply following raw output data
} __attribute__((aligned(4))) kdp2_ipc_generic_raw_result_t;

typedef kdp2_ipc_generic_raw_result_t kdp2_ipc_generic_raw_result_t_v1;

typedef struct
{
    /* header stamp is necessary for data transfer between host and device */
    kp_inference_header_stamp_t header_stamp;
    uint32_t product_id;   // enum kp_product_id_t.
    uint32_t inf_number;
    uint32_t crop_number;
    uint32_t is_last_crop; // 0: not last crop box, 1: last crop box
    uint32_t num_of_pre_proc_info;
    uint32_t pre_proc_info_offset;
    uint32_t raw_data_offset;
    uint8_t mix_data[];    // just imply following raw output data
} __attribute__((aligned(4))) kdp2_ipc_generic_raw_result_t_v2;

// input header for 'Generic RAW inference Bypass Pre-Process'
typedef struct
{
    /* header stamp is necessary for data transfer between host and device */
    kp_inference_header_stamp_t header_stamp;
    uint32_t inference_number;
    uint32_t model_id;
    uint32_t image_buffer_size;
} __attribute__((aligned(4))) kdp2_ipc_generic_raw_inf_bypass_pre_proc_header_t;

// result header for 'Generic RAW inference Bypass Pre-Process'
typedef kdp2_ipc_generic_raw_result_t kdp2_ipc_generic_raw_bypass_pre_proc_result_t;

typedef kdp2_ipc_generic_raw_result_t_v2 kdp2_ipc_generic_raw_bypass_pre_proc_result_t_v2;
