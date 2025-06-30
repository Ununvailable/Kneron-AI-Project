#pragma once

#include <stdint.h>

typedef struct
{
    // pre-proc config
    struct {
        uint32_t x;
        uint32_t y;
    } landmark[4];                  /**< license plate corner landmark */

    bool enable_hw_inproc;          /**< to enable hw inproc or run warp perspective transform instead */

    // post-proc config
    bool is_maxpool_in_model;       /**< is maxpool layer in ocr model */
    uint32_t ocr_rule_base_code;    /**< ocr rule base code for ocr correction (ref: enum OCR_RULE_BASE_CODE) */
    uint32_t max_ocr_number;        /**< max number of detected characters  */
    float ocr_score_threshold;      /**< score threshold for ocr character filter  */
    float ocr_iou_threshold;        /**< IoU threshold for ocr character bounding box filter  */
} ocr_pre_post_proc_config_t;

typedef struct
{
    // post-proc config
    float prob_threshold;           /**< probability threshold */
    float iou_threshold;            /**< IoU threshold */
} fcos_det_pre_post_proc_config_t;

typedef struct
{
    // post-proc config
    bool is_raft_gru_loop_end;      /**< raft gru model loop end flag */

    int raft_input_flow_radix;      /**< raft gru model input node flow radix */
    float raft_input_flow_scale;    /**< raft gru model input node flow scale */

    int raft_input_corr_radix;      /**< raft gru model input node corr radix */
    float raft_input_corr_scale;    /**< raft gru model input node corr radix */
} raft_pre_post_proc_config_t;
typedef struct
{
    int32_t class_map[7]; // update it if class number change
    bool enable_simd;
    uint32_t pad_width;
    uint32_t depthmap_width;
    
} pidnet_segmentation_post_proc_config_t;

typedef struct calibrate_parameter_s {
    float a[3];
    float b[3];
    float c[2];
    int x_off;
    uint32_t preproc_result_addr;
} calibrate_parameter;

typedef struct
{
    calibrate_parameter cali_data;
    int idx; /* to speficy the input image is left image or right image*/
} stereo_depth_fe_pre_proc_config_t;

typedef struct
{
    bool enable_simd;
    uint32_t temp_buffer_addr;
    float matrix_q[16];
    float focal_length;
    float baseline;
    float d_offset;
    float d_scale;
    float crop_width;
    float crop_height;
    float quantization_offset;
    float dividend;
    float divisor;

} stereo_depth_post_proc_config_t;
