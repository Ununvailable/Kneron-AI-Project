#pragma once

#include "kp_internal.h"

/**
 * @brief Metadata of RAW node output in fixed-point format
 */
typedef struct
{
    uint32_t height;                                            /**< node height */
    uint32_t channel;                                           /**< node channel */
    uint32_t width;                                             /**< node width, should be aligned to 16 bytes for futher processing due to low level output */
    int32_t radix;                                              /**< radix for fixed/floating point conversion */
    float scale;                                                /**< scale for fixed/floating point conversion */
    uint32_t data_layout;                                       /**< npu memory layout (ref. kp_model_tensor_data_layout_t) */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_node_metadata_t;

/**
 * @brief Inference debug ipc data structure represents for "after-inference" version 1
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t num_nodes;                                         /**< number of output nodes */
    dbg_ipc_checkpoint_data_node_metadata_t node_metadata[50];  /**< output node metada */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint8_t raw_output[];                                       /**< truly raw output from NPU */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_after_inference_t;

/**
 * @brief Inference debug ipc data structure represents for "after-inference" version 2
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint32_t num_nodes;                                         /**< number of output nodes */
    uint8_t data[];                                             /**< meta data and raw output data start */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_after_inference_t_v2;

/**
 * @brief Inference debug ipc data structure represents for "before-cpu operation" version 1
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t num_nodes;                                         /**< number of output nodes */
    dbg_ipc_checkpoint_data_node_metadata_t node_metadata[50];  /**< output node metada */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint8_t raw_output[];                                       /**< truly raw output from NPU */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_before_cpu_op_t;

/**
 * @brief Inference debug ipc data structure represents for "before-cpu operation" version 2
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint32_t num_nodes;                                         /**< number of output nodes */
    uint8_t data[];                                             /**< meta data and raw output data start */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_before_cpu_op_t_v2;

/**
 * @brief Inference debug ipc data structure represents for "after-cpu operation" version 1
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t num_nodes;                                         /**< number of output nodes */
    dbg_ipc_checkpoint_data_node_metadata_t node_metadata[50];  /**< output node metada */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint8_t raw_output[];                                       /**< truly raw output from NPU */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_after_cpu_op_t;

/**
 * @brief Inference debug ipc data structure represents for "after-cpu operation" version 2
 */
typedef struct
{
    kp_inference_header_stamp_t header_stamp;                   /**< magic_type = 'KDP2_MAGIC_TYPE_CHECKPOINT_DATA' */
    uint32_t checkpoint_tag;                                    /**< refer to kp_dbg_checkpoint_flag_t */
    int target_inf_model;                                       /**< inferencing model */
    uint32_t total_output_size;                                 /**< total raw output size in bytes */
    uint32_t num_nodes;                                         /**< number of output nodes */
    uint8_t data[];                                             /**< meta data and raw output data start */
} __attribute__((aligned(4))) dbg_ipc_checkpoint_data_after_cpu_op_t_v2;
