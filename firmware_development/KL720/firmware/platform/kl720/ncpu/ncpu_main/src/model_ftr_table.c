/* --------------------------------------------------------------------------
 * Copyright (c) 2018-2022 Kneron Inc. All rights reserved.
 *
 *      Name:    model_ftr_table.c
 *      Purpose: Extend new features implementation
 *
 *---------------------------------------------------------------------------*/

#include "model_type.h"       /* for model ID */
#include "ipc.h"              /* for standalone function ID */
#include "model_ppp.h"        /* Kneron builtin model pre/post process functions */
#include "standalone.h"       /* Kneron builtin standalone feature functions */


/*********************************************************************************
                  model pre-processing features list
*********************************************************************************/
model_pre_post_func_t model_pre_proc_fns[MAX_MODEL_REGISTRATIONS] = {
    /* < model type ID >                             < pre-process function > */
    /* -------------------------------------------------------------------------- */
    { IMAGENET_CLASSIFICATION_MOBILENET_V2_224_224_3, pre_proc_classify_mobilenet_v2 },
    { KNERON_FD_SMALLBOX_200_200_3,                   pre_proc_fd_smallbox },
    { KNERON_FD_MBSSD_200_200_3,                      pre_proc_fd_ssd },
    { KNERON_FD_MASK_MBSSD_200_200_3,                 pre_proc_fd_ssd },
    { KNERON_LM_5PTS_ONET_56_56_3,                    pre_proc_lm_5pts },
    { KNERON_FR_MASK_RES50_112_112_3,                 pre_proc_fr_mask_vgg10  },
    { KNERON_FR_VGG10,                                pre_proc_fr_vgg10 },
    { KNERON_FR_RES50_112_112_3,                      pre_proc_fr_vgg10 },
    { KNERON_TINY_YOLO_PERSON_416_416_3,              pre_proc_tiny_yolo_person },
    { TINY_YOLO_VOC_224_224_3,                        pre_proc_tiny_yolo_voc },
    { TINY_YOLO_V3_416_416_3,                         pre_proc_tiny_yolo_v3 },
    { KNERON_OBJECTDETECTION_CENTERNET_512_512_3,     pre_proc_tiny_yolo_v3 },
    { YOLO_V3_416_416_3,                              pre_proc_tiny_yolo_v3 },
    { KNERON_LICENSE_OCR_MBNETv2_96_256_3,            pre_proc_licenseplate_ocr},
    { KNERON_YOLOXM_USLPR_96_256_3,                   pre_proc_licenseplate_ocr},
    { KNERON_CAM2CAM4_172_224,                        pre_proc_tof_d2c},

	{ KNERON_FDmask_ROTATE_fcos_288_416_3,            pre_proc_fd_ssd },
    { KNERON_LM_5PTS_ROTATE_ONETPLUS_80_80_3,         pre_proc_lm_5pts },
    { KNERON_DEPTH_LIVENESS_FeatherNetB_112_112_1,    pre_proc_kn_tof_liveness},
    { KNERON_FUSE_TOF_LIVENESS_DEP_NIR_112_112_1,     pre_proc_kn_tof_liveness},
    { KNERON_TOF_NIR_COMBO_LV,                        pre_proc_nir_combo_tof_liveness},
    { KNERON_TOF_FR,                                  pre_proc_fr_vgg10_tof},
    { KNERON_FACESEG_VGG10L_rotate_128_128_3,         pre_proc_face_seg },
    { KNERON_FACE_ROTATE_POSE_ONET_56_56_3,           pre_proc_face_pose },
    { KNERON_YOLOV705_LicensePlateRecognition_256_96_3,                     pre_proc_licenseplate_ocr },

    { KNERON_STEREONET_FE_480_640_3,                  pre_proc_stereo_depth_fe},
    { KNERON_STEREONET_FE_128_320_3,                  pre_proc_stereo_depth_fe},
    { KNERON_STEREONET_FE_128_384_3,                  pre_proc_stereo_depth_fe},
    /* Put customized pre-process functions below:
    { CUSTOMER_MODEL_1,                                 preproc_customer_model_1 },
    { CUSTOMER_MODEL_2,                                 preproc_customer_model_2 },
    { CUSTOMER_MODEL_3,                                 preproc_customer_model_3 },
    */
};

/*********************************************************************************
                  model post-processing features list
*********************************************************************************/
model_pre_post_func_t model_post_proc_fns[MAX_MODEL_REGISTRATIONS] = {
    /* < model type ID >                              < post-process function > */
    /* -------------------------------------------------------------------------- */
    { IMAGENET_CLASSIFICATION_RES50_224_224_3,        post_proc_classify_res50 },
    { IMAGENET_CLASSIFICATION_MOBILENET_V2_224_224_3, post_proc_classify_mobilenet_v2 },
    { KNERON_PERSONCLASSIFIER_MB_56_32_3,             post_proc_classify_mobilenet_v2 },
    { KNERON_PERSONCLASSIFIER_MB_56_48_3,			  post_proc_classify_mobilenet_v2 },
    { KNERON_FD_SMALLBOX_200_200_3,                   post_proc_fd_smallbox },
    { KNERON_FD_MBSSD_200_200_3,                      post_proc_fd_ssd },
    { KNERON_FD_MASK_MBSSD_200_200_3,                 post_proc_fd_ssd },
    { KNERON_FDmask_fcos_512_704_3,                   post_proc_fcos },
    { KNERON_LM_5PTS_ONET_56_56_3,                    post_proc_lm_5pts },
    { KNERON_LM_5PTS_ONETPLUS_56_56_3,                post_proc_onet_plus },
    { KNERON_FR_MASK_RES50_112_112_3,                 post_proc_fr_mask_fixfloat_output },
    { KNERON_FR_VGG10,                                post_proc_fr_vgg10 },
    { KNERON_FR_RES50_112_112_3,                      post_proc_fr_vgg10 },
    { KNERON_OBJECTDETECTION_CENTERNET_512_512_3,     post_proc_centernet },
    { KNERON_TINY_YOLO_PERSON_416_416_3,              post_proc_tiny_yolo_person },
    { TINY_YOLO_VOC_224_224_3,                        post_proc_tiny_yolo_voc },
    { TINY_YOLO_V3_416_416_3,                         post_proc_tiny_yolo_v3 },
    { TINY_YOLO_V3_608_608_3,                         post_proc_tiny_yolo_v3 },
    { YOLO_V3_416_416_3,                              post_proc_tiny_yolo_v3 },
    { YOLO_V3_608_608_3,                              post_proc_tiny_yolo_v3 },
    { YOLO_V4_416_416_3,                              post_proc_tiny_yolo_v3 },
    { KNERON_YOLOV5S_COCO80_480_640_3,                post_proc_yolov5s },
    { KNERON_YOLOV5S_COCO80_640_640_3,                post_proc_yolov5s },
    { KNERON_YOLOV5m_COCO80_640_640_3,                post_proc_yolov5s },
    { KNERON_PERSONDETECTION_YOLOV5s_480_256_3,       post_proc_yolov5s },
    { KNERON_PERSONDETECTION_YOLOV5sParklot_480_256_3,post_proc_yolov5s },
    { KNERON_YOLOV5_FACE_MASK_384_640_3,              post_proc_yolov5s },
    { KNERON_YOLOV5S_PersonBicycleCarMotorcycleBusTruckCatDog8_480_256_3,   post_proc_yolov5s },
    { KNERON_YOLOV5S_CarMotorcycleBusTruckPlate5_640_352_3,                 post_proc_yolov5s },
    { KNERON_YOLOV5S_PersonBottleChairPottedplant4_480_832_3,               post_proc_yolov5s },
    { KNERON_YOLOV705_PersonBicycleCarMotorcycleBusTruckHead7_512_288_3,    post_proc_yolov5s },
    { KNERON_LICENSE_OCR_MBNETv2_96_256_3,            post_proc_licenseplate_ocr},
    { KNERON_LICENSE_DETECT_YOLO5FACE_S_4PT_608_800_3,post_proc_yolo_face },
    { KNERON_YOLOXS_LANDMARK_USLPD_480_832_3 ,        post_proc_yolox },
    { KNERON_YOLOXM_USLPR_96_256_3 ,                  post_proc_yolox_for_ocr },
    { KNERON_CAM2CAM4_172_224,                        post_proc_tof_d2c},
    { KNERON_FDmask_ROTATE_fcos_288_416_3,            post_proc_fcos },
    { KNERON_LM_5PTS_ROTATE_ONETPLUS_80_80_3,         post_proc_onet_plus },
    { KNERON_FACE_ROTATE_POSE_ONET_56_56_3,           post_proc_face_pose },
    { KNERON_DEPTH_LIVENESS_FeatherNetB_112_112_1,    post_proc_depth_tof_liveness },
    { KNERON_FUSE_TOF_LIVENESS_DEP_NIR_112_112_1,     post_proc_fuse_tof_liveness },
    { KNERON_TOF_NIR_COMBO_LV,                        post_proc_nir_combo_tof_liveness },
    { KNERON_TOF_FR,                                  post_proc_fr_recognition},
    { KNERON_FACESEG_VGG10L_rotate_128_128_3,         post_proc_face_seg },
    { KNERON_RAFT_FRONT_144_256_3 ,                   post_proc_raft_front },
    { KNERON_RAFT_GRU_144_256_3 ,                     post_proc_raft_gru },
    { KNERON_RAFT_UP_144_256_3 ,                      post_proc_raft_up },
    { KNERON_YOLOV705_LicensePlateRecognition_256_96_3,                     post_proc_yolov5s_for_ocr },
    { KNERON_YOLOV7pose_tiny_LicensePlateDetection_640_640_3,               post_proc_yolov7_pose_2 },
    { KNERON_STEREONET_CC_32_96_128,                  post_proc_stereo_depth_cc },
    { KNERON_STEREONET_CA_32_80_48,                   post_proc_stereo_depth_ca },
    { KNERON_STEREONET_CA_32_96_48,                   post_proc_stereo_depth_ca },
    { KNERON_SEGMENTATION_PIDNETSCONV_FLOOR_INDOOR_UPSAMPLE_128_320_3,  post_proc_pidnet_segmentation },
    { KNERON_SEGMENTATION_PIDNETMCONV_6CLS_INDOOR_UPSAMPLE_360_640_3,   post_proc_pidnet_segmentation },
    { KNERON_SEGMENTATION_PIDNETSCONV_6CLS_INDOOR_UPSAMPLE_O180X320_360_640_3,  post_proc_pidnet_segmentation },
    { KNERON_SEGMENTATION_PIDNETSCONV1125_LEAKY_6CLS_UPSAMPLE_O216X384_SIGMOID_432_768_3,   post_proc_pidnet_segmentation },
    { KNERON_SEGMENTATION_PIDNETS_LEAKY_7CLS_UPSAMPLE_O300X400_600_800_3,   post_proc_pidnet_segmentation },
    /* Put customized post-process functions below:
    { CUSTOMER_MODEL_1,                                 post_customer_model_1 },
    { CUSTOMER_MODEL_2,                                 post_customer_model_2 },
    { CUSTOMER_MODEL_3,                                 post_customer_model_3 },
    */
};

/*********************************************************************************
                  Non-CNN/RNN related features
*********************************************************************************/

dsp_ftr_node_t  dsp_node_list[TOTAL_STANDALONE_MODULE] = {
    { CMD_CROP_RESIZE,                    standalone_crop_resize_process },
    { CMD_TOF_DECODE,                     standalone_tof_dec_process },
    { CMD_TOF_DECODE_DUAL,                standalone_tof_dec_dual_process },
    { CMD_TOF_CALC_IR_BRIGHT,             standalone_tof_ir_bright_process},
    { CMD_STEREO_DEPTH_FUSION,            standalone_stereo_depth_fusion_process},

#if ENABLE_JPEGLIB
    { CMD_JPEG_ENCODE,                    standalone_jpeg_enc_process },
    { CMD_JPEG_DECODE,                    standalone_jpeg_dec_process },
#endif
};

