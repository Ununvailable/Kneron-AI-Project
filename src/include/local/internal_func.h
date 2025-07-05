/**
 * @file        internal_func.h
 * @brief       
 * @version     0.1
 * @date        2021-03-22
 *
 * @copyright   Copyright (c) 2021 Kneron Inc. All rights reserved.
 */


#ifndef __INTERNAL_FUNC_H__
#define __INTERNAL_FUNC_H__

#include <stddef.h>
#include "kp_struct.h"

/******************************************************************
 * [private] model defined
 ******************************************************************/
#define MODEL_DESCRIPTOR_MAGIC_NUM 0x5AA55AA5   /**< magic number of kp_model_nef_descriptor_t */

/******************************************************************
 * [private] model enum defined
 ******************************************************************/

/**
 * setup.bin schema: kl520 npu data format
 */
enum
{
    DATA_FMT_KL520_UNKNOWN  = -1,
    DATA_FMT_KL520_4W4C8B   = 16,
    DATA_FMT_KL520_16W1C8B  = 8
};

/**
 * setup.bin schema: kl720 npu data format
 */
enum
{
    DATA_FMT_KL720_UNKNOWN      = -1,
    DATA_FMT_KL720_1W16C8B      = 0,
    DATA_FMT_KL720_1W16C8BHL    = 2,
    DATA_FMT_KL720_4W4C8B       = 4,
    DATA_FMT_KL720_16W1C8B      = 5,
    DATA_FMT_KL720_8W1C16B      = 6,
    DATA_FMT_KL720_RAW8         = 100,
    DATA_FMT_KL720_RAW16        = 101,
    DATA_FMT_KL720_RAW_FLOAT    = 102
};

/**
 * setup.bin schema: kl530 npu data format
 */
enum
{
    DATA_FMT_KL530_UNKNOWN      = -1,
    DATA_FMT_KL530_1W16C8B      = 0,
    DATA_FMT_KL530_1W16C8BHL    = 1,
    DATA_FMT_KL530_4W4C8B       = 2,
    DATA_FMT_KL530_4W4C8BHL     = 3,
    DATA_FMT_KL530_16W1C8B      = 4,
    DATA_FMT_KL530_16W1C8BHL    = 5,
    DATA_FMT_KL530_8W1C16B      = 6,
    DATA_FMT_KL530_RAW8         = 100,
    DATA_FMT_KL530_RAW16        = 101,
    DATA_FMT_KL530_RAW_FLOAT    = 102
};

/**
 * setup.bin schema: kl630 npu data format
 */
enum
{
    DATA_FMT_KL630_UNKNOWN      = -1,
    DATA_FMT_KL630_1W16C8B      = 0,
    DATA_FMT_KL630_1W16C8BHL    = 1,
    DATA_FMT_KL630_4W4C8B       = 2,
    DATA_FMT_KL630_4W4C8BHL     = 3,
    DATA_FMT_KL630_16W1C8B      = 4,
    DATA_FMT_KL630_16W1C8BHL    = 5,
    DATA_FMT_KL630_8W1C16B      = 6,
    DATA_FMT_KL630_RAW8         = 100,
    DATA_FMT_KL630_RAW16        = 101,
    DATA_FMT_KL630_RAW_FLOAT    = 102
};

/**
 * kl730 npu data format
 */
enum
{
    DATA_FMT_KL730_UNKNOWN              = -1,
    DATA_FMT_KL730_1W16C8B_CH_COMPACT   = 0,
    DATA_FMT_KL730_1W16C8BHL_CH_COMPACT = 1,
    DATA_FMT_KL730_4W4C8B               = 2,
    DATA_FMT_KL730_4W4C8BHL             = 3,
    DATA_FMT_KL730_16W1C8B              = 4,
    DATA_FMT_KL730_16W1C8BHL            = 5,
    DATA_FMT_KL730_8W1C16B              = 6,
    DATA_FMT_KL730_1W16C8B              = 8,
    DATA_FMT_KL730_1W16C8BHL            = 9,
    DATA_FMT_KL730_RAW8                 = 100,
    DATA_FMT_KL730_RAW16                = 101,
    DATA_FMT_KL730_RAW_FLOAT            = 102
};

/******************************************************************
 * [private] model information struct
 ******************************************************************/

/**
 * @brief metadata for nef model data: metadata / fw_info / all_models
 */
typedef struct
{
    char* platform;                             /**< usb dongle, 96 board, etc. */
    uint32_t target;                            /**< 0: KL520, 1: KL720, etc. */
    uint32_t crc;                               /**< CRC value for all_models data */
    uint32_t kn_num;                            /**< KN number */
    uint32_t enc_type;                          /**< encrypt type */
    char* tc_ver;                               /**< toolchain version */
    char* compiler_ver;                         /**< compiler version */
    kp_nef_schema_version_t nef_schema_version; /**< nef schema version */
} kp_metadata_t;

/**
 * @brief nef info for nef model data: metadata / fw_info / all_models
 */
typedef struct
{
    uint32_t target;                            /**< 0: KL520, 1: KL720, etc. */
    char *fw_info_addr;                         /**< Address of fw_info part */
    uint32_t fw_info_size;                      /**< Size of fw_info part */
    char *all_models_addr;                      /**< Address of all_model part */
    uint32_t all_models_size;                   /**< Size of all_model part */
} kp_nef_info_t;

/**
 * @brief nef model info list
 */
typedef struct
{
    uint32_t model_num;                         /**< number of model */
    uintptr_t model_info_list;                  /**< model information handler list */
} kp_nef_model_info_list_t;

/**
 * @brief nef model info list
 */
typedef struct
{
    uint32_t id;                                /**< model id */
    char *name;                                 /**< model name */
    char *version;                              /**< model version */
} kp_nef_model_info_t;

/**
 * @brief metadata for nef model data: metadata / fw_info / all_models
 */
typedef struct
{
    uint32_t target;                            /**< 0: KL520, 1: KL720, etc. */
    uintptr_t kne_header;                       /**< kne header */
    uintptr_t kne_model_vec;                    /**< kne model list */
} kp_kne_info_t;

/**
 * @brief nef content table handler
 */
typedef uintptr_t kp_nef_handler_t;

/******************************************************************
 * [private] utils
 ******************************************************************/

void* realloc_zero(void* memory, size_t new_size);
void* strcpy_dst_realloc(char* dst_buff, const char* src_buff);

/******************************************************************
 * [private] kneron model utils
 ******************************************************************/

int is_tensor_info_reallocted(kp_tensor_descriptor_t* tensor);
uint32_t convert_data_format_to_kp_tensor_format(uint32_t data_format, uint32_t target_chip);

/******************************************************************
 * [private] setup_reader
 ******************************************************************/

int construct_single_setup_info(uintptr_t setup_buff, size_t setup_buff_size, kp_single_model_descriptor_t *single_model_descriptor);

/******************************************************************
 * [private] kne_reader
 ******************************************************************/

int read_kne(uintptr_t kne_data, kp_kne_info_t *kne_info);
int get_kne_single_model_output_buffer_size(uintptr_t kne_model_vec_ptr, uint32_t model_id, size_t *output_buffer_size);
int construct_kne_models_info(uintptr_t kne_model_vec_ptr, kp_model_nef_descriptor_t* loaded_model_desc);

/******************************************************************
 * [private] model_descriptor_builder
 ******************************************************************/

kp_single_model_descriptor_t* realloc_model_descriptor_list(kp_single_model_descriptor_t *model_descriptor_list, uint32_t element_num);
uint32_t* realloc_tensor_shape(uint32_t *shape, uint32_t element_num);
uint32_t* realloc_tensor_shape_uint32_t(uint32_t *shape, uint32_t element_num);
int32_t* realloc_tensor_shape_int32_t(int32_t *shape, uint32_t element_num);
kp_tensor_descriptor_t* realloc_tensor_list(kp_tensor_descriptor_t *tensor_list, uint32_t element_num);
kp_quantized_fixed_point_descriptor_t* realloc_quantized_fixed_point_descriptor_list(kp_quantized_fixed_point_descriptor_t *quantized_fixed_point_descriptor_list, uint32_t element_num);

/******************************************************************
 * [public] setup_reader
 ******************************************************************/

uint32_t convert_data_format_to_kp_tensor_format(uint32_t data_format, uint32_t target_chip);

/******************************************************************
 * [public] kneron_nef_reader
 ******************************************************************/

int read_nef_content_table(char* nef_data, uint32_t nef_size, kp_nef_handler_t *nef_handler);
int read_nef_header_information(kp_nef_handler_t *nef_handler, kp_metadata_t *metadata);
int read_nef_model_info_list(kp_nef_handler_t *nef_handler, kp_nef_model_info_list_t *model_info_handler);
int read_nef_model_info(kp_nef_model_info_list_t *model_info_handler, uint32_t index, kp_nef_model_info_t *model_info);
int read_nef_model_binary_info(kp_nef_handler_t *nef_handler, kp_metadata_t *metadata, kp_nef_info_t *nef_info);

/******************************************************************
 * [public] model_descriptor_builder
 ******************************************************************/

int deconstruct_tensor_descriptor(kp_tensor_descriptor_t* tensor_descriptor);
int deconstruct_model_nef_descriptor(kp_model_nef_descriptor_t* loaded_model_desc);
int build_model_nef_descriptor_from_nef(kp_nef_handler_t *nef_handler, kp_metadata_t *metadata, kp_nef_info_t *nef_info, kp_model_nef_descriptor_t* loaded_model_desc);
int build_model_nef_descriptor_from_device(kp_nef_handler_t *nef_handler, kp_nef_info_t *nef_info, kp_model_nef_descriptor_t* loaded_model_desc);
int load_model_info_from_nef(void *nef_buf, int nef_size, kp_product_id_t target_pid /* input */, kp_metadata_t *metadata, kp_nef_info_t *nef_info, kp_model_nef_descriptor_t *loaded_model_desc /* output */);

/******************************************************************
 * [public] model_descriptor_copier
 ******************************************************************/

int copy_single_tensor_info_quantization_parameters(kp_quantization_parameters_t *quantization_parameters_dst, kp_quantization_parameters_t *quantization_parameters_src);

/******************************************************************
 * [public] model_descriptor_copier
 ******************************************************************/

int copy_model_nef_descriptor(kp_model_nef_descriptor_t *loaded_model_desc_dst /* output */, kp_model_nef_descriptor_t *loaded_model_desc_src);

#endif
