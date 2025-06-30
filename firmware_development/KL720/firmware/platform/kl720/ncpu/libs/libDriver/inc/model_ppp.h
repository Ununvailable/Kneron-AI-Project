/**
 * @file      model_ppp.h
 * @brief     builtin model pre/post process functions
 * @copyright (c) 2020-2022 Kneron Inc. All right reserved.
 */

#ifndef __MODEL_PPP_H__
#define __MODEL_PPP_H__

#include "ncpu_gen_struct.h"

/*********************************************************************************
                  CNN pre-processing functions list
*********************************************************************************/

int pre_proc_classify_mobilenet_v2(struct kdp_image_s *pKdpImage);
int pre_proc_fd_smallbox(struct kdp_image_s *pKdpImage);
int pre_proc_fd_ssd(struct kdp_image_s *pKdpImage);
int pre_proc_lm_5pts(struct kdp_image_s *pKdpImage);
int pre_proc_fr_mask_vgg10(struct kdp_image_s *pKdpImage);
int pre_proc_fr_vgg10(struct kdp_image_s *pKdpImage);
int pre_proc_fr_vgg10_tof(struct kdp_image_s *pKdpImage);
int pre_proc_tiny_yolo_person(struct kdp_image_s *pKdpImage);
int pre_proc_tiny_yolo_voc(struct kdp_image_s *pKdpImage);
int pre_proc_tiny_yolo_v3(struct kdp_image_s *pKdpImage);
int pre_proc_licenseplate_ocr(struct kdp_image_s *pKdpImage);
int pre_proc_kn_tof_liveness(struct kdp_image_s *image_p);
int pre_proc_face_seg(struct kdp_image_s *pKdpImage);
int pre_proc_tof_d2c(struct kdp_image_s *pKdpImage);
int pre_proc_stereo_depth_fe(struct kdp_image_s *pKdpImage);
int pre_proc_nir_combo_tof_liveness(struct kdp_image_s *pKdpImage);
int pre_proc_face_pose(struct kdp_image_s *pKdpImage);
/*********************************************************************************
                  CNN post-processing functions list
*********************************************************************************/

int post_proc_classify_res50(struct kdp_image_s *pKdpImage);
int post_proc_classify_mobilenet_v2(struct kdp_image_s *pKdpImage);
int post_proc_fd_smallbox(struct kdp_image_s *pKdpImage);
int post_proc_fd_ssd(struct kdp_image_s *pKdpImage);
int post_proc_lm_5pts(struct kdp_image_s *pKdpImage);
int post_proc_onet_plus(struct kdp_image_s *pKdpImage);
int post_proc_fr_fixfloat_output(struct kdp_image_s *pKdpImage);
int post_proc_fr_mask_fixfloat_output(struct kdp_image_s *pKdpImage);
int post_proc_fr_vgg10(struct kdp_image_s *pKdpImage);
int post_proc_centernet(struct kdp_image_s *pKdpImage);
int post_proc_tiny_yolo_v3(struct kdp_image_s *pKdpImage);
int post_proc_tiny_yolo_voc(struct kdp_image_s *pKdpImage);
int post_proc_tiny_yolo_person(struct kdp_image_s *pKdpImage);
int post_proc_yolov5s(struct kdp_image_s *pKdpImage);
int post_proc_licenseplate_ocr(struct kdp_image_s *pKdpImage);
int post_proc_fcos(struct kdp_image_s *pKdpImage);
int post_proc_yolo_face(struct kdp_image_s *pKdpImage);
int post_proc_yolox(struct kdp_image_s *pKdpImage);
int post_proc_yolox_for_ocr(struct kdp_image_s *pKdpImage);
int post_proc_tof_d2c(struct kdp_image_s *pKdpImage);
int post_proc_face_pose(struct kdp_image_s *pKdpImage);
int post_proc_depth_tof_liveness(struct kdp_image_s *pKdpImage);
int post_proc_fuse_tof_liveness(struct kdp_image_s *pKdpImage);
int post_proc_nir_combo_tof_liveness(struct kdp_image_s *pKdpImage);
int post_proc_fr_recognition(struct kdp_image_s *pKdpImage);
int post_proc_face_seg(struct kdp_image_s *image_p);
int post_proc_raft_front(struct kdp_image_s *pKdpImage);
int post_proc_raft_gru(struct kdp_image_s *pKdpImage);
int post_proc_raft_up(struct kdp_image_s *pKdpImage);
int post_proc_yolov5s_for_ocr(struct kdp_image_s *pKdpImage);
int post_proc_yolov7_pose_2(struct kdp_image_s *pKdpImage);

int post_proc_stereo_depth_cc(struct kdp_image_s *pKdpImage);
int post_proc_stereo_depth_ca(struct kdp_image_s *pKdpImage);
int post_proc_pidnet_segmentation(struct kdp_image_s *pKdpImage);
#endif    /* __MODEL_PPP_H__ */

