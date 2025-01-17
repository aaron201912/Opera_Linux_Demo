/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef _SSTAR_DRM_H_
#define _SSTAR_DRM_H_

#ifdef __KERNEL__
#include <drm/drm.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_ioctl.h>
#else
#include "drm.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************Modifier**************************************************/
#define SSTAR_MOP_TILE_MODE_SHIFT_BASE 0
#define SSTAR_MOP_TILE_MODE_MASK (0xff << SSTAR_MOP_TILE_MODE_SHIFT_BASE)
#define SSTAR_MOP_TILE_MODE_SHIFT(bit) \
    (SSTAR_MOP_TILE_MODE_MASK & (1ULL << (SSTAR_MOP_TILE_MODE_SHIFT_BASE + (bit))))
#define SSTAR_MOP_TILE_MODE_16x16 SSTAR_MOP_TILE_MODE_SHIFT(0)
#define SSTAR_MOP_TILE_MODE_16x32 SSTAR_MOP_TILE_MODE_SHIFT(1)
#define SSTAR_MOP_TILE_MODE_32x16 SSTAR_MOP_TILE_MODE_SHIFT(2)
#define SSTAR_MOP_TILE_MODE_32x32 SSTAR_MOP_TILE_MODE_SHIFT(3)

#define SSTAR_MOP_COMPRESS_MODE_SHIFT_BASE 8
#define SSTAR_MOP_COMPRESS_MODE_MASK (0xffff << SSTAR_MOP_COMPRESS_MODE_SHIFT_BASE)
#define SSTAR_MOP_COMPRESS_MODE_SHIFT(bit) \
    (SSTAR_MOP_COMPRESS_MODE_MASK & (1ULL << (SSTAR_MOP_COMPRESS_MODE_SHIFT_BASE + (bit))))
#define SSTAR_MOP_COMPRESS_MODE_SEG SSTAR_MOP_COMPRESS_MODE_SHIFT(0)
#define SSTAR_MOP_COMPRESS_MODE_LINE SSTAR_MOP_COMPRESS_MODE_SHIFT(1)
#define SSTAR_MOP_COMPRESS_MODE_FRAME SSTAR_MOP_COMPRESS_MODE_SHIFT(2)
#define SSTAR_MOP_COMPRESS_MODE_TO_8BIT SSTAR_MOP_COMPRESS_MODE_SHIFT(3)
#define SSTAR_MOP_COMPRESS_MODE_TO_6BIT SSTAR_MOP_COMPRESS_MODE_SHIFT(4)
#define SSTAR_MOP_COMPRESS_MODE_IFC SSTAR_MOP_COMPRESS_MODE_SHIFT(5)
#define SSTAR_MOP_COMPRESS_MODE_SFBC0 SSTAR_MOP_COMPRESS_MODE_SHIFT(6)
#define SSTAR_MOP_COMPRESS_MODE_SFBC1 SSTAR_MOP_COMPRESS_MODE_SHIFT(7)
#define SSTAR_MOP_COMPRESS_MODE_SFBC2 SSTAR_MOP_COMPRESS_MODE_SHIFT(8)
#define SSTAR_MOP_COMPRESS_MODE_BUTT SSTAR_MOP_COMPRESS_MODE_SHIFT(9)

#define DRM_FORMAT_MOD_VENDOR_SSTAR 0x0b
#define DRM_FORMAT_MOD_TYPE_SSTAR_MOP 0x00
#define DRM_FORMAT_MOD_SSTAR_CODE(__type, __val) \
    fourcc_mod_code(SSTAR, ((__u64)(__type) << 52) | ((__val)&0x000fffffffffffffULL))
#define DRM_FORMAT_MOD_SSTAR_MOP(__mode) \
    DRM_FORMAT_MOD_SSTAR_CODE(DRM_FORMAT_MOD_TYPE_SSTAR_MOP, __mode)
/*************************************************************************************************************/

/****************************************************IOCTL****************************************************/
#define DRM_SSTAR_UPDATE_PLANE 0x00
#define DRM_SSTAR_SPREAD_SPECTURM 0x01
#define DRM_SSTAR_SET_WHITE_BALANCE 0x02
#define DRM_SSTAR_PICTURE_QUALITY 0x03
#define DRM_SSTAR_SET_PQ 0x04
#define DRM_SSTAR_SET_DLC_ACTIVE 0x05
#define DRM_SSTAR_GET_LUMA 0x06
#define DRM_SSTAR_GET_GAMMA_CURVE 0x07
#define DRM_SSTAR_SET_GAMMA_CURVE 0x08
#define DRM_SSTAR_PQ_ALGO_PARAM 0x09
#define DRM_SSTAR_GET_HSY_CURVE_NUM 0x0A
#define DRM_SSTAR_SET_HSY_PARAM 0x0B
/* ioctl for PQ tool, cmd start from 0x5D and decreasing */
#define DRM_SSTAR_TOOL_GET_DLC_INFO 0x5E

#define DRM_SSTAR_END 0x5F

#define DRM_IOCTL_SSTAR_UPDATE_PLANE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_UPDATE_PLANE, struct drm_sstar_update_plane)
#define DRM_IOCTL_SSTAR_SET_SPREAD_SPECTRUM \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SPREAD_SPECTURM, struct drm_sstar_set_spread_spectrum)
#define DRM_IOCTL_SSTAR_SET_WHITE_BALANCE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SET_WHITE_BALANCE, struct drm_sstar_set_white_balance)
#define DRM_IOCTL_SSTAR_PICTURE_QUALITY \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_PICTURE_QUALITY, struct drm_sstar_picture_quality)
#define DRM_IOCTL_SSTAR_SET_PQ \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SET_PQ, struct drm_sstar_set_pq)
#define DRM_IOCTL_SSTAR_SET_DLC_ACTIVE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SET_DLC_ACTIVE, struct drm_sstar_set_dlc_active)
#define DRM_IOCTL_SSTAR_GET_LUMA \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_GET_LUMA, struct drm_sstar_get_luma)
#define DRM_IOCTL_SSTAR_GET_GAMMA_CURVE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_GET_GAMMA_CURVE, struct drm_sstar_get_gamma_curve)
#define DRM_IOCTL_SSTAR_SET_GAMMA_CURVE \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SET_GAMMA_CURVE, struct drm_sstar_set_gamma_curve)
#define DRM_IOCTL_SSTAR_PQ_ALGO_PARAM \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_PQ_ALGO_PARAM, struct drm_sstar_pq_algo_param)
#define DRM_IOCTL_SSTAR_GET_HSY_CURVE_NUM \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_GET_HSY_CURVE_NUM, struct drm_sstar_get_hsy_curve_num)
#define DRM_IOCTL_SSTAR_SET_HSY_PARAM \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_SET_HSY_PARAM, struct drm_sstar_set_hsy_param)
/* ioctl for PQ tool */
#define DRM_IOCTL_SSTAR_TOOL_GET_DLC_INFO \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_SSTAR_TOOL_GET_DLC_INFO, struct drm_sstar_tool_get_dlc_info)

#define DLC_CURVE_NUM 16

struct drm_sstar_update_plane {
    uint32_t planeId;
    uint32_t fbId; /* Use drm frabebuffer if (fbId > 0) */
    /* Otherwise, use the following parameter */
    uint32_t format;
    uint16_t width;
    uint16_t height;
    uint64_t modifier;
    uint64_t phyAddr[3];
    uint32_t pitches[4]; /* stride */
    uint32_t offsets[4];

    uint64_t fence; /* Out fence ptr */
};

struct drm_sstar_set_spread_spectrum {
    bool enable;           ///< 1 : enable, 0 : disalbe
    uint32_t crtcId;       ///< crtc id
    uint32_t connectorId;  ///< connector id
    uint16_t step;         ///< step of SSC
    uint16_t span;         ///< span of SSC
};

struct drm_sstar_set_white_balance {
    uint32_t crtcId;       ///< crtc id
    uint16_t redOffset;    ///< Offset of color R
    uint16_t greenOffset;  ///< Offset of color G
    uint16_t blueOffset;   ///< Offset of color B
    uint16_t redColor;     ///< Gain of Color R
    uint16_t greenColor;   ///< Gain of Color G
    uint16_t blueColor;    ///< Gain of Color B
};

///< Define sstar operation type
enum sstar_op {
    SSTAR_OP_GET = 0,
    SSTAR_OP_SET = 1,
};

///< Define CSC matrix type
enum drm_sstar_csc_matrix_type {
    SSTAR_CSC_MATRIX_BYPASS = 0,  ///< Do not change color space

    SSTAR_CSC_MATRIX_BT601_TO_BT709,  ///< Change color space from BT.601 to BT.709
    SSTAR_CSC_MATRIX_BT709_TO_BT601,  ///< Change color space from BT.709 to BT.601

    SSTAR_CSC_MATRIX_BT601_TO_RGB_PC,  ///< Change color space from BT.601 to RGB
    SSTAR_CSC_MATRIX_BT709_TO_RGB_PC,  ///< Change color space from BT.709 to RGB

    SSTAR_CSC_MATRIX_RGB_TO_BT601_PC,  ///< Change color space from RGB to BT.601
    SSTAR_CSC_MATRIX_RGB_TO_BT709_PC,  ///< Change color space from RGB to BT.709
    SSTAR_CSC_MATRIX_USER,             ///< Change color space fromPQbin

    SSTAR_CSC_MATRIX_BT601_L_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT709_L_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT2020_L_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT601_F_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT709_F_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT2020_F_TO_RGB_F,
    SSTAR_CSC_MATRIX_BT601_L_TO_RGB_L,
    SSTAR_CSC_MATRIX_BT709_L_TO_RGB_L,
    SSTAR_CSC_MATRIX_BT2020_L_TO_RGB_L,

    SSTAR_CSC_MATRIX_MAX
};

///< Define color space transform inforamtion
struct sstar_csc {
    uint8_t onlySetCsc;  ///< 1 cscMatrix is available only; 0 all is available
    enum drm_sstar_csc_matrix_type cscMatrix;  ///< CSC matrix type
    uint32_t luma;                             ///< Adjustment of luminance, Range: 0 ~ 100
    uint32_t contrast;                         ///< Adjustment of contrast, Range:   0 ~ 100
    uint32_t hue;                              ///< Adjustment of Hue, Range: 0 ~ 100
    uint32_t saturation;                       ///< Adjustment of saturation, Range: 0 ~ 100
};

struct drm_sstar_picture_quality {
    uint32_t crtcId;       ///< crtc id
    uint32_t connectorId;  ///< connector id
    enum sstar_op op;      ///< get or set parameter
    struct sstar_csc csc;  ///< Adjustment of CSC
    uint32_t sharpness;    ///< Adjustment of sharpness
    uint32_t gain;         ///< Adjustment of driving current, only for VGA
};

///< Define the flag of PQ
typedef enum {
    SSTAR_PQ_FLAG_LOAD_BIN,               ///< Load binary file
    SSTAR_PQ_FLAG_SET_SRC_ID,             ///< Set source ID
    SSTAR_PQ_FLAG_SET_LOAD_SETTING_TYPE,  ///< Set load type
    SSTAR_PQ_FLAG_FREE_BIN,               ///< Free binary file
    SSTAR_PQ_FLAG_BYPASS,                 ///< OFF device pq setting.
} SSTAR_PQFLAG_E;

struct drm_sstar_set_pq {
    uint32_t crtcId;         ///< crtc id
    SSTAR_PQFLAG_E pqFlags;  ///< The flag of pq action
    uint32_t dataSize;
    uint64_t data;
};

struct drm_sstar_set_dlc_active {
    uint32_t crtcId;
    uint32_t enable;
};

struct drm_sstar_get_luma {
    uint8_t avgVal;
    uint8_t maxVal;
    uint8_t minVal;
    uint8_t res;
};

struct drm_sstar_get_gamma_curve {
    uint32_t crtcId;
    uint32_t curveCnt;
};

///< Define sstar operation type
enum sstar_entry_num {
    SSTAR_ENTRY_NUM_256 = 0,
    SSTAR_ENTRY_NUM_33 = 1,
};

struct drm_sstar_set_gamma_curve {
    uint32_t crtcId;
    bool useCurve;                  ///< Use gamma curve in PQ.bin
    uint32_t curveId;               ///< Curve ID in PQ.bin
    bool useCust;                   ///< Use customer gamma table
    enum sstar_entry_num entryNum;  ///< Total entry number of gamma table
    uint64_t red;                   ///< Address of Gamma R(in uint8_t)
    uint64_t green;                 ///< Address of Gamma G(in uint8_t)
    uint64_t blue;                  ///< Address of Gamma B(in uint8_t)
};

struct sstar_global_paramaters {
    uint8_t g_dlcAutoEn;             // u8    0: disalbe DLC Auto adjust 1: enable DLC Auto adjust
    uint8_t ucDlcCurveMode;          // u8    0: curve by Yavg  1: curve by Histogram  2: Mix
    uint8_t alphaBlendingMin;        // u8    0~128
    uint8_t alphaBlendingMax;        // u8    0~128
    uint16_t alphaSpeed;             // u16   0~256 blending ratio between alphaBlendingMin and
                                     // alphaBlendingMax
    uint8_t ucDlcCurveModeMixAlpha;  // u8    0~128  for curve mode2
    uint8_t yAvgThresholdL;          // u8
    uint8_t yAvgThresholdM;          // u8
    uint8_t yAvgThresholdH;          // u8
    uint8_t curveDiffThresholdL;     // u8, 0~255
    uint8_t curveDiffThresholdH;     // u8, 0~255
    uint8_t yAvgDiffThresholdL;      // u8, 0~255
    uint8_t yAvgDiffThresholdH;      // u8, 0~255
};

struct drm_sstar_pq_algo_param {
    uint32_t dispId;
    enum sstar_op op;  ///< get or set parameter
    uint32_t id;       ///< ID of algo param
    uint32_t len;      ///< Length of algo param
    uint64_t param;    ///< Address of algo param
};

struct drm_sstar_get_hsy_curve_num {
    uint32_t crtcId;
    uint32_t curveNum;
};

enum drm_sstar_hsy_curve_type {
    SSTASR_HSY_CURVE_TYPE_STANDARD = 0,
    SSTASR_HSY_CURVE_TYPE_BRIGHTNESS = 1,
    SSTASR_HSY_CURVE_TYPE_SOFT = 2,
    SSTASR_HSY_CURVE_TYPE_NATURAL = 3,
    SSTASR_HSY_CURVE_TYPE_MOVE = 4,
    SSTASR_HSY_CURVE_TYPE_USER = 5,
    SSTASR_HSY_CURVE_TYPE_SPORT = 6,
    SSTASR_HSY_CURVE_TYPE_GAME = 7,
};

struct drm_sstar_set_hsy_param {
    uint32_t crtcId;
    enum drm_sstar_hsy_curve_type hsyCurve;
};

struct drm_sstar_tool_get_dlc_info {
#define DLC_HISTOGRAM_LEN (256 * sizeof(uint16_t))
#define DLC_CURVE_LEN (64 * sizeof(int32_t))
    uint32_t dispId;
    bool needHistInfo;  ///< Want to get histogram
    uint64_t histInfo;  ///< Address of histogram buffer, buffer length = DLC_HISTOGRAM_LEN
    bool needDlcCurve;  ///< Want to get dlc curve
    uint64_t dlcCurve;  ///< Address of dlc curve buffer, buffer length = DLC_CURVE_LEN
};

/*********************************************************************************************************/
typedef struct {
    int32_t x, y;
    int32_t width, height;
} SSTAR_WIN_T;

#if defined(__cplusplus)
}
#endif

#endif /* _SSTAR_DRM_H_ */
