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


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <map>
#include <sys/ioctl.h>
#include <vector>
#include "drm.h"
#include "xf86drm.h"
#include "xf86drmMode.h"
#include "drm_fourcc.h"
#include <signal.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <inttypes.h>

#include <cstring>

#include "player.h"
#include "list.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_common_datatype.h"
#include "mi_hdmirx.h"
#include "mi_hdmirx_datatype.h"
#include "mi_hvp.h"
#include "mi_hvp_datatype.h"
#include "mi_scl.h"
#include <gpu_graphic_effect.h>
#include <gpu_graphic_render.h>

#include "sstar_drm.h"

#include "sstar_sensor_demo.h"
#include "hdmi_player.h"

#define VEDIO_WIDTH     1920
#define VEDIO_HEIGHT    1080


#define DUMP_OPTION

#define USE_GPUGFX_ROCESS


typedef struct plane_info_s {
    seq_info_t seq_info; // current stream sequence info, get seq info by callback function

    int rotate;          // current rotate mode
    int ratio;           // current video display ratio

    int in_width;        // screen width, change along with the rotate mode
    int in_height;       // screen height, change along with the rotate mode
    int dec_width;       // decode output width
    int dec_height;      // decode output height
    int rot_width;       // output width after rotation
    int rot_height;      // output height after rotation
    int out_width;       // display window width
    int out_height;      // display window height
    int pos_x;           // display the starting horizontal axis of the image
    int pos_y;           // display the starting vertical axis of the image
} plane_info_t;

typedef struct player_obj_s {
    MI_BOOL pIsCreated;
    bool exit;
    bool player_working;
    int  loop_mode;
    plane_info_t video_info;
    int stream_width;
    int stream_height;
    plane_info_t ui_info;
    player_info_t param;
} media_player_t;

typedef struct hdmirx_obj_s {
    MI_BOOL pIsCreated;
    MI_BOOL pIsCleaned; //clean mop
    bool player_working;
    MI_HDMIRX_PortId_e u8HdmirxPort;
    MI_SYS_ChnPort_t stHvpChnPort;
    MI_SYS_ChnPort_t stSclChnPort;
    MI_SYS_BindType_e eBindType;
    int hvpFrameRate;
    int sclFrameRate;
    MI_BOOL pbConnected;
    MI_HDMIRX_SigStatus_e  eSignalStatus;
    MI_HDMIRX_TimingInfo_t pstTimingInfo;

    MI_HVP_SignalStatus_e eHvpSignalStatus;
    MI_HVP_SignalTiming_t pHvpstTimingInfo;

} hdmirx_player_t;


typedef struct convans_obj_s {
    MI_BOOL pIsCreated;
    std::shared_ptr<GpuGraphicBuffer> mainFbCanvas;
    pthread_mutex_t mutex;

} convans_contex_t;


convans_contex_t _g_MainCanvas;
convans_contex_t _g_MainSorCanvas;
std::shared_ptr<GpuGraphicBuffer> g_cursor_canvas;

std::shared_ptr<GpuGraphicBuffer> g_MainFbCanvas;
pthread_mutex_t g_MainFbCanvas_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_cursor_canvas_mutex = PTHREAD_MUTEX_INITIALIZER;


static media_player_t _g_MediaPlayer;
static hdmirx_player_t _g_HdmiRxPlayer;


#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))
#define ALIGN_UP(x, a)            (((x+a-1) / (a)) * (a))

#define MIN(a,b)                    ((a)>(b)?(b):(a))
#define MAX(a, b)           ((a) < (b) ? (b) : (a))

#define ENCRYPTED_HDCP_LEN 341
#define DECRYPTED_HDCP_LEN 293
#define EDID_LEN 256
#define DMA_HEAP_IOC_MAGIC    'H'
#define DMA_HEAP_IOCTL_ALLOC    _IOWR(DMA_HEAP_IOC_MAGIC,0,dma_heap_allocation_data)

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_)                                                              \
    do                                                                                     \
    {                                                                                      \
        MI_S32 s32Ret = MI_SUCCESS;                                                        \
        s32Ret        = _func_;                                                            \
        if (s32Ret != MI_SUCCESS)                                                          \
        {                                                                                  \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret;                                                                 \
        }                                                                                  \
        else                                                                               \
        {                                                                                  \
            printf("(%s %d)exec function pass\n", __FUNCTION__, __LINE__);                 \
        }                                                                                  \
    } while (0)
#endif

#ifndef SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP
#define SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(__hvp, __hdmirx) \
    do                                                          \
    {                                                           \
        switch (__hdmirx)                                       \
        {                                                       \
            case E_MI_HDMIRX_PIXEL_FORMAT_RGB:                  \
                __hvp = E_MI_HVP_COLOR_FORMAT_RGB444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV444:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV444;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV422:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV422;           \
                break;                                          \
            case E_MI_HDMIRX_PIXEL_FORMAT_YUV420:               \
                __hvp = E_MI_HVP_COLOR_FORMAT_YUV420;           \
                break;                                          \
            default:                                            \
                __hvp = E_MI_HVP_COLOR_FORMAT_MAX;              \
                break;                                          \
        }                                                       \
    } while(0)
#endif

#ifndef SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP
#define SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(__hvp, __hdmirx) \
do                                                             \
    {                                                          \
        switch (__hdmirx)                                      \
        {                                                      \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_8BIT:              \
                __hvp = E_MI_HVP_COLOR_DEPTH_8;                \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_10BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_10;               \
                break;                                         \
            case E_MI_HDMIRX_PIXEL_BITWIDTH_12BIT:             \
                __hvp = E_MI_HVP_COLOR_DEPTH_12;               \
                break;                                         \
            default:                                           \
                __hvp = E_MI_HVP_COLOR_DEPTH_MAX;              \
                break;                                         \
        }                                                      \
    } while(0)
#endif

#ifndef SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP
#define SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(__hvp, __hdmirx)      \
do                                                                       \
    {                                                                    \
        switch (__hdmirx)                                                \
        {                                                                \
            case E_MI_HDMIRX_OVERSAMPLE_1X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_1X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_2X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_2X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_3X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_3X;                        \
                break;                                                   \
            case E_MI_HDMIRX_OVERSAMPLE_4X:                              \
                __hvp = E_MI_HVP_PIX_REP_TYPE_4X;                        \
                break;                                                   \
            default:                                                     \
                __hvp = E_MI_HVP_PIX_REP_TYPE_MAX;                       \
                break;                                                   \
        }                                                                \
    } while(0)
#endif

enum DrmPlaneId
{
    GOP_UI_ID = 31,
    GOP_CURSOR_ID = 40,
    MOPG_ID0 = 47,
    MOPG_ID1 = 53,
    MOPG_ID2 = 59,
    MOPG_ID3 = 65,
    MOPG_ID4 = 71,
    MOPG_ID5 = 77,
    MOPG_ID6 = 83,
    MOPG_ID7 = 89,
    MOPG_ID8 = 95,
    MOPG_ID9 = 101,
    MOPG_ID10 = 107,
    MOPG_ID11 = 113,
    MOPS_ID = 119,
};

typedef enum
{
    E_STREAM_YUV422 = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
    E_STREAM_YUV422_UYVY = E_MI_SYS_PIXEL_FRAME_YUV422_UYVY,
    E_STREAM_YUV422_YVYU = E_MI_SYS_PIXEL_FRAME_YUV422_YVYU,
    E_STREAM_YUV422_VYUY = E_MI_SYS_PIXEL_FRAME_YUV422_VYUY,
    E_STREAM_ARGB8888 = E_MI_SYS_PIXEL_FRAME_ARGB8888,
    E_STREAM_ABGR8888 = E_MI_SYS_PIXEL_FRAME_ABGR8888,
    E_STREAM_BGRA8888 = E_MI_SYS_PIXEL_FRAME_BGRA8888,
    E_STREAM_YUV420 = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    E_STREAM_RGB_BAYER_BASE = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_BASE,
    E_STREAM_RGB_BAYER_MAX = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_NUM,
    E_STREAM_MAX = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX,
} E_VIDEO_RAW_FORMAT;


unsigned long long cursor_time = 0;
unsigned int cursor_x_size = 320;
unsigned int cursor_y_size = 240;

unsigned int g_tmp_x = 0;
unsigned int g_tmp_y = 0;


struct buffer_object {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint8_t *vaddr;
    uint32_t fb_id;
};

typedef struct
{
    unsigned int fb_id;
    unsigned int gem_handle[3];
}drm_buf_t;

typedef struct
{
    unsigned int  width;
    unsigned int  height;
    unsigned int  format;
    int fds[3];
    int stride[3];
    int dataOffset[3];
    uint64_t modifiers;
    void *priavte;
}stDmaBuf_t;

typedef struct{
    MI_U64 len;
    MI_U32 fd;
    MI_U32 fd_flags;
    MI_U64 heap_flags;
}dma_heap_allocation_data;

struct buffer_object buf;

typedef struct drm_property_ids_s {
    uint32_t FB_ID;
    uint32_t CRTC_ID;
    uint32_t CRTC_X;
    uint32_t CRTC_Y;
    uint32_t CRTC_W;
    uint32_t CRTC_H;
    uint32_t SRC_X;
    uint32_t SRC_Y;
    uint32_t SRC_W;
    uint32_t SRC_H;
    uint32_t FENCE_ID;
    uint32_t ACTIVE;
    uint32_t MODE_ID;
    uint32_t zpos;
    uint32_t realtime_mode;
} drm_property_ids_t;


typedef struct DrmModeConfiction_s
{
    int fd;
    int out_fence;
    unsigned int crtc_id;
    unsigned int has_modifiers;
    unsigned int osd_commited;
    unsigned int sor_commited;
    unsigned int mop_commited;
    unsigned int conn_id;
    unsigned int width;
    unsigned int height;
    unsigned int blob_id;
    unsigned int fb_id[3];//0 for GOP;1 for mop
    unsigned int planeNeedUpdate[3];//0 for GOP;1 for mop
    unsigned int planeNeedClean[3];//0 for GOP;1 for mop
    drm_property_ids_t drm_mode_prop_ids[3];//0 for GOP;1 for mop
    drmModeRes *pDrmModeRes;
    pthread_mutex_t commit_Mutex;
    drmModeConnectorPtr Connector;
}DrmModeConfiction_t;
static DrmModeConfiction_t g_stDrmCfg;

typedef struct St_ListNode_s
{
    struct list_head    list;
    pthread_mutex_t EntryMutex;
    frame_info_t frame_info;
    int planeNeedUpdate;
    unsigned long eTime;
    std::shared_ptr<GpuGraphicBuffer>   pGraphicBuffer;

}St_ListNode_t;


#define BUF_DEPTH 3  //enqueue for scl outputport
std::shared_ptr<GpuGraphicBuffer> _g_GfxInputBuffer[BUF_DEPTH];
St_ListNode_t *_g_GfxInputEntryNode[BUF_DEPTH];



static int  _g_rotate_mode  = (int)AV_ROTATE_NONE;
static plane_info_t g_CurVideoInfo;
static plane_info_t g_CurOsdInfo;
static plane_info_t g_CurSorInfo;

MI_U32 g_u32UiWidth = 0;
MI_U32 g_u32UiHeight = 0;
MI_U16 g_u16DispWidth = 0;
MI_U16 g_u16DispHeight = 0;


MI_U8 g_u8PipelineMode = 0xff;
char  *g_InputFile;
char  g_InputUiPath[128];

MI_BOOL bShowUi = 0;

pthread_t hdmi_detect_thread;

pthread_t g_pThreadSnrReceive;
MI_BOOL g_bThreadExitReceive = TRUE;

pthread_t hvp_event_thread;
static bool hvp_event_thread_running = false;

pthread_t g_pThreadScl;
MI_BOOL g_bThreadExitScl = TRUE;

pthread_t g_pThreadGfx;
MI_BOOL g_bThreadExitGfx = TRUE;


pthread_t g_pThreadAudio;


pthread_t g_pThreadHdmi;
MI_BOOL g_bThreadExitHdmi = TRUE;

pthread_t g_pThreadHdmiAudio;
MI_BOOL g_bThreadExitHdmiAudio = TRUE;


pthread_t g_pThreadUiDrm;
MI_BOOL g_bThreadExitUiDrm = TRUE;

pthread_t g_pThreadCursorDrm;
MI_BOOL g_bThreadExitCursorDrm = TRUE;

pthread_t g_pThreadCommit;
MI_BOOL g_bThreadExitCommit = TRUE;

pthread_t g_pThreadUpdatePoint;
MI_BOOL g_bThreadExitUpdatePoint = TRUE;


pthread_t g_player_thread;
MI_BOOL g_bThreadExitPlayer = TRUE;


MI_U32 g_u32GpuLT_X, g_u32GpuLT_Y;
MI_U32 g_u32GpuLB_X, g_u32GpuLB_Y;
MI_U32 g_u32GpuRT_X, g_u32GpuRT_Y;
MI_U32 g_u32GpuRB_X, g_u32GpuRB_Y;


MI_BOOL g_bGpuProcessNeed = false;
pthread_mutex_t g_OsdProcessMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_SorProcessMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_VideoProcessMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_HdmiRXProcessMutex = PTHREAD_MUTEX_INITIALIZER;


MI_BOOL g_bChangePointOffset = false;

std::pair<uint32_t, uint32_t> prev_gop_gem_handle(static_cast<uint32_t>(0), static_cast<uint32_t>(0));
std::pair<uint32_t, uint32_t> prev_mop_gem_handle(static_cast<uint32_t>(0), static_cast<uint32_t>(0));
std::pair<uint32_t, uint32_t> prev_sor_gem_handle(static_cast<uint32_t>(0), static_cast<uint32_t>(0));


std::shared_ptr<GpuGraphicEffect> g_stdOsdGpuGfx = NULL;
std::shared_ptr<GpuGraphicRender> g_stdOsdGpuRender = NULL;

std::shared_ptr<GpuGraphicEffect> g_stdSorGpuGfx = NULL;
std::shared_ptr<GpuGraphicRender> g_stdSorGpuRender = NULL;


std::shared_ptr<GpuGraphicEffect> g_stdVideoGpuGfx = NULL;
std::shared_ptr<GpuGraphicEffect> g_stdHdmiRxGpuGfx = NULL;
std::shared_ptr<GpuGraphicRender> g_stdGfxGpuRender = NULL;

Rect g_RectDisplayRegion[2]; //0 for osd,1 for video

static frame_info_t g_TemFrameInfo;
std::shared_ptr<GpuGraphicBuffer>   g_CurGraphicBuffer = NULL;


typedef struct  St_List_s {
    MI_BOOL listCreated;
    pthread_mutex_t listMutex;
    struct list_head queue_list;
}St_List_t;


St_List_t g_HeadListSclOutput;
St_List_t g_HeadListGpuGfxInput;

St_List_t g_HeadListOsdOutput;
St_List_t g_HeadListOsdCommit;

St_List_t g_HeadListCursorOutput;
St_List_t g_HeadListCursorCommit;

St_List_t g_HeadListVideoOutput;
St_List_t g_HeadListVideoCommit;

std::shared_ptr<GpuGraphicBuffer> g_OsdOutputBuffer[BUF_DEPTH];
static St_ListNode_t *_g_OsdListEntryNode[BUF_DEPTH];
static St_ListNode_t *_g_CursorListEntryNode[BUF_DEPTH];

static St_ListNode_t *_g_VideoListEntryNode[BUF_DEPTH];


//MI_U8 u8EnqueueDmabufCnt = 0;

static  MI_U8 EDID_Test[256] =
{
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x4D, 0x27, 0x72, 0x09, 0x01, 0x00, 0x00, 0x00,
    0x07, 0x1E, 0x01, 0x03, 0x80, 0x73, 0x41, 0x78, 0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
    0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
    0x45, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00, 0xBC, 0x52, 0xD0, 0x1E, 0x20,
    0xB8, 0x28, 0x55, 0x40, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x53,
    0x69, 0x67, 0x6D, 0x61, 0x73, 0x74, 0x61, 0x72, 0x0A, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
    0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x00,
    0x02, 0x03, 0x3E, 0xF0, 0x53, 0x10, 0x1F, 0x14, 0x05, 0x13, 0x04, 0x20, 0x22, 0x3C, 0x3E, 0x12,
    0x16, 0x03, 0x07, 0x11, 0x15, 0x02, 0x06, 0x01, 0x2F, 0x09, 0x7F, 0x05, 0x15, 0x07, 0x50, 0x57,
    0x07, 0x00, 0x3D, 0x07, 0xC0, 0x5F, 0x7E, 0x01, 0x83, 0x01, 0x00, 0x00, 0x6E, 0x03, 0x0C, 0x00,
    0x10, 0x00, 0xB8, 0x3C, 0x2F, 0x00, 0x80, 0x01, 0x02, 0x03, 0x04, 0xE2, 0x00, 0xFB, 0x01, 0x1D,
    0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
    0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00, 0x13, 0x8E, 0x21, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
};

static  MI_U8 HDCP_KEY[289] =
{
    0x00,
    0x0B,0xE4,0xCE,0xED,0x24,0x00,0x00,0x00,
    0x02,0xD6,0xD3,0xBC,0x1E,0x39,0x40,
    0xA9,0x46,0xAF,0x6B,0x46,0xF2,0x80,
    0x07,0x12,0xC0,0x19,0xE1,0xE1,0x5D,
    0xF8,0x02,0x14,0x3C,0xD9,0x8F,0xBB,
    0x3B,0xCD,0x09,0x23,0x59,0x11,0x7B,
    0x11,0x47,0x6F,0x95,0x74,0xDC,0x0A,
    0x29,0xFF,0x5C,0xFF,0xE8,0x56,0x59,
    0x58,0x8E,0x3F,0x41,0xFA,0x06,0xAB,
    0xA5,0xD8,0x37,0xE6,0x3D,0x64,0x7C,
    0xB5,0xBB,0x80,0x28,0x2F,0x2E,0x22,
    0xD5,0x87,0x68,0x90,0xB4,0x23,0xB8,
    0x02,0x2F,0x16,0x6B,0x10,0x2C,0xA9,
    0x15,0x1E,0x47,0x45,0x52,0x38,0xF8,
    0x9A,0x4A,0x5C,0x7C,0x4B,0xF7,0x70,
    0x5C,0x72,0xAC,0xBB,0xAC,0xF4,0xFD,
    0xEB,0x0C,0x2B,0x09,0x81,0xE1,0xBF,
    0x73,0xEF,0x60,0x6B,0x20,0x4C,0x5C,
    0x14,0x80,0x0D,0x0C,0x89,0x86,0x91,
    0x7B,0x06,0x5F,0x83,0xE0,0x2F,0xA3,
    0x30,0x2D,0x11,0x64,0x37,0xC2,0xB3,
    0x3B,0xA5,0x36,0x2D,0x1F,0x05,0x3B,
    0x86,0x04,0x0C,0x53,0xEE,0xD5,0x33,
    0xB5,0x27,0xB1,0x73,0x4C,0x1E,0xBB,
    0x0E,0xF6,0xAA,0x3D,0xE6,0xDB,0x6C,
    0x05,0x07,0x27,0xB9,0x29,0xE4,0x87,
    0xA2,0xEB,0xDB,0xB5,0xAA,0xD4,0x4D,
    0xB6,0x0A,0x33,0x45,0x2F,0x1A,0x58,
    0x81,0xF0,0x2E,0x5A,0x8D,0x1D,0x6E,
    0xA2,0xEA,0x9F,0x8D,0x05,0x11,0xDA,
    0xDB,0xCF,0xCF,0x57,0xCB,0x88,0x17,
    0x4F,0x29,0x8B,0xE3,0x14,0xDA,0x09,
    0xA2,0x37,0x9C,0xDE,0x0A,0xDB,0x2A,
    0xE3,0xD4,0xF9,0xFC,0x79,0x31,0xAD,
    0xFE,0xEF,0xEC,0x4B,0x2E,0xE9,0xFF,
    0x87,0x90,0x14,0xC3,0xD4,0x7D,0x39,
    0x28,0x42,0x92,0x3D,0x4E,0x43,0xB7,
    0x22,0xAE,0x4B,0x36,0x41,0x9F,0x4E,
    0xD7,0x97,0x57,0x59,0x97,0x4A,0xD8,
    0xE3,0x50,0x9A,0x03,0x88,0xEE,0x21,
    0xD8,0x53,0x6A,0x9F,0x31,0xB5,0x88,
};

typedef struct St_Csc_s
{
    drm_sstar_csc_matrix_type eCscMatrix;
    MI_U32 u32Luma;                     /* luminance:   0 ~ 100 default: 50 */
    MI_U32 u32Contrast;                 /* contrast :   0 ~ 100 default: 50 */
    MI_U32 u32Hue;                      /* hue      :   0 ~ 100 default: 50 */
    MI_U32 u32Saturation;               /* saturation:  0 ~ 100 default: 50 */
    MI_U32 u32Gain;                     /* current gain of VGA signals. [0, 64). default:0x30 */
    MI_U32 u32Sharpness;

} St_Csc_t;


unsigned long long getOsTime(void)
{
    unsigned long long u64CurTime = 0;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    u64CurTime = ((unsigned long long)(tv.tv_sec))*1000 + tv.tv_usec/1000;
    return u64CurTime;
}


static St_ListNode_t* get_first_node(St_List_t *st_list)
{
    St_ListNode_t *entry_tmp;

    St_ListNode_t *entry_node = NULL;
    pthread_mutex_lock(&st_list->listMutex);
    if(st_list->listCreated)
    {
        if(!(list_empty(&st_list->queue_list)))
        {
            list_for_each_entry_safe(entry_node, entry_tmp, &st_list->queue_list, list) {
                list_del(&entry_node->list); // Delete node
                break;
            }
        }
    }

    pthread_mutex_unlock(&st_list->listMutex);
    return entry_node;
}

static void init_list_head(St_List_t *st_list)
{
    st_list->listMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&st_list->listMutex);
    INIT_LIST_HEAD(&st_list->queue_list);
    st_list->listCreated = true;
    pthread_mutex_unlock(&st_list->listMutex);
}


static void add_tail_node(St_List_t *st_list, list_head *list_node)
{

    pthread_mutex_lock(&st_list->listMutex);
    if(st_list->listCreated)
    {
        list_add_tail(list_node, &st_list->queue_list);
    }
    pthread_mutex_unlock(&st_list->listMutex);
}

static void clean_list_node(St_List_t *st_list)
{
    St_ListNode_t *entry_tmp = NULL;
    St_ListNode_t *entry_node = NULL;
    pthread_mutex_lock(&st_list->listMutex);
    printf("clean_list_node listCreated=%d\n",st_list->listCreated);
    if(st_list->listCreated)
    {
        if(!(list_empty(&st_list->queue_list)))
        {
            list_for_each_entry_safe(entry_node, entry_tmp, &st_list->queue_list, list)
            {
                list_del(&entry_node->list); // 删除节点
            }
        }
    }
    st_list->listCreated = false;
    pthread_mutex_unlock(&st_list->listMutex);
}


MI_S32 sstar_set_pictureQuality(St_Csc_t picQuality)
{
    struct drm_sstar_picture_quality pictureQuality;

    pictureQuality.op = SSTAR_OP_SET;
    pictureQuality.csc.onlySetCsc = 0;
    pictureQuality.csc.cscMatrix = SSTAR_CSC_MATRIX_USER;
    pictureQuality.connectorId = g_stDrmCfg.conn_id;
    pictureQuality.crtcId = g_stDrmCfg.crtc_id;
    pictureQuality.csc.luma = picQuality.u32Luma;
    pictureQuality.csc.contrast = picQuality.u32Contrast;
    pictureQuality.csc.saturation = picQuality.u32Saturation;
    pictureQuality.sharpness = picQuality.u32Sharpness;
    pictureQuality.csc.hue = picQuality.u32Hue;
    pictureQuality.gain = picQuality.u32Gain;
    int32_t ret = drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_SSTAR_PICTURE_QUALITY, &pictureQuality);
    if (ret < 0) {
        printf("sstar_set_pictureQuality error\n");
        return -1;
    }
    printf("set:luma=%d contrast=%d saturation=%d sharpness=%d\n",pictureQuality.csc.luma, pictureQuality.csc.contrast, pictureQuality.csc.saturation, pictureQuality.sharpness);
    return MI_SUCCESS;

}

MI_S32 sstar_get_pictureQuality(St_Csc_t *picQuality)
{
    struct drm_sstar_picture_quality pictureQuality;
    memset(&pictureQuality, 0x00, sizeof(drm_sstar_picture_quality));
    pictureQuality.op = SSTAR_OP_GET;
    pictureQuality.connectorId = g_stDrmCfg.conn_id;
    pictureQuality.crtcId = g_stDrmCfg.crtc_id;

    int32_t ret = drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_SSTAR_PICTURE_QUALITY, &pictureQuality);
    if (ret < 0) {
        printf("sstar_get_pictureQuality error\n");
        return -1;
    }
    picQuality->u32Gain = pictureQuality.gain;
    picQuality->u32Luma = pictureQuality.csc.luma;
    picQuality->u32Contrast = pictureQuality.csc.contrast;
    picQuality->u32Saturation = pictureQuality.csc.saturation;
    picQuality->u32Sharpness = pictureQuality.sharpness;
    picQuality->u32Hue = pictureQuality.csc.hue;
    picQuality->eCscMatrix = pictureQuality.csc.cscMatrix;

    printf("get:luma=%d contrast=%d saturation=%d sharpness=%d\n",pictureQuality.csc.luma, pictureQuality.csc.contrast, pictureQuality.csc.saturation, pictureQuality.sharpness);
    return MI_SUCCESS;
}

MI_S32 sstar_set_DLC(int enable)
{
    struct drm_sstar_set_dlc_active dlc;
    memset(&dlc, 0x00, sizeof(drm_sstar_set_dlc_active));
    dlc.crtcId = g_stDrmCfg.crtc_id;
    dlc.enable = enable;
    if(dlc.crtcId){
        int32_t ret = drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_SSTAR_SET_DLC_ACTIVE, &dlc);
        if (ret < 0) {
            printf("sstar_set_DLC error\n");
            return -1;
        }
    }else{
        return -1;
    }
    return MI_SUCCESS;
}

MI_S32 sstar_get_luma(drm_sstar_get_luma luma)
{
    memset(&luma, 0x00, sizeof(drm_sstar_get_luma));
    int32_t ret = drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_SSTAR_GET_LUMA, &luma);
    if (ret < 0) {
        printf("sstar_get_luma error\n");
        return -1;
    }
    return MI_SUCCESS;
}

MI_S32 sstar_get_histInfo(uint16_t Dlc_info[256])
{
    struct drm_sstar_tool_get_dlc_info pictureQuality;
    memset(&pictureQuality, 0x00, sizeof(drm_sstar_tool_get_dlc_info));
    pictureQuality.needHistInfo = 1;
    pictureQuality.histInfo = (uint64_t)Dlc_info;
    int32_t ret = drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_SSTAR_TOOL_GET_DLC_INFO, &pictureQuality);
    if (ret < 0) {
        printf("sstar_get_histInfo error\n");
        return -1;
    }
    return MI_SUCCESS;
}

St_Csc_t g_picQuality;


FILE* open_dump_file(char *path)
{
#ifdef DUMP_OPTION
    FILE *out_fd = NULL;

    char file_out[128];
    memset(file_out, '\0', sizeof(file_out));
    sprintf(file_out, path);

    if(out_fd == NULL)
    {
        out_fd = fopen(file_out, "awb");
        if (!out_fd)
        {
            printf("fopen %s falied!\n", file_out);
            fclose(out_fd);
            out_fd = NULL;
        }
    }
    return out_fd;
#else
    return NULL;
#endif
}

void wirte_dump_file(FILE *fd, char *p_viraddr, int len)
{
#ifdef DUMP_OPTION
    printf("write len=%d \n",len);
    if(fd && p_viraddr && len > 0)
    {
        fwrite(p_viraddr, len, 1, fd);
    }
#endif
}

void close_dump_file(FILE *fd)
{
#ifdef DUMP_OPTION
    if(fd)
    {
        fclose(fd);
        fd = NULL;
    }
#endif
}

static int signal_monitor_sync_hdmi_paramter_2_hvp(const MI_HDMIRX_TimingInfo_t *pstTimingInfo)
{
    MI_S32 ret = 0;
    MI_HVP_ChannelParam_t stChnParam;
    MI_HVP_DEV dev = 0;
    MI_HVP_CHN chn = 0;

    memset(&stChnParam, 0, sizeof(MI_HVP_ChannelParam_t));
    ret = MI_HVP_GetChannelParam(dev, chn, &stChnParam);
    if (ret != MI_SUCCESS)
    {
        printf("Get HVP Param error! Dev %d Chn %d\n", dev, chn);
        return -1;
    }
    SIGNAL_MONITOR_PIXEX_REPETITIVE_HDMI_2_HVP(stChnParam.stChnSrcParam.enPixRepType, pstTimingInfo->eOverSample);
    SIGNAL_MONITOR_COLOR_FORMAT_HDMI_2_HVP(stChnParam.stChnSrcParam.enInputColor, pstTimingInfo->ePixelFmt);
    SIGNAL_MONITOR_COLOR_DEPTH_HDMI_2_HVP(stChnParam.stChnSrcParam.enColorDepth, pstTimingInfo->eBitWidth);
    ret = MI_HVP_SetChannelParam(dev, chn, &stChnParam);
    if (ret != MI_SUCCESS)
    {
        printf("Set HVP param error! Dev %d Chn %d\n", dev, chn);
        return -1;
    }
    return 0;
}


static inline int sync_wait(int fd, int timeout)
{
    struct timeval time;
    fd_set fs_read;
    int ret;
    time.tv_sec = timeout / 1000;
    time.tv_usec = timeout % 1000 * 1000;

    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

    ret = select(fd + 1, &fs_read, NULL, NULL, &time);
    if(ret <= 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static int get_property_id(int fd, drmModeObjectProperties* props, const char* name) {
    int id = 0;
    drmModePropertyPtr property;
    int found = 0;

    for (unsigned int i = 0; !found && i < props->count_props; ++i) {
        property = drmModeGetProperty(fd, props->props[i]);
        if (!strcmp(property->name, name)) {
            id = property->prop_id;
            found = 1;
        }
        drmModeFreeProperty(property);
    }

    return id;
}

#if 0
static int get_property_value(int fd, drmModeObjectProperties* props, const char* name) {
    int value = 0;
    drmModePropertyPtr property;
    int found = 0;

    for (unsigned int i = 0; !found && i < props->count_props; ++i) {
        property = drmModeGetProperty(fd, props->props[i]);
        if (!strcmp(property->name, name)) {
            value = property->values[i];
            found = 1;
        }
        drmModeFreeProperty(property);
    }

    return value;
}
#endif

int init_drm_property_ids(uint32_t plane_id, drm_property_ids_t* prop_ids)//int fd, uint32_t crtc_id, uint32_t plane_id, uint32_t conn_id,drm_property_ids_t* prop_ids)
{

    drmModeObjectProperties* props;
    drmModeAtomicReqPtr req;
    int ret;

    if(g_stDrmCfg.fd <= 0)
    {
        printf("Please check param \n");
        return -1;
    }

    props = drmModeObjectGetProperties(g_stDrmCfg.fd, g_stDrmCfg.crtc_id, DRM_MODE_OBJECT_CRTC);
    if (!props) {
        printf("Get properties error,crtc_id=%d \n",g_stDrmCfg.crtc_id);
        return -1;
    }

    prop_ids->FENCE_ID = get_property_id(g_stDrmCfg.fd, props, "OUT_FENCE_PTR");
    prop_ids->ACTIVE = get_property_id(g_stDrmCfg.fd, props, "ACTIVE");
    prop_ids->MODE_ID = get_property_id(g_stDrmCfg.fd, props, "MODE_ID");

    drmModeFreeObjectProperties(props);


    drmModeCreatePropertyBlob(g_stDrmCfg.fd, &g_stDrmCfg.Connector->modes[0],sizeof(g_stDrmCfg.Connector->modes[0]), &g_stDrmCfg.blob_id);


    props = drmModeObjectGetProperties(g_stDrmCfg.fd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!props) {
        printf("Get properties error,plane_id=%d \n",plane_id);
        return -1;
    }

    prop_ids->FB_ID = get_property_id(g_stDrmCfg.fd, props, "FB_ID");
    prop_ids->CRTC_ID = get_property_id(g_stDrmCfg.fd, props, "CRTC_ID");
    prop_ids->CRTC_X = get_property_id(g_stDrmCfg.fd, props, "CRTC_X");
    prop_ids->CRTC_Y = get_property_id(g_stDrmCfg.fd, props, "CRTC_Y");
    prop_ids->CRTC_W = get_property_id(g_stDrmCfg.fd, props, "CRTC_W");
    prop_ids->CRTC_H = get_property_id(g_stDrmCfg.fd, props, "CRTC_H");
    prop_ids->SRC_X = get_property_id(g_stDrmCfg.fd, props, "SRC_X");
    prop_ids->SRC_Y = get_property_id(g_stDrmCfg.fd, props, "SRC_Y");
    prop_ids->SRC_W = get_property_id(g_stDrmCfg.fd, props, "SRC_W");
    prop_ids->SRC_H = get_property_id(g_stDrmCfg.fd, props, "SRC_H");
    prop_ids->realtime_mode = get_property_id(g_stDrmCfg.fd, props, "realtime_mode");
    prop_ids->zpos = get_property_id(g_stDrmCfg.fd, props, "zpos");

    drmModeFreeObjectProperties(props);

    printf("DRM property ids. FB_ID=%d CRTC_ID=%d CRTC_X=%d CRTC_Y=%d CRTC_W=%d CRTC_H=%d"
           "SRC_X=%d SRC_Y=%d SRC_W=%d SRC_H=%d, realtime_mode=%d, FENCE_ID=%d zpos=%d\n",
           prop_ids->FB_ID, prop_ids->CRTC_ID, prop_ids->CRTC_X, prop_ids->CRTC_Y, prop_ids->CRTC_W,
           prop_ids->CRTC_H, prop_ids->SRC_X, prop_ids->SRC_Y, prop_ids->SRC_W, prop_ids->SRC_H,
           prop_ids->realtime_mode,prop_ids->FENCE_ID,prop_ids->zpos);

    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, prop_ids->ACTIVE, 1);
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, prop_ids->MODE_ID, g_stDrmCfg.blob_id);
    drmModeAtomicAddProperty(req, g_stDrmCfg.conn_id, prop_ids->CRTC_ID, g_stDrmCfg.crtc_id);
    if(plane_id == GOP_UI_ID)
    {
        ret = drmModeAtomicCommit(g_stDrmCfg.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK, NULL);

        if(ret != 0)
        {
            printf("drmModeAtomicCommit failed ret=%d plane_id=%d\n",ret,plane_id);
            //return -1;
        }
    }
    drmModeAtomicFree(req);
    return 0;
}


static int add_plane_property(drmModeAtomicReqPtr req, int plane_id, uint32_t prop_id, int val)
{
    #if 0
    drmModeObjectProperties* pModeProps = NULL;
    int prop_id = -1;
    pModeProps = drmModeObjectGetProperties(g_stDrmCfg.fd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!pModeProps) {
        printf("Get properties error,plane_id=%d \n",plane_id);
        return  -1;
    }
    prop_id = get_property_id(g_stDrmCfg.fd, pModeProps, name);

    drmModeFreeObjectProperties(pModeProps);
    #endif
    drmModeAtomicAddProperty(req, plane_id, prop_id, val);
    return 0;
}


static int sstar_add_commitbuf(std::shared_ptr<GpuGraphicBuffer> commitBuf, drm_buf_t *drm_buf)
{
    int ret = -1;
    unsigned int pitches[4] = {0}, offsets[4] = {0}, gem_handles[4] = {0};
    uint64_t modifiers[4] = {0};
    unsigned int width = 0, height = 0, fb_id = 0, drm_fmt = 0;
    int prime_fd;

    //printf("getFormat=%d DRM_FORMAT_NV12=%d DRM_FORMAT_ABGR8888=%d\n", commitBuf->getFormat(),DRM_FORMAT_NV12,DRM_FORMAT_ABGR8888);
    width = commitBuf->getWidth();
    height = commitBuf->getHeight();
    prime_fd= commitBuf->getFd();
    drm_fmt = commitBuf->getFormat();

    if(drm_fmt  == DRM_FORMAT_NV12)
    {
        pitches[0] = commitBuf->getStride()[0];
        pitches[1] = commitBuf->getStride()[1];
        offsets[0] = commitBuf->getPlaneOffset()[0];
        offsets[1] = commitBuf->getPlaneOffset()[1];
        modifiers[0] = commitBuf->getModifier();
        modifiers[1] = commitBuf->getModifier();
        drm_fmt = DRM_FORMAT_NV12;
    }
    if(drm_fmt  == DRM_FORMAT_ABGR8888)
    {
        pitches[0] = commitBuf->getStride()[0];
        offsets[0] = commitBuf->getPlaneOffset()[0];
        modifiers[0] = commitBuf->getModifier();
        drm_fmt = DRM_FORMAT_ABGR8888;
    }

    ret = drmPrimeFDToHandle(g_stDrmCfg.fd, prime_fd, &gem_handles[0]);
    if (ret) {
        printf("drmPrimeFDToHandle failed, ret=%d dma_buffer_fd=%d\n", ret, prime_fd);
        return -1;
    }
    //printf("g_stDrmCfg.has_modifiers=%d modifiers=%lld\n", g_stDrmCfg.has_modifiers,modifiers[0]);
    if(modifiers[0] == 0)
    {
        gem_handles[1] = gem_handles[0];
        ret = drmModeAddFB2(g_stDrmCfg.fd, width, height, drm_fmt, gem_handles, pitches, offsets, &fb_id, 0);
        if (ret)
        {
            printf("drmModeAddFB2 failed, ret=%d fb_id=%d\n", ret, fb_id);
            return -1;
        }
    }
    else
    {
        ret = drmModeAddFB2WithModifiers(g_stDrmCfg.fd, width, height, drm_fmt, gem_handles, pitches,
                                        offsets, modifiers, &fb_id, DRM_MODE_FB_MODIFIERS);
        if (ret)
        {
            printf("drmModeAddFB2WithModifiers failed, ret=%d width=%d,height=%d pitches=%d offsets=%d DRM_MODE_FB_MODIFIERS=%d modifiers=%lld\n",
                    ret, width,height,pitches[0],offsets[0],DRM_MODE_FB_MODIFIERS,modifiers[0]);
            return -1;
        }
    }
    //printf("add fb:\n w=%d,h=%d,fmt=%d\n", width, height, drm_fmt);
    for(int i = 0;i < 4; i++)
    {
        if(drm_fmt  == E_STREAM_ABGR8888)
        {
            //printf("%d : handle = %d  pitches = %d  offset = %d\n", i, gem_handles[i], pitches[i], offsets[i]);
        }
    }
    drm_buf->fb_id = fb_id;
    drm_buf->gem_handle[0] = gem_handles[0];

    return fb_id;
}

static int drm_free_gem_handle(std::pair<uint32_t, uint32_t> *gem_handle) {
    // drm gem list that should be released
    int ret = -1;
    struct drm_gem_close gem_close {};
    uint32_t fb_id = gem_handle->first;
    gem_close.handle = gem_handle->second;

    if (fb_id == static_cast<uint32_t>(0) || gem_close.handle == static_cast<uint32_t>(0)) {
        return 0;
    }

    ret = drmModeRmFB(g_stDrmCfg.fd, fb_id);
    if (ret != 0) {
        printf("Failed to remove fb, ret=%d \n", ret);
    }
    ret |= drmIoctl(g_stDrmCfg.fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    if (ret != 0) {
        //printf("Failed to close gem handle %d ret=%d fb_id=%d\n", gem_close.handle,ret,fb_id);
    }
    gem_handle->first = static_cast<uint32_t>(0);
    gem_handle->second = static_cast<uint32_t>(0);
    return 0;
}

static void update_palne_dispinfo(drmModeAtomicReqPtr req, int plane_id, plane_info_t plane_info)
{
    //plane_info_t video_info;
    if(plane_id == GOP_UI_ID)
    {
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_X, plane_info.pos_x);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_Y, plane_info.pos_y);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_W, (plane_info.out_width ? plane_info.out_width : g_stDrmCfg.width));
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_H, (plane_info.out_width ? plane_info.out_height : g_stDrmCfg.height));
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].SRC_X, 0);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].SRC_Y, 0);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].SRC_W, (plane_info.out_width ? plane_info.out_width : g_stDrmCfg.width) << 16);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[1].SRC_H, (plane_info.out_height ? plane_info.out_height : g_stDrmCfg.height) << 16);
    }
	else if(plane_id == GOP_CURSOR_ID)
    {
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_X, plane_info.pos_x);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_Y, plane_info.pos_y);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_W, cursor_x_size);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_H, cursor_y_size);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].SRC_X, 0);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].SRC_Y, 0);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].SRC_W, cursor_x_size << 16);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].SRC_H, cursor_y_size << 16);
    }
    else
    {
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_X, plane_info.pos_x);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_Y, plane_info.pos_y);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_W, (plane_info.out_width ? plane_info.out_width : g_stDrmCfg.width));
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_H, (plane_info.out_height ? plane_info.out_height : g_stDrmCfg.height));
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].SRC_X, 0);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].SRC_Y, 0);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].SRC_W, (plane_info.out_width ? plane_info.out_width : g_stDrmCfg.width) << 16);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].SRC_H, (plane_info.out_height ? plane_info.out_height : g_stDrmCfg.height) << 16);

    }

}

static int atomic_set_plane(unsigned int *fb_id, unsigned int *plane_clear, unsigned int *needUpdate) {
    int ret;
    drmModeAtomicReqPtr req;

    req = drmModeAtomicAlloc();
    if (!req) {
        printf("drmModeAtomicAlloc failed \n");
        return -1;
    }
    //GOP Plane
    if(fb_id[0] > 0)
    {
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[0].FB_ID , fb_id[0]);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_ID, g_stDrmCfg.crtc_id);

        if(needUpdate[0] || !g_stDrmCfg.osd_commited)
        {
            g_stDrmCfg.osd_commited = 1;
            update_palne_dispinfo(req, GOP_UI_ID, g_CurOsdInfo);
        }
    }
    else if(plane_clear[0] == 1)
    {

        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[0].FB_ID, 0);
        add_plane_property(req, GOP_UI_ID, g_stDrmCfg.drm_mode_prop_ids[0].CRTC_ID, 0);
        plane_clear[0] = 0;  //clear once
    }
    //SOR Plane
    if(fb_id[2] > 0)
    {
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].FB_ID , fb_id[2]);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_ID, g_stDrmCfg.crtc_id);

        if(needUpdate[2])
        {
            //printf("x = %d,y = %d w = %d,h = %d\n",g_CurSorInfo.pos_x,g_CurSorInfo.pos_y,g_CurSorInfo.out_width,g_CurSorInfo.out_width);
            update_palne_dispinfo(req, GOP_CURSOR_ID, g_CurSorInfo);
        }
    }
    else if(plane_clear[2] == 1)
    {
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].FB_ID, 0);
        add_plane_property(req, GOP_CURSOR_ID, g_stDrmCfg.drm_mode_prop_ids[2].CRTC_ID, 0);
        plane_clear[2] = 0;  //clear once
    }
    //MOP Plane
    if(fb_id[1] > 0)
    {
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].FB_ID, fb_id[1]);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_ID, g_stDrmCfg.crtc_id);
        if(needUpdate[1] || !g_stDrmCfg.mop_commited)
        {
            g_stDrmCfg.mop_commited = 1;
            update_palne_dispinfo(req, MOPG_ID0, g_CurVideoInfo);
        }
    }
    else if(plane_clear[1] == 1)
    {
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].FB_ID, 0);
        add_plane_property(req, MOPG_ID0, g_stDrmCfg.drm_mode_prop_ids[1].CRTC_ID, 0);
        printf("atomic_set_plane clean mop \n");
        plane_clear[1] = 0;  //clear once
    }
    drmModeAtomicAddProperty(req, g_stDrmCfg.crtc_id, g_stDrmCfg.drm_mode_prop_ids[0].FENCE_ID, (uint64_t)&g_stDrmCfg.out_fence);//use this flag,must be close out_fence

    ret = drmModeAtomicCommit(g_stDrmCfg.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_ATOMIC_NONBLOCK , NULL);
    if(ret != 0)
    {
        printf("[atomic_set_plane]drmModeAtomicCommit failed,ret=%d out_width=%d out_height=%d video_fb_id=%d\n",
            ret, g_CurVideoInfo.out_width, g_CurVideoInfo.out_height,fb_id[1]);
    }

    drmModeAtomicFree(req);

    return 0;
}



/* User handle event */
static int mm_cal_video_size(plane_info_t *video_info, int pic_width, int pic_height)
{
    int screen_width    = 0;
    int screen_height   = 0;
    int sar_16_9_width  = 0;
    int sar_16_9_height = 0;
    int sar_4_3_width   = 0;
    int sar_4_3_height  = 0;
    int origin_width    = 0;
    int origin_height   = 0;
    int auto_width      = 0;
    int auto_height     = 0;


    double video_ratio  = 1.0 * pic_width / pic_height;     //Decode
    double panel_ratio  = 1.0 * video_info->in_width / video_info->in_height;    //actual panel

    int disp_max_width = video_info->in_width > pic_width ? pic_width:video_info->in_width;
    int disp_max_height = video_info->in_height > pic_height ? pic_height:video_info->in_height;
    double disp_max_ratio  = 1.0 * disp_max_width / disp_max_height; ;    //actual panel

    printf("disp_max_width=%d disp_max_height=%d disp_max_ratio=%f\n", disp_max_width, disp_max_height, disp_max_ratio);

    if(disp_max_width > disp_max_height) //landscape
    {
        if (panel_ratio > 1.78) {
            sar_16_9_width  = ALIGN_UP(video_info->in_height * 16 / 9, 2);
            sar_16_9_height = video_info->in_height;
        } else {
            sar_16_9_width  = video_info->in_width;
            sar_16_9_height = ALIGN_UP(video_info->in_width * 9 / 16, 2);
        }

        if (panel_ratio > 1.34) {
            sar_4_3_width  = ALIGN_UP(video_info->in_height * 4 / 3, 2);
            sar_4_3_height = video_info->in_height;
        } else {
            sar_4_3_width  = video_info->in_width;
            sar_4_3_height = ALIGN_UP(video_info->in_width * 3 / 4, 2);
        }

        if (panel_ratio / video_ratio > 1.0) {
            origin_height = MIN(pic_height, video_info->in_height);
            origin_width  = MIN(ALIGN_UP(origin_height * pic_width / pic_height, 2), video_info->in_width);
        } else {
            origin_width  = MIN(pic_width, video_info->in_width);
            origin_height = MIN(ALIGN_UP(origin_width * pic_height / pic_width, 2), video_info->in_height);
        }

        if (panel_ratio / video_ratio > 1.0) {
            auto_height = video_info->in_height;
            auto_width  = ALIGN_UP(auto_height * pic_width / pic_height, 2);
        } else {
            auto_width  = video_info->in_width;
            auto_height = ALIGN_UP(auto_width * pic_height / pic_width, 2);
        }
    }
    else    //vertical
    {

        sar_16_9_width  = disp_max_width;
        sar_16_9_height = ALIGN_UP(disp_max_width * 9 / 16, 2);

        sar_4_3_width  = disp_max_width;
        sar_4_3_height = ALIGN_UP(disp_max_width * 3 / 4, 2);

        if (disp_max_ratio / video_ratio > 1.0) {
            origin_height = MIN(pic_height, disp_max_height);
            origin_width  = MIN(ALIGN_UP(origin_height * pic_width / pic_height, 2), disp_max_width);
        } else {
            origin_width  = MIN(pic_width, disp_max_width);
            origin_height = MIN(ALIGN_UP(origin_width * pic_height / pic_width, 2), disp_max_height);
        }

        if (disp_max_ratio / video_ratio > 1.0) {
            auto_height = disp_max_height;
            auto_width  = ALIGN_UP(auto_height * pic_width / pic_height, 2);
        } else {
            auto_width  = disp_max_width;
            auto_height = ALIGN_UP(auto_width * pic_height / pic_width, 2);
        }
    }

    screen_width  = video_info->in_width;
    screen_height = video_info->in_height;

    switch (video_info->ratio) {
        case AV_ORIGIN_MODE : {
            video_info->out_width  = origin_width;
            video_info->out_height = origin_height;
            video_info->dec_width  = MIN(origin_width , pic_width);
            video_info->dec_height = MIN(origin_height, pic_height);
        }
        break;

        case AV_SAR_4_3_MODE : {
            video_info->out_width  = sar_4_3_width;
            video_info->out_height = sar_4_3_height;
            video_info->dec_width  = MIN(sar_4_3_width , pic_width);
            video_info->dec_height = MIN(sar_4_3_height, pic_height);
        }
        break;

        case AV_SAR_16_9_MODE : {
            video_info->out_width  = sar_16_9_width;
            video_info->out_height = sar_16_9_height;
            video_info->dec_width  = MIN(sar_16_9_width , pic_width);
            video_info->dec_height = MIN(sar_16_9_height, pic_height);
        }
        break;

        case AV_AUTO_MODE : {
            video_info->out_width  = auto_width;
            video_info->out_height = auto_height;
            video_info->dec_width  = MIN(auto_width , pic_width);
            video_info->dec_height = MIN(auto_height, pic_height);
        }
        break;

        case AV_SCREEN_MODE :
        default : {
            video_info->out_width  = screen_width;
            video_info->out_height = screen_height;
            video_info->dec_width  = MIN(screen_width , pic_width);
            video_info->dec_height = MIN(screen_height, pic_height);
        }
        break;
    }

    video_info->dec_width  = ALIGN_UP(video_info->dec_width, 2);
    video_info->dec_height = ALIGN_UP(video_info->dec_height, 2);

    printf("origin w/h=[%d %d], screen w/h=[%d %d], sar4:3 w/h=[%d %d], sar16:9 w/h=[%d %d], auto w/h=[%d %d]\n",
        origin_width, origin_height, screen_width, screen_height, sar_4_3_width, sar_4_3_height, sar_16_9_width, sar_16_9_height, auto_width, auto_height);
    printf("in w/h=[%d %d], dec w/h=[%d %d], rot w/h=[%d %d], out w/h=[%d %d]\n",
                video_info->in_width, video_info->in_height, video_info->dec_width, video_info->dec_height,
                video_info->rot_width, video_info->rot_height, video_info->out_width, video_info->out_height);

    if (video_info->out_width <= 0 || video_info->out_height <= 0) {
        printf("set video sar width and height error!\n");
        return -1;
    }

    return 0;
}

static int mm_cal_rotate_size(plane_info_t *video_info)
{
    if (video_info->rotate != AV_ROTATE_180 && video_info->rotate != AV_ROTATE_NONE) {
        int tmp_value;
        video_info->rot_width  = video_info->dec_height;
        video_info->rot_height = video_info->dec_width;
        tmp_value = video_info->out_width;
        video_info->out_width  = video_info->out_height;
        video_info->out_height = tmp_value;
        video_info->pos_x = MAX(0, (video_info->in_height - video_info->out_width) / 2);
        video_info->pos_y = MAX(0, (video_info->in_width - video_info->out_height) / 2);
    } else {
        video_info->rot_width  = video_info->dec_width;
        video_info->rot_height = video_info->dec_height;
        video_info->pos_x = MAX(0, (video_info->in_width - video_info->out_width) / 2);
        video_info->pos_y = MAX(0, (video_info->in_height - video_info->out_height) / 2);
    }
    printf("scaler w/h = [%d %d], dst x/y/w/h = [%d %d %d %d]\n",
        video_info->dec_width, video_info->dec_height, video_info->pos_x,
        video_info->pos_y, video_info->out_width, video_info->out_height);

    return 0;
}

static int sstar_set_ratio(int ratio)
{
    int ret;
    if(ratio > AV_AUTO_MODE)
    {
        printf("Error:sstar_set_ratio=%d fail,please check param\n",_g_MediaPlayer.video_info.ratio);
        return -1;
    }
    _g_MediaPlayer.video_info.ratio = ratio;
    printf("sstar_set_ratio=%d \n",_g_MediaPlayer.video_info.ratio);

    // set video size in user mode
    ret = mm_cal_video_size(&_g_MediaPlayer.video_info, _g_MediaPlayer.stream_width, _g_MediaPlayer.stream_height);
    if (ret < 0) {
        printf("set_video_size error!\n");
        return -1;
    }

    // set video size by rotate
    mm_cal_rotate_size(&_g_MediaPlayer.video_info);
    printf("mm_set_video_size  rotate=%d dec_width=%d dec_height=%d x=%d y=%d out_width=%d out_height=%d.\n",_g_MediaPlayer.video_info.rotate, _g_MediaPlayer.video_info.dec_width, _g_MediaPlayer.video_info.dec_height,
        _g_MediaPlayer.video_info.pos_x, _g_MediaPlayer.video_info.pos_y,_g_MediaPlayer.video_info.out_width, _g_MediaPlayer.video_info.out_height);

    mm_player_set_dec_out_attr(_g_MediaPlayer.video_info.dec_width, _g_MediaPlayer.video_info.dec_height);
    return 0;
}

static int mm_set_video_size(seq_info_t *seq_info)
{
    int strm_width = 0;
    int strm_height = 0;

    if (seq_info->crop_width > 0 && seq_info->crop_height > 0) {
        strm_width = seq_info->crop_width;
        strm_height = seq_info->crop_height;
    } else {
        strm_width = seq_info->pic_width;
        strm_height = seq_info->pic_height;
    }

    // reset in width/height
    _g_MediaPlayer.stream_width = strm_width;
    _g_MediaPlayer.stream_height = strm_height;

    sstar_set_ratio(_g_MediaPlayer.video_info.ratio);
    return 0;
}


static void mm_video_event_handle(event_info_t *event_info)
{
    if (!event_info) {
        printf("get event info null ptr\n");
        return;
    }

    switch (event_info->event)
    {
        case PLAYBACK_EVENT_DECERR:
            printf("get dec err event...\n");
            break;

        case PLAYBACK_EVENT_STREAMEND:
            printf("get eos event ...\n");
            break;

        case PLAYBACK_EVENT_SEQCHANGE:
            //printf("get seq change event...\n");
            printf("get seq info wh(%d %d), crop(%d %d %d %d)\n",
                   event_info->seq_info.pic_width, event_info->seq_info.pic_height,
                   event_info->seq_info.crop_x, event_info->seq_info.crop_y,
                   event_info->seq_info.crop_width, event_info->seq_info.crop_height);
            memcpy(&_g_MediaPlayer.video_info.seq_info, &event_info->seq_info, sizeof(seq_info_t));
            mm_set_video_size(&event_info->seq_info);
            break;

        case PLAYBACK_EVENT_DROPPED:
            printf("get frame drop event...\n");
            break;
        case PLAYBACK_EVENT_STARTED:
            _g_MediaPlayer.player_working = true;
            printf("PLAYBACK_EVENT_STARTED\n");
            break;
        case PLAYBACK_EVENT_STOPED:
            _g_MediaPlayer.player_working = false;
            printf("PLAYBACK_EVENT_STOPED\n");
            break;


        case PLAYBACK_EVENT_DECDONE:
            //printf("get decframe done event...\n");
            break;

        default:
            printf("event type(%d) \n", event_info->event);
            break;
    }
}

static MI_S32 sstar_init_drm()
{
    int ret;
    /************************************************
    step :init DRM(DISP)
    *************************************************/
    g_stDrmCfg.has_modifiers = 1;
    g_stDrmCfg.commit_Mutex  = PTHREAD_MUTEX_INITIALIZER;

    g_stDrmCfg.fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if(g_stDrmCfg.fd < 0)
    {
        printf("Open dri/card0 fail \n");
        return -1;
    }

    ret = drmSetClientCap(g_stDrmCfg.fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_ATOMIC fail ret=%d  \n",ret);
    }

    ret = drmSetClientCap(g_stDrmCfg.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if(ret != 0)
    {
        printf("drmSetClientCap DRM_CLIENT_CAP_UNIVERSAL_PLANES fail ret=%d  \n",ret);
    }

    g_stDrmCfg.pDrmModeRes = drmModeGetResources(g_stDrmCfg.fd);
    g_stDrmCfg.crtc_id = g_stDrmCfg.pDrmModeRes->crtcs[0];
    g_stDrmCfg.conn_id = g_stDrmCfg.pDrmModeRes->connectors[0];

    g_stDrmCfg.Connector = drmModeGetConnector(g_stDrmCfg.fd, g_stDrmCfg.conn_id);
	if (g_stDrmCfg.Connector->connector_type == DRM_MODE_CONNECTOR_DSI)//MIPI
	{
	    printf("This is MIPI Panel \n");
	}
    else
    {

	    printf("This is TTL Panel \n");
    }
    g_stDrmCfg.width = g_stDrmCfg.Connector->modes[0].hdisplay;
    g_stDrmCfg.height = g_stDrmCfg.Connector->modes[0].vdisplay;

    init_drm_property_ids(GOP_UI_ID, &g_stDrmCfg.drm_mode_prop_ids[0]);
    init_drm_property_ids(GOP_CURSOR_ID, &g_stDrmCfg.drm_mode_prop_ids[2]);
    init_drm_property_ids(MOPG_ID0, &g_stDrmCfg.drm_mode_prop_ids[1]);

    return 0;

}

static void sstar_deinit_drm()
{

    if(g_stDrmCfg.Connector != NULL)
    {
    	drmModeFreeConnector(g_stDrmCfg.Connector);
        g_stDrmCfg.Connector = NULL;
    }
    if(g_stDrmCfg.pDrmModeRes != NULL)
    {
    	drmModeFreeResources(g_stDrmCfg.pDrmModeRes);
        g_stDrmCfg.pDrmModeRes = NULL;
    }

    //destory property blob
    if(g_stDrmCfg.blob_id)
    {
        drmModeDestroyPropertyBlob(g_stDrmCfg.fd, g_stDrmCfg.blob_id);
    }

    if(g_stDrmCfg.fd)
    {
        close(g_stDrmCfg.fd);
    }

}


static MI_S32 sstar_sys_init()
{
    /************************************************
    step :init SYS
    *************************************************/
    MI_U64           u64Pts = 0;
    MI_SYS_Version_t stVersion;
    struct timeval   curStamp;
    STCHECKRESULT(MI_SYS_Init(0));
    STCHECKRESULT(MI_SYS_GetVersion(0, &stVersion));
    printf("Get MI_SYS_Version:%s\n", stVersion.u8Version);

    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    STCHECKRESULT(MI_SYS_InitPtsBase(0, u64Pts));

    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    STCHECKRESULT(MI_SYS_SyncPts(0, u64Pts));

    STCHECKRESULT(MI_SYS_GetCurPts(0, &u64Pts));
    printf("%s:%d Get MI_SYS_CurPts:0x%llx(%ds,%dus)\n", __func__, __LINE__, u64Pts, (int)curStamp.tv_sec,
           (int)curStamp.tv_usec);
    //***********************************************
    //end init sys
    return MI_SUCCESS;
}


static MI_S32 sstar_scl_init()
{
    MI_U32 u32SclChnId = _g_HdmiRxPlayer.stSclChnPort.u32ChnId;
    MI_U32 u32SclPortId = _g_HdmiRxPlayer.stSclChnPort.u32PortId;
    MI_U32 u32SclDevId = _g_HdmiRxPlayer.stSclChnPort.u32DevId;


    MI_SCL_DevAttr_t stSclDevAttr;
    MI_SCL_ChannelAttr_t stSclChnAttr;
    MI_SCL_ChnParam_t stSclChnParam;
    MI_SYS_WindowRect_t stSclInputCrop;
    MI_SCL_OutPortParam_t  stSclOutPortParam;

    memset(&stSclDevAttr, 0x00, sizeof(MI_SCL_DevAttr_t));
    stSclDevAttr.u32NeedUseHWOutPortMask = E_MI_SCL_HWSCL0;//E_MI_SCL_HWSCL1 is already user in ssplayer for rotate

    STCHECKRESULT(MI_SCL_CreateDevice(u32SclDevId, &stSclDevAttr));

    memset(&stSclChnAttr, 0x00, sizeof(MI_SCL_ChannelAttr_t));
    memset(&stSclInputCrop, 0x00, sizeof(MI_SYS_WindowRect_t));
    memset(&stSclChnParam, 0x00, sizeof(MI_SCL_ChnParam_t));

    STCHECKRESULT(MI_SCL_CreateChannel(u32SclDevId, u32SclChnId, &stSclChnAttr));
    STCHECKRESULT(MI_SCL_SetChnParam(u32SclDevId, u32SclChnId, &stSclChnParam));

    STCHECKRESULT(MI_SCL_StartChannel(u32SclDevId, u32SclChnId));

    memset(&stSclOutPortParam, 0x0, sizeof(MI_SCL_OutPortParam_t));
    stSclOutPortParam.ePixelFormat = (MI_SYS_PixelFormat_e) E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stSclOutPortParam.stSCLOutputSize.u16Width= VEDIO_WIDTH;
    stSclOutPortParam.stSCLOutputSize.u16Height= VEDIO_HEIGHT;

    stSclOutPortParam.bMirror = FALSE;
    stSclOutPortParam.bFlip = FALSE;
    stSclOutPortParam.eCompressMode = (MI_SYS_CompressMode_e)E_MI_SYS_COMPRESS_MODE_NONE;

    STCHECKRESULT(MI_SCL_SetOutputPortParam(u32SclDevId, u32SclChnId, u32SclPortId, &stSclOutPortParam));
    STCHECKRESULT(MI_SCL_EnableOutputPort(u32SclDevId, u32SclChnId, u32SclPortId));


    MI_SYS_ChnPort_t stSclChnPort;
    memset(&stSclChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort.eModId    = E_MI_MODULE_ID_SCL;
    stSclChnPort.u32DevId  = u32SclDevId;
    stSclChnPort.u32ChnId  = u32SclChnId;
    stSclChnPort.u32PortId = u32SclPortId;

    STCHECKRESULT(MI_SYS_CreateChnOutputPortDmabufCusAllocator(&stSclChnPort));

    return MI_SUCCESS;

}
static MI_S32 sstar_scl_deinit()
{
    MI_U32 u32SclChnId = _g_HdmiRxPlayer.stSclChnPort.u32ChnId;
    MI_U32 u32SclPortId = _g_HdmiRxPlayer.stSclChnPort.u32PortId;
    MI_U32 u32SclDevId = _g_HdmiRxPlayer.stSclChnPort.u32DevId;
    MI_SYS_ChnPort_t stSclChnPort;

    memset(&stSclChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort.eModId    = E_MI_MODULE_ID_SCL;
    stSclChnPort.u32DevId  = u32SclDevId;
    stSclChnPort.u32ChnId  = u32SclChnId;
    stSclChnPort.u32PortId = u32SclPortId;
    STCHECKRESULT(MI_SYS_DestroyChnOutputPortDmabufCusAllocator(&stSclChnPort));

    STCHECKRESULT(MI_SCL_DisableOutputPort(u32SclDevId, u32SclChnId, u32SclPortId));
    STCHECKRESULT(MI_SCL_StopChannel(u32SclDevId, u32SclChnId));
    STCHECKRESULT(MI_SCL_DestroyChannel(u32SclDevId, u32SclChnId));
    STCHECKRESULT(MI_SCL_DestroyDevice(u32SclDevId));
    return MI_SUCCESS;
}

static int sstar_OsdList_Init() //for osd
{
    int i = 0;
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);

    init_list_head(&g_HeadListOsdOutput);
    init_list_head(&g_HeadListOsdCommit);

    for(i = 0; i < BUF_DEPTH  ; i++)
    {
        _g_OsdListEntryNode[i] = new St_ListNode_t;
        if(_g_OsdListEntryNode[i] == NULL)
        {
            printf("_g_OsdListEntryNode  malloc Error!\n");
            pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
            return -1;
        }
        _g_OsdListEntryNode[i]->pGraphicBuffer = NULL;
        _g_OsdListEntryNode[i]->EntryMutex = PTHREAD_MUTEX_INITIALIZER;
        add_tail_node(&g_HeadListOsdOutput, &_g_OsdListEntryNode[i]->list);

    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
    return 0;

}

static int sstar_CursorList_Init() //for osd
{
    int i = 0;
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);


    init_list_head(&g_HeadListCursorOutput);
    init_list_head(&g_HeadListCursorCommit);

    for(i = 0; i < BUF_DEPTH  ; i++)
    {
        _g_CursorListEntryNode[i] = new St_ListNode_t;
        if(_g_CursorListEntryNode[i] == NULL)
        {
            printf("_g_CursorListEntryNode  malloc Error!\n");
            pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
            return -1;
        }
        _g_CursorListEntryNode[i]->pGraphicBuffer = NULL;
        _g_CursorListEntryNode[i]->EntryMutex = PTHREAD_MUTEX_INITIALIZER;
        add_tail_node(&g_HeadListCursorOutput, &_g_CursorListEntryNode[i]->list);

    }

    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
    return 0;

}

static void sstar_CursorList_DeInit() //for video
{
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);
    clean_list_node(&g_HeadListCursorCommit);
    clean_list_node(&g_HeadListCursorOutput);
    for(int i = 0; i < BUF_DEPTH  ; i++)
    {
        if(_g_CursorListEntryNode[i])
        {
            delete _g_CursorListEntryNode[i];
            _g_CursorListEntryNode[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
}


static void sstar_OsdList_DeInit() //for video
{
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);
    clean_list_node(&g_HeadListOsdOutput);
    for(int i = 0; i < BUF_DEPTH  ; i++)
    {
        if(_g_OsdListEntryNode[i])
        {
            delete _g_OsdListEntryNode[i];
            _g_OsdListEntryNode[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
}

static int sstar_MediaList_Init() //for video
{
    int i = 0;

    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);

    init_list_head(&g_HeadListVideoOutput);
    init_list_head(&g_HeadListVideoCommit);

    for(i = 0; i < BUF_DEPTH  ; i++)
    {
        _g_VideoListEntryNode[i] = new St_ListNode_t;
        if(_g_VideoListEntryNode[i] == NULL)
        {
            printf("_g_OsdListEntryNode || _g_VideoListEntryNode malloc Error!\n");
            pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
            return -1;
        }
        _g_VideoListEntryNode[i]->pGraphicBuffer = NULL;
        _g_VideoListEntryNode[i]->EntryMutex = PTHREAD_MUTEX_INITIALIZER;

        add_tail_node(&g_HeadListVideoOutput, &_g_VideoListEntryNode[i]->list);
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
    return 0;
}
static void sstar_MediaList_DeInit() //for video
{
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);
    printf("sstar_MediaList_DeInits\n");
    clean_list_node(&g_HeadListVideoOutput);
    clean_list_node(&g_HeadListVideoCommit);
    for(int i = 0; i < BUF_DEPTH  ; i++)
    {
        if(_g_VideoListEntryNode[i])
        {
            delete _g_VideoListEntryNode[i];
            _g_VideoListEntryNode[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
}

static int sstar_HdmiList_Init() //for hdmi
{
    int i;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;
    int count_fail = 0;;
    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);

    init_list_head(&g_HeadListSclOutput);
    init_list_head(&g_HeadListGpuGfxInput);
    init_list_head(&g_HeadListVideoOutput);
    init_list_head(&g_HeadListVideoCommit);

    for(i = 0; i < BUF_DEPTH  ; i++)
    {
        _g_VideoListEntryNode[i] = new St_ListNode_t;
        if( _g_VideoListEntryNode[i] == NULL)
        {
            printf("_g_VideoListEntryNode malloc Error!\n");
            return -1;
        }
        _g_VideoListEntryNode[i]->pGraphicBuffer = NULL;//g_VideoOutputBuffer[i];
        _g_VideoListEntryNode[i]->EntryMutex = PTHREAD_MUTEX_INITIALIZER;
        add_tail_node(&g_HeadListVideoOutput, &_g_VideoListEntryNode[i]->list);

    }

    for(i = 0; i < BUF_DEPTH  ; i++)//for scl outbuf enqueue
    {

        _g_GfxInputEntryNode[i] = new St_ListNode_t;
        if(_g_GfxInputEntryNode[i] == NULL)
        {
            printf("_g_GfxInputEntryNode malloc Error!\n");
            return -1;
        }

        _g_GfxInputBuffer[i] = std::make_shared<GpuGraphicBuffer>(VEDIO_WIDTH, VEDIO_HEIGHT, u32GpuInputFormat, ALIGN_UP(VEDIO_WIDTH, 64));
        if (!_g_GfxInputBuffer[i]->initCheck())
        {
            printf("Create GpuGraphicBuffer failed\n");
            count_fail++;
            continue;
        }

        _g_GfxInputEntryNode[i]->pGraphicBuffer = _g_GfxInputBuffer[i];
        _g_GfxInputEntryNode[i]->EntryMutex = PTHREAD_MUTEX_INITIALIZER;
        add_tail_node(&g_HeadListSclOutput, &_g_GfxInputEntryNode[i]->list);

    }
    if( count_fail >= BUF_DEPTH )
    {
        printf("count_fail=%d \n",count_fail);
        pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
        return -1;
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
    return 0;
}

static void sstar_HdmiList_DeInit() //for video
{
    unsigned long eTime1;
    unsigned long eTime2;
    unsigned long eTime3;

    struct timeval timeEnqueue1;

    printf("sstar_HdmiList_DeInit\n");
    gettimeofday(&timeEnqueue1, NULL);
    eTime1 = timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;

    pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);

    gettimeofday(&timeEnqueue1, NULL);
    eTime2 = timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
    {
        printf("sstar_HdmiList_DeInit 1 time=%lu \n",(eTime2 - eTime1));
    }

    clean_list_node(&g_HeadListVideoCommit);
    clean_list_node(&g_HeadListVideoOutput);
    clean_list_node(&g_HeadListGpuGfxInput);
    clean_list_node(&g_HeadListSclOutput);
    for(int i = 0; i < BUF_DEPTH  ; i++)
    {
        if(_g_VideoListEntryNode[i])
        {
            delete _g_VideoListEntryNode[i];
            _g_VideoListEntryNode[i] = NULL;
        }
        if(_g_GfxInputEntryNode[i])
        {
            delete _g_GfxInputEntryNode[i];
            _g_VideoListEntryNode[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);

    gettimeofday(&timeEnqueue1, NULL);
    eTime3 = timeEnqueue1.tv_sec*1000 + timeEnqueue1.tv_usec/1000;
    {
        printf("sstar_HdmiList_DeInit 2 time=%lu \n",(eTime3 - eTime2));
    }
}

static MI_S32 sstar_update_pointoffset()
{
    MI_BOOL bUpdataGpuGfxPointFail = false;

    if(g_stdOsdGpuGfx != NULL)
    {
        pthread_mutex_lock(&g_OsdProcessMutex);
        //Update osd Pointoffset
        if (!g_stdOsdGpuGfx->updateLTPointOffset(g_u32GpuLT_X+1, g_u32GpuLT_Y+1))
        {
            printf("Failed to set osd keystone LT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdOsdGpuGfx->updateLBPointOffset(g_u32GpuLB_X+1, g_u32GpuLB_Y+1))
        {
            printf("Failed to set osd keystone LB correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdOsdGpuGfx->updateRTPointOffset(g_u32GpuRT_X+1, g_u32GpuRT_Y+1))
        {
            printf("Failed to set osd keystone RT correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdOsdGpuGfx->updateRBPointOffset(g_u32GpuRB_X+1, g_u32GpuRB_Y+1))
        {
            printf("Failed to set osd keystone RB correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        pthread_mutex_unlock(&g_OsdProcessMutex);
    }

    if(g_stdSorGpuGfx != NULL)
    {
        pthread_mutex_lock(&g_SorProcessMutex);
        //Update osd Pointoffset
        if (!g_stdSorGpuGfx->updateLTPointOffset(g_u32GpuLT_X, g_u32GpuLT_Y))
        {
            printf("Failed to set osd keystone LT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdSorGpuGfx->updateLBPointOffset(g_u32GpuLB_X, g_u32GpuLB_Y))
        {
            printf("Failed to set osd keystone LB correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdSorGpuGfx->updateRTPointOffset(g_u32GpuRT_X, g_u32GpuRT_Y))
        {
            printf("Failed to set osd keystone RT correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdSorGpuGfx->updateRBPointOffset(g_u32GpuRB_X, g_u32GpuRB_Y))
        {
            printf("Failed to set osd keystone RB correction points\n");
            bUpdataGpuGfxPointFail = true;
        }
        pthread_mutex_unlock(&g_SorProcessMutex);
    }

    if(g_stdVideoGpuGfx != NULL )
    {

        //Update video Pointoffset
        pthread_mutex_lock(&g_VideoProcessMutex);
        if (!g_stdVideoGpuGfx->updateLTPointOffset(g_u32GpuLT_X, g_u32GpuLT_Y))
        {
            printf("Failed to set keystone LT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdVideoGpuGfx->updateLBPointOffset(g_u32GpuLB_X, g_u32GpuLB_Y))
        {
            printf("Failed to set keystone LB correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdVideoGpuGfx->updateRTPointOffset(g_u32GpuRT_X, g_u32GpuRT_Y))
        {
            printf("Failed to set keystone RT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdVideoGpuGfx->updateRBPointOffset(g_u32GpuRB_X, g_u32GpuRB_Y))
        {
            printf("Failed to set keystone RB correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        pthread_mutex_unlock(&g_VideoProcessMutex);
    }

    if(g_stdHdmiRxGpuGfx != NULL )
    {

        //Update hdmirx Pointoffset
        pthread_mutex_lock(&g_HdmiRXProcessMutex);
        if (!g_stdHdmiRxGpuGfx->updateLTPointOffset(g_u32GpuLT_X, g_u32GpuLT_Y))
        {
            printf("Failed to set keystone LT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdHdmiRxGpuGfx->updateLBPointOffset(g_u32GpuLB_X, g_u32GpuLB_Y))
        {
            printf("Failed to set keystone LB correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdHdmiRxGpuGfx->updateRTPointOffset(g_u32GpuRT_X, g_u32GpuRT_Y))
        {
            printf("Failed to set keystone RT correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        if (!g_stdHdmiRxGpuGfx->updateRBPointOffset(g_u32GpuRB_X, g_u32GpuRB_Y))
        {
            printf("Failed to set keystone RB correction points g_u32GpuLT_X=%d g_u32GpuLT_Y=%d\n",g_u32GpuLT_X,g_u32GpuLT_Y);
            bUpdataGpuGfxPointFail = true;
        }
        pthread_mutex_unlock(&g_HdmiRXProcessMutex);
    }


    if (bUpdataGpuGfxPointFail == false)
    {
        printf("Updata Keystone info done:\n");
        printf("==================================\n");
        printf("LTPointOffset X:%d, Y:%d\n", g_u32GpuLT_X, g_u32GpuLT_Y);
        printf("LBPointOffset X:%d, Y:%d\n", g_u32GpuLB_X, g_u32GpuLB_Y);
        printf("RTPointOffset X:%d, Y:%d\n", g_u32GpuRT_X, g_u32GpuRT_Y);
        printf("RBPointOffset X:%d, Y:%d\n", g_u32GpuRB_X, g_u32GpuRB_Y);
        printf("==================================\n");

        if(!g_u32GpuLT_X && !g_u32GpuLT_Y && !g_u32GpuLB_X && !g_u32GpuLB_Y &&
            !g_u32GpuRT_X && !g_u32GpuRT_Y && !g_u32GpuRB_X && !g_u32GpuRB_Y && !_g_rotate_mode)
        {
            if(g_bGpuProcessNeed == true)//change to no keustone
            {
                g_bGpuProcessNeed = false;
            }
        }
        else
        {
            if(g_bGpuProcessNeed == false)//change to keustone
            {
                g_bGpuProcessNeed = true;
            }
        }
    }
    return MI_SUCCESS;
}


static std::shared_ptr<GpuGraphicEffect> sstar_gpugfx_context_init(uint32_t u32Width, uint32_t u32Height, uint32_t u32GpuInputFormat, bool isModifier)
{
    std::shared_ptr<GpuGraphicEffect> stGpuGfx;

    stGpuGfx = std::make_shared<GpuGraphicEffect>();
    stGpuGfx->init(u32Width, u32Height, u32GpuInputFormat, isModifier);

    if(_g_rotate_mode == 1)
    {
        stGpuGfx->updateTransformStatus(Transform :: ROT_90);
    }
    else if(_g_rotate_mode == 2)
    {
        stGpuGfx->updateTransformStatus(Transform :: ROT_180);
    }
    else if(_g_rotate_mode == 3)
    {
        stGpuGfx->updateTransformStatus(Transform :: ROT_270);
    }
    if (!stGpuGfx->updateLTPointOffset(g_u32GpuLT_X, g_u32GpuLT_Y) || !stGpuGfx->updateLBPointOffset(g_u32GpuLB_X, g_u32GpuLB_Y)
        || !stGpuGfx->updateRTPointOffset(g_u32GpuRT_X, g_u32GpuRT_Y) || !stGpuGfx->updateRBPointOffset(g_u32GpuRB_X, g_u32GpuRB_Y))
    {
        printf("Failed to set keystone correction points\n");
        //return NULL;
    }
    else
    {

        printf("=========success=========================\n");
        printf("LTPointOffset X:%d, Y:%d\n", g_u32GpuLT_X, g_u32GpuLT_Y);
        printf("LBPointOffset X:%d, Y:%d\n", g_u32GpuLB_X, g_u32GpuLB_Y);
        printf("RTPointOffset X:%d, Y:%d\n", g_u32GpuRT_X, g_u32GpuRT_Y);
        printf("RBPointOffset X:%d, Y:%d\n", g_u32GpuRB_X, g_u32GpuRB_Y);
        printf("==================================\n");
    }
    return stGpuGfx;
}

static void sstar_hdmirx_context_init()
{
    memset(&_g_HdmiRxPlayer, 0x00, sizeof(hdmirx_player_t));
    _g_HdmiRxPlayer.u8HdmirxPort = E_MI_HDMIRX_PORT0;
    _g_HdmiRxPlayer.stHvpChnPort.eModId = E_MI_MODULE_ID_HVP;
    _g_HdmiRxPlayer.stHvpChnPort.u32DevId = 0;
    _g_HdmiRxPlayer.stHvpChnPort.u32ChnId = 0;
    _g_HdmiRxPlayer.stHvpChnPort.u32PortId = 0;

    _g_HdmiRxPlayer.stSclChnPort.eModId = E_MI_MODULE_ID_SCL;
    _g_HdmiRxPlayer.stSclChnPort.u32DevId = 8;
    _g_HdmiRxPlayer.stSclChnPort.u32ChnId = 0;
    _g_HdmiRxPlayer.stSclChnPort.u32PortId =0;
    _g_HdmiRxPlayer.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;

    _g_HdmiRxPlayer.eSignalStatus = E_MI_HDMIRX_SIG_NO_SIGNAL;
    _g_HdmiRxPlayer.eHvpSignalStatus = E_MI_HVP_SIGNAL_UNSTABLE;

    _g_HdmiRxPlayer.hvpFrameRate = 60;
    _g_HdmiRxPlayer.sclFrameRate = 60;
}

static void sstar_media_context_init()
{
    // set default ratio mode
    //_g_MediaPlayer.video_info.ratio = (int)AV_SAR_16_9_MODE;
    _g_MediaPlayer.param.protocol = AV_PROTOCOL_FILE;

    // set default disp windows size
    _g_MediaPlayer.param.url = g_InputFile;


    if (_g_rotate_mode == AV_ROTATE_NONE || _g_rotate_mode == AV_ROTATE_180)
    {
        _g_MediaPlayer.video_info.in_width  = g_stDrmCfg.width;
        _g_MediaPlayer.video_info.in_height = g_stDrmCfg.height;

    } else
    {
        _g_MediaPlayer.video_info.in_width  = g_stDrmCfg.height;
        _g_MediaPlayer.video_info.in_height = g_stDrmCfg.width;
    }

    _g_MediaPlayer.param.panel_width = VEDIO_WIDTH;
    _g_MediaPlayer.param.panel_height = VEDIO_HEIGHT;

    mm_player_set_opts("audio_format", "", AV_PCM_FMT_S16);
    mm_player_set_opts("audio_channels", "", 2);
    mm_player_set_opts("audio_samplerate", "", 48000);
    mm_player_set_opts("video_bypass", "", 0);
    mm_player_set_opts("audio_bypass", "", 0);

    mm_player_set_opts("play_mode", "", _g_MediaPlayer.loop_mode);
    //mm_player_set_opts("video_only", "", 1);
    //mm_player_set_opts("video_rotate", "", _g_MediaPlayer.video_info.rotate);//do not use ssplayer rotate,use gpu if you need
    mm_player_set_opts("video_ratio", "", _g_MediaPlayer.video_info.ratio);
    mm_player_register_event_cb(mm_video_event_handle);

    printf("ssplayer video_rotate=%d _g_rotate_mode=%d url:%s panel_height=%d panel_width=%d\n",_g_MediaPlayer.video_info.rotate,_g_rotate_mode, _g_MediaPlayer.param.url,_g_MediaPlayer.param.panel_height,_g_MediaPlayer.param.panel_width);

}

static std::shared_ptr<GpuGraphicBuffer> sstar_canvas_init(unsigned int width, unsigned int height)
{
    std::shared_ptr<GpuGraphicBuffer> pGpuCanvasBuf;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;
    if(g_stdOsdGpuRender == NULL)
    {
        g_stdOsdGpuRender = std::make_shared<GpuGraphicRender>();
        g_stdOsdGpuRender->init(u32UiFormat);
    }
    pGpuCanvasBuf = std::make_shared<GpuGraphicBuffer>(width, height, DRM_FORMAT_ABGR8888, ALIGN_UP(width * 4, 64));
    if (!pGpuCanvasBuf->initCheck()) {
        printf("Create _g_MainCanvas failed\n");
        return NULL;
    }
    return pGpuCanvasBuf;
}
static void sstar_canvas_deinit()
{
    _g_MainCanvas.pIsCreated = false;
    _g_MainCanvas.mainFbCanvas = NULL;
    if(g_stdOsdGpuRender != NULL)
    {
        g_stdOsdGpuRender->deinit();
    }
}

static std::shared_ptr<GpuGraphicBuffer> sstar_sorcanvas_init(unsigned int width, unsigned int height)
{
    std::shared_ptr<GpuGraphicBuffer> pGpuCanvasBuf;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;

    if(g_stdSorGpuRender == NULL)
    {
        g_stdSorGpuRender = std::make_shared<GpuGraphicRender>();
        g_stdSorGpuRender->init(u32UiFormat);
    }
    pGpuCanvasBuf = std::make_shared<GpuGraphicBuffer>(width, height, DRM_FORMAT_ABGR8888, ALIGN_UP(width * 4, 64));
    if (!pGpuCanvasBuf->initCheck()) {
        printf("Create _g_MainCanvas failed\n");
        return NULL;
    }
    return pGpuCanvasBuf;
}

static void sstar_sorcanvas_deinit()
{
    _g_MainSorCanvas.mainFbCanvas = NULL;
    if(g_stdSorGpuRender != NULL)
    {
        g_stdSorGpuRender->deinit();
    }
}

void sstar_clear_plane(int plane)
{
    St_ListNode_t *entryNode;
    entryNode = new St_ListNode_t;
    entryNode->pGraphicBuffer = NULL;
    entryNode->EntryMutex = PTHREAD_MUTEX_INITIALIZER;
    if(plane == GOP_UI_ID)
    {
        printf("Clear osd \n");
        add_tail_node(&g_HeadListOsdCommit, &entryNode->list);

    }
    else if(plane == MOPG_ID0)
    {
        printf("Clear mop \n");
        add_tail_node(&g_HeadListVideoCommit, &entryNode->list);
    }
    else
    {
        printf("Clear cursor \n");
        add_tail_node(&g_HeadListCursorCommit, &entryNode->list);
    }
}


void *sstar_PointOffsetMoniter_Thread(void * arg)
{
    while(g_bThreadExitUpdatePoint)
    {
        if(g_bChangePointOffset)
        {
            sstar_update_pointoffset();
            g_bChangePointOffset = false;
        }
        else
        {
            usleep(10 * 1000);
            continue;
        }
    }
    printf("EXIT sstar_PointOffsetMoniter_Thread \n");
    return NULL;
}

static void sstar_put_videobuf(    St_ListNode_t *ListVideoCommit)
{
    if(g_TemFrameInfo.dma_buf_fd[0] != 0)
    {
        mm_player_put_video_frame(&g_TemFrameInfo);
        memset(&g_TemFrameInfo, 0x00, sizeof(frame_info_t));
    }
    if(g_stdVideoGpuGfx || g_stdGfxGpuRender || g_stdHdmiRxGpuGfx)
    {
        g_CurGraphicBuffer = ListVideoCommit->pGraphicBuffer; //hold the reference
        mm_player_put_video_frame(&ListVideoCommit->frame_info);
    }
    else
    {
        memcpy(&g_TemFrameInfo, &ListVideoCommit->frame_info, sizeof(frame_info_t));
    }
}

#ifdef USE_GPUGFX_ROCESS

#define MAKE_NV12_VALUE(y,u,v,a)  ((y) << 24) | ((u) << 16) | ((v) << 8) | (a)
#define YUYV_BLACK              MAKE_NV12_VALUE(0,128,128,0)
#define YUYV_RED                MAKE_NV12_VALUE(76,84,255,0)
#define YUYV_BLUE               MAKE_NV12_VALUE(29,225,107,0)

static std::shared_ptr<GpuGraphicBuffer> sstar_frame_process(std::shared_ptr<GpuGraphicBuffer> inputBuffer)
{
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;
    std::shared_ptr<GpuGraphicBuffer>   pDstGraphicBuffer;
    GrFillInfo dstFillInfo;
    int ret;
    if(!g_stdGfxGpuRender)
    {
        g_stdGfxGpuRender = std::make_shared<GpuGraphicRender>();
        g_stdGfxGpuRender->init(u32GpuInputFormat);
        memset(&g_CurVideoInfo, 0x00, sizeof(plane_info_t));
    }

    pDstGraphicBuffer = std::make_shared<GpuGraphicBuffer>(g_stDrmCfg.width, g_stDrmCfg.height, u32GpuInputFormat, ALIGN_UP(g_stDrmCfg.width, 64));
    if (!pDstGraphicBuffer->initCheck())
    {
        printf("Error Create GpuGraphicBuffer failed\n");
        return NULL;
    }

    dstFillInfo.rect.left = 0;
    dstFillInfo.rect.top = 0;
    dstFillInfo.rect.right = g_stDrmCfg.width;
    dstFillInfo.rect.bottom = g_stDrmCfg.height;
    dstFillInfo.color = YUYV_BLACK;

    g_stdGfxGpuRender->fill(pDstGraphicBuffer, dstFillInfo);

    GrBitblitInfo bitblitinfo;
    bitblitinfo.transfrom = NONE;
    bitblitinfo.srcRect.left = 0;
    bitblitinfo.srcRect.top = 0;
    bitblitinfo.srcRect.right = inputBuffer->getWidth();
    bitblitinfo.srcRect.bottom = inputBuffer->getHeight();

    bitblitinfo.dstRect.left = _g_MediaPlayer.video_info.pos_x;
    bitblitinfo.dstRect.top = _g_MediaPlayer.video_info.pos_y;

    if((_g_MediaPlayer.video_info.pos_x + _g_MediaPlayer.video_info.out_width) > (int)g_stDrmCfg.width)
    {
        bitblitinfo.dstRect.right = g_stDrmCfg.width;
    }
    else
    {
        bitblitinfo.dstRect.right = _g_MediaPlayer.video_info.pos_y + _g_MediaPlayer.video_info.out_width;
    }


    if((_g_MediaPlayer.video_info.pos_y + _g_MediaPlayer.video_info.out_height) > (int)g_stDrmCfg.height)
    {
        bitblitinfo.dstRect.bottom = g_stDrmCfg.height;
    }
    else
    {
        bitblitinfo.dstRect.bottom = _g_MediaPlayer.video_info.pos_y + _g_MediaPlayer.video_info.out_height;
    }
    //printf("src[%d,%d,%d,%d] dst[%d,%d,%d,%d] color=%d\n",bitblitinfo.srcRect.left,bitblitinfo.srcRect.top,bitblitinfo.srcRect.right,bitblitinfo.srcRect.bottom,
    //    bitblitinfo.dstRect.left,bitblitinfo.dstRect.top,bitblitinfo.dstRect.right,bitblitinfo.dstRect.bottom,dstFillInfo.color);

    ret = g_stdGfxGpuRender->bitblit(inputBuffer, pDstGraphicBuffer, bitblitinfo);
    if (ret) {
        printf("Video:Gpu graphicbitblits failed,IgetBufferSize=%d getBufferSize=%d\n", inputBuffer->getBufferSize(), pDstGraphicBuffer->getBufferSize());
        return NULL;
    }
    return pDstGraphicBuffer;
}

#endif


void *sstar_DrmCommit_Thread(void * arg)
{
    drm_buf_t osd_drm_buf;
    drm_buf_t sor_drm_buf;
    drm_buf_t video_drm_buf;
    St_ListNode_t *ListOsdCommit = NULL;
    St_ListNode_t *ListCursorCommit = NULL;
    St_ListNode_t *ListVideoCommit = NULL;
    int  try_count = 5;//timeout 5ms
    std::shared_ptr<GpuGraphicBuffer> osdCommitBuf;
    std::shared_ptr<GpuGraphicBuffer> cusorCommitBuf;
    std::shared_ptr<GpuGraphicBuffer> videoCommitBuf;
    memset(&osd_drm_buf, 0x00, sizeof(drm_buf_t));
    memset(&video_drm_buf, 0x00, sizeof(drm_buf_t));
    int ret = 0;
    while(g_bThreadExitCommit)
    {
        pthread_mutex_lock(&g_stDrmCfg.commit_Mutex);
        if(_g_MainCanvas.pIsCreated)
        {
            ListOsdCommit = get_first_node(&g_HeadListOsdCommit);
            if(ListOsdCommit)
            {
                osdCommitBuf = ListOsdCommit->pGraphicBuffer;
                if(osdCommitBuf != NULL)
                {
                    sstar_add_commitbuf(osdCommitBuf, &osd_drm_buf);
                    g_stDrmCfg.fb_id[0] = osd_drm_buf.fb_id;
                    g_stDrmCfg.planeNeedClean[0] = 0;
                    g_stDrmCfg.planeNeedUpdate[0] = ListOsdCommit->planeNeedUpdate;
                    if(g_stDrmCfg.planeNeedUpdate[0])
                    {
                        ListOsdCommit->planeNeedUpdate = false;
                    }
                }
                else
                {
                     //clear osd plane
                     osd_drm_buf.fb_id = 0;
                     g_stDrmCfg.fb_id[0] = osd_drm_buf.fb_id;
                     g_stDrmCfg.planeNeedClean[0] = 1;

                     delete ListOsdCommit;
                     ListOsdCommit = NULL;
                     printf("clear gop to black now \n");
                }
            }
        }
        else
        {
            osd_drm_buf.fb_id = 0;
            g_stDrmCfg.fb_id[0] = osd_drm_buf.fb_id;
        }

        if(_g_MainSorCanvas.pIsCreated)
        {
            pthread_mutex_lock(&g_cursor_canvas_mutex);
            ListCursorCommit = get_first_node(&g_HeadListCursorCommit);
            if(ListCursorCommit)
            {
                printf("%s_%d \n",__FUNCTION__,__LINE__);
                cusorCommitBuf = ListCursorCommit->pGraphicBuffer;

                if(cusorCommitBuf)
                {

                    sstar_add_commitbuf(cusorCommitBuf, &sor_drm_buf);
                    g_stDrmCfg.fb_id[2] = sor_drm_buf.fb_id;
                    g_stDrmCfg.planeNeedClean[2] = 0;
                    g_stDrmCfg.planeNeedUpdate[2] = ListCursorCommit->planeNeedUpdate;
                }
                else
                {
                    sor_drm_buf.fb_id = 0;
                    g_stDrmCfg.fb_id[2] = 0;
                    g_stDrmCfg.planeNeedClean[2] = 1;
                    g_stDrmCfg.planeNeedUpdate[2] = false;
                }
            }
            pthread_mutex_unlock(&g_cursor_canvas_mutex);
        }
        else
        {
            sor_drm_buf.fb_id = 0;
            g_stDrmCfg.fb_id[2] = sor_drm_buf.fb_id;
            g_stDrmCfg.planeNeedUpdate[2] = false;
        }

        if(_g_HdmiRxPlayer.pIsCreated || _g_MediaPlayer.pIsCreated)
        {
            ListVideoCommit = get_first_node(&g_HeadListVideoCommit);
            try_count = 5;
            while((_g_HdmiRxPlayer.player_working || _g_MediaPlayer.player_working) && !ListVideoCommit &&  try_count)
            {
                ListVideoCommit = get_first_node(&g_HeadListVideoCommit);
                try_count --;
                usleep(1 * 1000);
            }
            if(ListVideoCommit)
            {
                videoCommitBuf = ListVideoCommit->pGraphicBuffer;
                if(videoCommitBuf != NULL)
                {
                    sstar_add_commitbuf(videoCommitBuf, &video_drm_buf);
                    g_stDrmCfg.fb_id[1] = video_drm_buf.fb_id;
                    g_stDrmCfg.planeNeedClean[1] = 0;
                    g_stDrmCfg.planeNeedUpdate[1] = ListVideoCommit->planeNeedUpdate;
                    if(g_stDrmCfg.planeNeedUpdate[1])
                    {
                        ListVideoCommit->planeNeedUpdate = false;
                    }
                }
                else
                {
                    //clear video plane
                    video_drm_buf.fb_id = 0;
                    g_stDrmCfg.fb_id[1] = video_drm_buf.fb_id;
                    g_stDrmCfg.planeNeedClean[1] = 1;

                    delete ListVideoCommit;
                    ListVideoCommit = NULL;
                    printf("clear mop to black now \n");
                }
            }
        }
        else
        {
            video_drm_buf.fb_id = 0;
            g_stDrmCfg.fb_id[1] = video_drm_buf.fb_id;
        }

        if((!_g_MainCanvas.pIsCreated && !_g_HdmiRxPlayer.pIsCreated && !_g_MediaPlayer.pIsCreated && !_g_MainSorCanvas.pIsCreated) ||
            ((prev_mop_gem_handle.first == video_drm_buf.fb_id) && (prev_gop_gem_handle.first == osd_drm_buf.fb_id) && (prev_sor_gem_handle.first == sor_drm_buf.fb_id)))
        {
            pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
            usleep(1 * 1000);
            continue;
        }
        else
        {

            atomic_set_plane(g_stDrmCfg.fb_id, g_stDrmCfg.planeNeedClean, g_stDrmCfg.planeNeedUpdate);
            //wait frame disp
            if(g_stDrmCfg.out_fence != -1)
            {
                ret = sync_wait(g_stDrmCfg.out_fence, 50);
                if(ret != 0)
                {
                    //printf("waring:maybe drop one drm frame, ret=%d out_fence=%d\n", ret, g_stDrmCfg.out_fence);
                }
                close(g_stDrmCfg.out_fence);
                g_stDrmCfg.out_fence = -1;
            }

            if(prev_sor_gem_handle.first != sor_drm_buf.fb_id)
            {
                g_cursor_canvas = ListCursorCommit->pGraphicBuffer;
                if((sor_drm_buf.fb_id > 0))
                {
                    add_tail_node(&g_HeadListCursorOutput, &ListCursorCommit->list);
                }
                drm_free_gem_handle(&prev_sor_gem_handle);
                prev_sor_gem_handle.first = sor_drm_buf.fb_id;
                prev_sor_gem_handle.second = sor_drm_buf.gem_handle[0];
            }

            if((prev_mop_gem_handle.first != video_drm_buf.fb_id))
            {
                if((video_drm_buf.fb_id > 0))
                {
                    sstar_put_videobuf(ListVideoCommit);
                    add_tail_node(&g_HeadListVideoOutput, &ListVideoCommit->list);
                }

                drm_free_gem_handle(&prev_mop_gem_handle);
                prev_mop_gem_handle.first = video_drm_buf.fb_id;
                prev_mop_gem_handle.second = video_drm_buf.gem_handle[0];
            }
            if((prev_gop_gem_handle.first != osd_drm_buf.fb_id))
            {
                if((osd_drm_buf.fb_id > 0))
                {
                    add_tail_node(&g_HeadListOsdOutput, &ListOsdCommit->list);
                }
                drm_free_gem_handle(&prev_gop_gem_handle);
                prev_gop_gem_handle.first = osd_drm_buf.fb_id;
                prev_gop_gem_handle.second = osd_drm_buf.gem_handle[0];
            }
        }
        pthread_mutex_unlock(&g_stDrmCfg.commit_Mutex);
    }
    printf("sstar_DrmCommit_Thread exit\n");
    return NULL;
}

static std::shared_ptr<GpuGraphicBuffer> sstar_draw_subcanvas(GrFillInfo srcFillInfo)
{
    FILE* file = NULL;
    char *pSubFbVaddr = NULL;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;
    int subFbWidth  = srcFillInfo.rect.right;
    int subFbHeight = srcFillInfo.rect.bottom;

    std::shared_ptr<GpuGraphicBuffer> subFbCanvas = std::make_shared<GpuGraphicBuffer>(subFbWidth, subFbHeight, u32UiFormat, ALIGN_UP(subFbWidth * 4, 64));

    g_stdOsdGpuRender->fill(subFbCanvas, srcFillInfo);

    pSubFbVaddr = (char *)subFbCanvas->map(READWRITE);
    if(!pSubFbVaddr) {
        printf("Failed to mmap subFbCanvas buffer\n");
        return NULL;
    }

    file = fopen(g_InputUiPath, "r+");
    if (file)
    {
        for (int i = 0; i < subFbHeight; i++)
        {
            fread(pSubFbVaddr + (i * ALIGN_UP(subFbWidth * 4, 64)), sizeof(char), subFbWidth*4, file);
        }
        fclose(file);
    }
    else
    {
        printf("Error: can not open: %s\n", g_InputUiPath);
    }
    subFbCanvas->flushCache(READWRITE);
    subFbCanvas->unmap(pSubFbVaddr, READWRITE);

    return subFbCanvas;
}


static int sstar_draw_canvas(plane_info_t osdInfo)
{
    GrFillInfo srcFillInfo;
    GrFillInfo dstFillInfo;
    GrBitblitInfo bitblitinfo;
    std::shared_ptr<GpuGraphicBuffer> subFbCanvas;

    _g_MainCanvas.mainFbCanvas = sstar_canvas_init(g_stDrmCfg.width, g_stDrmCfg.height);
    if(_g_MainCanvas.mainFbCanvas == NULL)
    {
        printf("Error:mainFbCanvas pointer is NULL\n");
        return NULL;
    }
    _g_MainCanvas.mutex = PTHREAD_MUTEX_INITIALIZER;

    srcFillInfo.rect.left = 0;
    srcFillInfo.rect.top = 0;
    srcFillInfo.rect.right = _g_MediaPlayer.ui_info.in_width;
    srcFillInfo.rect.bottom = _g_MediaPlayer.ui_info.in_height;
    srcFillInfo.color = 0x00;
    subFbCanvas = sstar_draw_subcanvas(srcFillInfo);
    if (subFbCanvas == NULL) {
        printf("dram_SubFbCanvas failed\n");
        return -1;
    }

    pthread_mutex_lock(&g_MainFbCanvas_mutex);
    dstFillInfo.rect.left = 0;
    dstFillInfo.rect.top = 0;
    dstFillInfo.rect.right = osdInfo.out_width;
    dstFillInfo.rect.bottom = osdInfo.out_height;
    dstFillInfo.color = 0x00000000;
    g_stdOsdGpuRender->fill(_g_MainCanvas.mainFbCanvas, dstFillInfo);

    bitblitinfo.transfrom = NONE;
    memcpy(&bitblitinfo.srcRect, &srcFillInfo.rect, sizeof(Rect));
    bitblitinfo.dstRect.left = 0;
    bitblitinfo.dstRect.top = 0;
    bitblitinfo.dstRect.right = bitblitinfo.srcRect.right;
    bitblitinfo.dstRect.bottom = bitblitinfo.srcRect.bottom;

    printf("src[%d,%d,%d,%d] dst[%d,%d,%d,%d]\n",bitblitinfo.srcRect.left,bitblitinfo.srcRect.top,bitblitinfo.srcRect.right,bitblitinfo.srcRect.bottom,
        bitblitinfo.dstRect.left,bitblitinfo.dstRect.top,bitblitinfo.dstRect.right,bitblitinfo.dstRect.bottom);

    g_stdOsdGpuRender->bitblit(subFbCanvas, _g_MainCanvas.mainFbCanvas, bitblitinfo);

    pthread_mutex_unlock(&g_MainFbCanvas_mutex);
    return 0;

}

static int sstar_draw_sorcanvas(plane_info_t sorInfo)
{
    GrFillInfo dstFillInfo;
    GrBitblitInfo bitblitinfo;
    std::shared_ptr<GpuGraphicBuffer> subFbCanvas;
    _g_MainSorCanvas.mainFbCanvas = sstar_sorcanvas_init(sorInfo.out_width, sorInfo.out_height);
    if(_g_MainSorCanvas.mainFbCanvas == NULL)
    {
        printf("Error:mainFbCanvas pointer is NULL\n");
        return -1;
    }
    _g_MainSorCanvas.mutex = PTHREAD_MUTEX_INITIALIZER;
    _g_MainSorCanvas.pIsCreated = true;
    dstFillInfo.rect.left = 0;
    dstFillInfo.rect.top = 0;
    dstFillInfo.rect.right = sorInfo.out_width;
    dstFillInfo.rect.bottom = sorInfo.out_height;
    dstFillInfo.color = 0xff0000ff;
    g_stdSorGpuRender->fill(_g_MainSorCanvas.mainFbCanvas, dstFillInfo);
    g_stdSorGpuRender->deinit();
    g_stdSorGpuRender = NULL;
    return 0;

}


void *sstar_OsdProcess_Thread(void * arg)
{
    int ret;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;
    St_ListNode_t *ListOsdOutput;
    ret = sstar_OsdList_Init();
    if (ret < 0)
    {
        printf("Error:sstar_OsdList_Init fail \n");
        return NULL;
    }

    _g_MainCanvas.pIsCreated = true;
    while(g_bThreadExitUiDrm == TRUE)
    {
        ListOsdOutput = get_first_node(&g_HeadListOsdOutput);//get scl out buf

        if(ListOsdOutput)
        {
            pthread_mutex_lock(&_g_MainCanvas.mutex);

            if(ListOsdOutput->pGraphicBuffer && g_stdOsdGpuGfx)
            {
                g_stdOsdGpuGfx->releaseBuffer(ListOsdOutput->pGraphicBuffer);
                ListOsdOutput->pGraphicBuffer = NULL;
            }

            if(!g_bGpuProcessNeed)
            {
                if(g_stdOsdGpuGfx != NULL)
                {
                    g_stdOsdGpuGfx->deinit();
                    g_stdOsdGpuGfx = NULL;
                }
                ListOsdOutput->pGraphicBuffer = _g_MainCanvas.mainFbCanvas;
                //ListOsdOutput->planeNeedUpdate = true;
            }
            else
            {
                pthread_mutex_lock(&g_OsdProcessMutex);
                if(!g_stdOsdGpuGfx)
                {
                    g_RectDisplayRegion[0].left = 0;
                    g_RectDisplayRegion[0].top = 0;
                    g_RectDisplayRegion[0].right = g_CurOsdInfo.out_width;
                    g_RectDisplayRegion[0].bottom = g_CurOsdInfo.out_height;
                    g_stdOsdGpuGfx = sstar_gpugfx_context_init(g_CurOsdInfo.out_width, g_CurOsdInfo.out_height, u32UiFormat, false);
                    ListOsdOutput->planeNeedUpdate = true;
                }
                ret = g_stdOsdGpuGfx->process(_g_MainCanvas.mainFbCanvas, g_RectDisplayRegion[0], ListOsdOutput->pGraphicBuffer);
                if (ret)
                {
                    printf("Osd:Gpu graphic effect process failed\n");

                    pthread_mutex_unlock(&g_OsdProcessMutex);
                    pthread_mutex_unlock(&_g_MainCanvas.mutex);
                    add_tail_node(&g_HeadListOsdOutput, &ListOsdOutput->list);
                    continue;
                }

                pthread_mutex_unlock(&g_OsdProcessMutex);
            }
            pthread_mutex_unlock(&_g_MainCanvas.mutex);
            add_tail_node(&g_HeadListOsdCommit, &ListOsdOutput->list);
            usleep(33 * 1000);
        }
        else
        {
            //printf("g_HeadListOsdOutput.queue_list list is empty\n");
            usleep(5 * 1000);
        }

    }

    printf("sstar_OsdProcess_Thread exit \n");
    sstar_clear_plane(GOP_UI_ID);

    if(g_stdOsdGpuGfx != NULL)
    {
        g_stdOsdGpuGfx->deinit();
        g_stdOsdGpuGfx = NULL;
    }
    sstar_canvas_deinit();
    sstar_OsdList_DeInit();
    return NULL;
}


int sstar_set_cursorxy(unsigned int pos_x, unsigned int pos_y)
{
    Rect tmp_RectDisplayRegion;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;
    St_ListNode_t* ListCursorOutput;
    int ret;

    if(pos_x < 0)
    {
       g_CurSorInfo.pos_x = 0;
    }
    else if(pos_x >= g_stDrmCfg.width - cursor_x_size)
    {
        g_CurSorInfo.pos_x = (g_stDrmCfg.width - cursor_x_size);
    }
    else
    {
        g_CurSorInfo.pos_x = pos_x;
    }


    if(pos_y < 0)
    {
       g_CurSorInfo.pos_y = 0;
    }
    else if(pos_x >= g_stDrmCfg.height - cursor_y_size)
    {
        g_CurSorInfo.pos_y = (g_stDrmCfg.height - cursor_x_size);
    }
    else
    {
        g_CurSorInfo.pos_y = pos_y;
    }


    ListCursorOutput = get_first_node(&g_HeadListCursorOutput);//get scl out buf

    if(ListCursorOutput)
    {
        cursor_time = getOsTime();
        if(!g_bGpuProcessNeed)
        {
            ListCursorOutput->pGraphicBuffer = _g_MainSorCanvas.mainFbCanvas;
        }
        else
        {
            if(ListCursorOutput->pGraphicBuffer && g_stdSorGpuGfx)
            {
                g_stdSorGpuGfx->releaseBuffer(ListCursorOutput->pGraphicBuffer);
                ListCursorOutput->pGraphicBuffer = NULL;
            }
            pthread_mutex_lock(&g_SorProcessMutex);
            if(!g_stdSorGpuGfx)
            {
                g_stdSorGpuGfx = sstar_gpugfx_context_init(g_CurSorInfo.out_width, g_CurSorInfo.out_height, u32UiFormat, false);
            }

            tmp_RectDisplayRegion.left = 0;
            tmp_RectDisplayRegion.top = 0;
            tmp_RectDisplayRegion.right = g_CurSorInfo.out_width;
            tmp_RectDisplayRegion.bottom = g_CurSorInfo.out_height;
            ret = g_stdSorGpuGfx->process(_g_MainSorCanvas.mainFbCanvas, tmp_RectDisplayRegion, ListCursorOutput->pGraphicBuffer);
            if (ret)
            {
                printf("cursor:Gpu graphic effect process failed\n");
                pthread_mutex_unlock(&g_SorProcessMutex);
                return -1;
            }
            pthread_mutex_unlock(&g_SorProcessMutex);
        }
        ListCursorOutput->planeNeedUpdate = true;
        add_tail_node(&g_HeadListCursorCommit, &ListCursorOutput->list);
    }
    return 0;

}


#if 0
void *sstar_CursorProcess_Thread(void * arg)
{
    Rect tmpRectDisplayRegion;
    uint32_t u32UiFormat = DRM_FORMAT_ABGR8888;
    while(_g_MainSorCanvas.pIsCreated){
        pthread_mutex_lock(&g_cursor_canvas_mutex);
        if (cursor_time && (getOsTime() - cursor_time >= 5000))//hide cursor
        {
            if (!g_stDrmCfg.planeNeedClean[2] && g_stDrmCfg.planeNeedUpdate[2]){
                sstar_clear_plane(GOP_CURSOR_ID);
            }
        }
        pthread_mutex_unlock(&g_cursor_canvas_mutex);
        usleep(100 * 1000);
    }
    return NULL;
}
#endif

int sttar_Cursor_Creat()
{
    int ret = 0;
    if(!_g_MainSorCanvas.mainFbCanvas)
    {
        g_CurSorInfo.pos_x = 0;
        g_CurSorInfo.pos_y = 0;
        g_CurSorInfo.out_width = cursor_x_size;
        g_CurSorInfo.out_height = cursor_y_size;
        sstar_CursorList_Init();
        ret = sstar_draw_sorcanvas(g_CurSorInfo);
        if(0 != ret)
        {
            printf("sttar_Cursor_Creat fail \n");
            return ret;
        }

        printf("sttar_Cursor_Creat\n");
        //pthread_create(&g_pThreadCursorDrm, NULL, sstar_CursorProcess_Thread, NULL);
    }
    return ret;
}
void sttar_Cursor_Destory()
{
    _g_MainSorCanvas.pIsCreated = false;
    if(g_stdSorGpuGfx != NULL)
    {
        g_stdSorGpuGfx->deinit();
        g_stdSorGpuGfx = NULL;
    }
    sstar_sorcanvas_deinit();
    sstar_CursorList_DeInit();
}

void* sstar_HdmiPlugsDetect_Thread(void * arg)
{
    MI_S32  s32HDMIRxFd;
    fd_set set;
    int ret;
    struct timeval tv;
    static MI_HDMIRX_TimingInfo_t last_hdmirx_timing_info;

    s32HDMIRxFd = MI_HDMIRX_GetFd(_g_HdmiRxPlayer.u8HdmirxPort);
    while (hvp_event_thread_running)
    {
        FD_ZERO(&set);
        FD_SET(s32HDMIRxFd , &set);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(s32HDMIRxFd+1, &set, NULL, NULL, &tv);
        if(ret < 0 )
        {
            printf("select error \n");
            break;
        }
        else if (ret == 0)
        {
            //printf("MI_HDMIRX_GetFd select timeout \n");
            continue;
        }
        else
        {
            ret = MI_HDMIRX_GetHDMILineDetStatus(_g_HdmiRxPlayer.u8HdmirxPort, &_g_HdmiRxPlayer.pbConnected);
            if(ret != MI_SUCCESS)
            {
                printf("MI_HDMIRX_GetHDMILineDetStatus fail,ret=%d \n",ret);
            }

            //printf("MI_HDMIRX_GetHDMILineDetStatus pbConnected=%d \n",_g_HdmiRxPlayer.pbConnected);

            if(_g_HdmiRxPlayer.pbConnected)
            {
                printf("HdmiRx plugs in\n");
                ret = MI_HDMIRX_GetSignalStatus(_g_HdmiRxPlayer.u8HdmirxPort, &_g_HdmiRxPlayer.eSignalStatus);
                if(ret != MI_SUCCESS)
                {
                    printf("MI_HDMIRX_GetSignalStatus fail,ret=0x%x \n",ret);
                }
                if(_g_HdmiRxPlayer.eSignalStatus == E_MI_HDMIRX_SIG_SUPPORT)
                {
                    printf("%s_%d MI HdmiRx signal stable\n",__FUNCTION__,__LINE__);
                    ret = MI_HDMIRX_GetTimingInfo(_g_HdmiRxPlayer.u8HdmirxPort, &last_hdmirx_timing_info);
                    if (ret != MI_SUCCESS)
                    {
                        printf("Get Hdmirx Timing error!\n");
                        break;
                    }
                    _g_HdmiRxPlayer.player_working = true;
                    _g_HdmiRxPlayer.pIsCleaned = false;
                }
                while(_g_HdmiRxPlayer.eSignalStatus != E_MI_HDMIRX_SIG_SUPPORT && hvp_event_thread_running)
                {
                    ret = MI_HDMIRX_GetSignalStatus(_g_HdmiRxPlayer.u8HdmirxPort, &_g_HdmiRxPlayer.eSignalStatus);
                    if(ret != MI_SUCCESS)
                    {
                        printf("MI_HDMIRX_GetSignalStatus fail,ret=%d \n",ret);
                    }

                    if(_g_HdmiRxPlayer.eSignalStatus == E_MI_HDMIRX_SIG_SUPPORT)
                    {
                        printf("%s_%d MI HdmiRx signal stable\n",__FUNCTION__,__LINE__);
                        _g_HdmiRxPlayer.pbConnected = true;
                        ret = MI_HDMIRX_GetTimingInfo(_g_HdmiRxPlayer.u8HdmirxPort, &last_hdmirx_timing_info);
                        if (ret != MI_SUCCESS)
                        {
                            printf("Get Hdmirx Timing error!\n");
                            break;
                        }
                        _g_HdmiRxPlayer.player_working = true;
                        _g_HdmiRxPlayer.pIsCleaned = false;
                    }
                    else
                    {
                        //printf("MI HdmiRx signal unstable,eSignalStatus=%d \n", _g_HdmiRxPlayer.eSignalStatus);
                        ret = MI_HDMIRX_GetHDMILineDetStatus(_g_HdmiRxPlayer.u8HdmirxPort, &_g_HdmiRxPlayer.pbConnected);//signal not stable,check again
                        if(ret != MI_SUCCESS)
                        {
                            printf("MI_HDMIRX_GetHDMILineDetStatus fail,ret=%d \n",ret);
                        }

                        if(!_g_HdmiRxPlayer.pbConnected && !_g_HdmiRxPlayer.pIsCleaned)
                        {
                            printf("HdmiRx plugs out.\n");
                            sstar_clear_plane(MOPG_ID0);
                            _g_HdmiRxPlayer.player_working = false;
                            _g_HdmiRxPlayer.pIsCleaned = true;
                            memset(&_g_HdmiRxPlayer.pstTimingInfo, 0x00, sizeof(MI_HDMIRX_TimingInfo_t));
                            memset(&_g_HdmiRxPlayer.pHvpstTimingInfo, 0x00, sizeof(MI_HVP_SignalTiming_t));
                            continue;
                        }
                    }
                }
                if (_g_HdmiRxPlayer.pstTimingInfo.eOverSample != last_hdmirx_timing_info.eOverSample
                    || _g_HdmiRxPlayer.pstTimingInfo.ePixelFmt != last_hdmirx_timing_info.ePixelFmt
                    || _g_HdmiRxPlayer.pstTimingInfo.eBitWidth != last_hdmirx_timing_info.eBitWidth)
                {
                    memcpy(&_g_HdmiRxPlayer.pstTimingInfo,&last_hdmirx_timing_info,  sizeof(MI_HDMIRX_TimingInfo_t));
                    printf("Get Signal OverSample: %d HdmirxFmt: %d BitWidth: %d OverSample: %d HdmirxFmt: %d BitWidth: %d\n",
                            last_hdmirx_timing_info.eOverSample, last_hdmirx_timing_info.ePixelFmt,
                            last_hdmirx_timing_info.eBitWidth,
                            _g_HdmiRxPlayer.pstTimingInfo.eOverSample, _g_HdmiRxPlayer.pstTimingInfo.ePixelFmt,
                            _g_HdmiRxPlayer.pstTimingInfo.eBitWidth);

                    signal_monitor_sync_hdmi_paramter_2_hvp(&_g_HdmiRxPlayer.pstTimingInfo);
                }
            }
            else
            {
                printf("HdmiRx plugs out.\n");
                sstar_clear_plane(MOPG_ID0);
                _g_HdmiRxPlayer.player_working = false;
                memset(&_g_HdmiRxPlayer.pstTimingInfo, 0x00, sizeof(MI_HDMIRX_TimingInfo_t));
                memset(&_g_HdmiRxPlayer.pHvpstTimingInfo, 0x00, sizeof(MI_HVP_SignalTiming_t));
                _g_HdmiRxPlayer.eSignalStatus = E_MI_HDMIRX_SIG_NO_SIGNAL;
                _g_HdmiRxPlayer.eHvpSignalStatus = E_MI_HVP_SIGNAL_UNSTABLE;
                _g_HdmiRxPlayer.pIsCleaned = true;
            }
        }
    }

    MI_HDMIRX_CloseFd(_g_HdmiRxPlayer.u8HdmirxPort);
    printf("hdmi_hotplug_detection Exit\n");
    return NULL;

}

void* sstar_HvpEventMoniter_Thread(void * arg)
{
    printf("hvp_event_thread enter\n");
    MI_HVP_DEV dev = 0;
    MI_S32 ret = 0;
    MI_S32 s32Fd = -1;
    MI_BOOL bTrigger;
    fd_set read_fds;
    struct timeval tv;

    static MI_HVP_SignalTiming_t last_signal_timing = {0, 0, 0, 0};

    ret = MI_HVP_GetResetEventFd(dev, &s32Fd);
    if (ret != MI_SUCCESS)
    {
        printf("Get Reset fd Errr, Hvp Dev%d!\n", dev);
        return NULL;
    }

    while (hvp_event_thread_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
        if (ret < 0)
        {
            printf("Reset fd select error!!\n");
            break;
        }
        else if(ret == 0)
        {
            //printf("MI_HVP_GetResetEventFd timeout \n");
            continue;
        }

        ret = MI_HVP_GetSourceSignalStatus(dev, &_g_HdmiRxPlayer.eHvpSignalStatus);
        if (ret != MI_SUCCESS)
        {
            printf("MI_HVP_GetSourceSignalStatus, hvp=%d!\n", dev);

        }
        while(_g_HdmiRxPlayer.pbConnected && _g_HdmiRxPlayer.eSignalStatus == E_MI_HDMIRX_SIG_SUPPORT
                && _g_HdmiRxPlayer.eHvpSignalStatus != E_MI_HVP_SIGNAL_STABLE && hvp_event_thread_running)
        {
            ret = MI_HVP_GetSourceSignalStatus(dev, &_g_HdmiRxPlayer.eHvpSignalStatus);
            if (ret != MI_SUCCESS)
            {
                printf("Get Signal Status Errr, Hvp Dev%d!\n", dev);
            }
            usleep(5 * 1000);
        }
        if(_g_HdmiRxPlayer.eHvpSignalStatus == E_MI_HVP_SIGNAL_STABLE)
        {
            _g_HdmiRxPlayer.player_working = true;
        }
        ret = MI_HVP_GetSourceSignalTiming(dev, &last_signal_timing);
        if (ret != MI_SUCCESS)
        {
            printf("Get Signal Timing Errr, Hvp Dev%d!\n", dev);
        }


        if (_g_HdmiRxPlayer.pHvpstTimingInfo.u16Width != last_signal_timing.u16Width
            || _g_HdmiRxPlayer.pHvpstTimingInfo.u16Height!= last_signal_timing.u16Height
            || _g_HdmiRxPlayer.pHvpstTimingInfo.bInterlace!= last_signal_timing.bInterlace)
        {
            printf("Get Signal St W: %d H: %d, Fps: %d bInterlace: %d\n",
                    _g_HdmiRxPlayer.pHvpstTimingInfo.u16Width, _g_HdmiRxPlayer.pHvpstTimingInfo.u16Height,
                    _g_HdmiRxPlayer.pHvpstTimingInfo.u16Fpsx100, (int)_g_HdmiRxPlayer.pHvpstTimingInfo.bInterlace);
            memcpy(&_g_HdmiRxPlayer.pHvpstTimingInfo, &last_signal_timing, sizeof(MI_HVP_SignalTiming_t));
        }

        ret = MI_HVP_GetResetEvent(dev, &bTrigger);
        if (ret != MI_SUCCESS)
        {
            printf("Get Reset cnt Errr, Hvp Dev%d!\n", dev);
        }

        if (bTrigger)
        {
            printf("ClearResetEvent !!!\n");
            MI_HVP_ClearResetEvent(dev);
        }
    }
    MI_HVP_CloseResetEventFd(dev, s32Fd);
    printf("hvp_event_thread exit\n");

    return NULL;
}


void *sstar_SclEnqueue_thread(void * param)
{
    MI_S32 s32Ret = MI_SUCCESS;
    std::shared_ptr<GpuGraphicBuffer> SclOutputBuffer;
    St_ListNode_t *ListSclToGfx;

    //*******scl port0*********
    MI_SYS_ChnPort_t stSclChnPort0;
    MI_SYS_DmaBufInfo_t stDmaBufInfo;
    MI_SYS_ChnPort_t *pstSclChnPort;

    pstSclChnPort = (MI_SYS_ChnPort_t *)param;

    memset(&stSclChnPort0, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSclChnPort0.eModId = pstSclChnPort->eModId;
    stSclChnPort0.u32DevId = pstSclChnPort->u32DevId;
    stSclChnPort0.u32ChnId = pstSclChnPort->u32ChnId;
    stSclChnPort0.u32PortId = pstSclChnPort->u32PortId;

    ///*******g_bThreadExitScl*********//
    while (g_bThreadExitScl == TRUE)
    {

        ListSclToGfx = get_first_node(&g_HeadListSclOutput);//get scl out buf
        if(!ListSclToGfx)
        {
            usleep(3*1000);
            continue;
        }
        SclOutputBuffer = ListSclToGfx->pGraphicBuffer;
        memset(&stDmaBufInfo, 0x00, sizeof(MI_SYS_DmaBufInfo_t));
        stDmaBufInfo.s32Fd[0] = SclOutputBuffer->getFd();
        stDmaBufInfo.s32Fd[1] = SclOutputBuffer->getFd();
        stDmaBufInfo.u16Width = (MI_U16)SclOutputBuffer->getWidth();
        stDmaBufInfo.u16Height = (MI_U16)SclOutputBuffer->getHeight();
        stDmaBufInfo.u32Stride[0] = SclOutputBuffer->getWidth();
        stDmaBufInfo.u32Stride[1] = SclOutputBuffer->getWidth();
        stDmaBufInfo.u32DataOffset[0] = 0;
        stDmaBufInfo.u32DataOffset[1] = stDmaBufInfo.u16Width *stDmaBufInfo.u16Height;
        if(SclOutputBuffer->getFormat() == DRM_FORMAT_NV12)
        {
            stDmaBufInfo.eFormat = (MI_SYS_PixelFormat_e)E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        }

        s32Ret = MI_SYS_ChnOutputPortEnqueueDmabuf(&stSclChnPort0, &stDmaBufInfo);
        if (s32Ret != 0)
        {
            printf("MI_SYS_ChnOutputPortEnqueueDmabuf fail\n");
            add_tail_node(&g_HeadListSclOutput, &ListSclToGfx->list);
            return NULL;
        }
        add_tail_node(&g_HeadListGpuGfxInput, &ListSclToGfx->list);
    }
    printf("sstar_SclEnqueue_thread g_bThreadExitScl == end!!!\n ");

    return NULL;
}





void *sstar_HdmiRxProcess_Thread(void * param)
{
    MI_S32 s32Ret = MI_SUCCESS;
    std::shared_ptr<GpuGraphicBuffer> GfxInputBuffer;
    St_ListNode_t *ListVideoOutput;
    MI_SYS_ChnPort_t *pstSclChnPort = (MI_SYS_ChnPort_t *)param;
    MI_SYS_ChnPort_t stSclChnPort0;
    St_ListNode_t *GfxReadListNode;
    MI_SYS_DmaBufInfo_t stDmaOutputBufInfo;
    memset(&stSclChnPort0, 0x0, sizeof(MI_SYS_ChnPort_t));

    stSclChnPort0.eModId = pstSclChnPort->eModId;
    stSclChnPort0.u32DevId = pstSclChnPort->u32DevId;
    stSclChnPort0.u32ChnId = pstSclChnPort->u32ChnId;
    stSclChnPort0.u32PortId = pstSclChnPort->u32PortId;

    /************************************************
    step :init GPU
    *************************************************/
    uint32_t u32Width  = g_stDrmCfg.width;
    uint32_t u32Height = g_stDrmCfg.height;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;
    g_RectDisplayRegion[1].left = 0;
    g_RectDisplayRegion[1].top = 0;
    g_RectDisplayRegion[1].right = g_stDrmCfg.width;
    g_RectDisplayRegion[1].bottom = g_stDrmCfg.height;

    struct timeval tv;
    MI_S32 s32FdSclPort0 = -1;
    fd_set ReadFdsSclPort0;

    if (MI_SUCCESS != (s32Ret = MI_SYS_GetFd(&stSclChnPort0, &s32FdSclPort0)))
    {
        printf("GET scl Fd Failed , scl Chn = %d\n", stSclChnPort0.u32ChnId);
        return NULL;
    }
    sstar_clear_plane(MOPG_ID0);
    while(g_bThreadExitHdmi == TRUE)
    {
        FD_ZERO(&ReadFdsSclPort0);
        FD_SET(s32FdSclPort0, &ReadFdsSclPort0);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        s32Ret = select(s32FdSclPort0 + 1, &ReadFdsSclPort0, NULL, NULL, &tv);
        if (s32Ret < 0)
        {
            printf("slc select failed\n");
            sleep(1);
            break;
        }
        else if (0 == s32Ret)
        {
            //printf("scl select timeout\n");
            continue;
        }
        else
        {

            GfxReadListNode = get_first_node(&g_HeadListGpuGfxInput);//get scl out buf
            if(GfxReadListNode == NULL)
            {
                //printf("Warn: Get output buf from list failed\n");
                continue;
            }
            GfxInputBuffer = GfxReadListNode->pGraphicBuffer;
            s32Ret = MI_SYS_ChnOutputPortDequeueDmabuf(&stSclChnPort0, &stDmaOutputBufInfo);
            if(stDmaOutputBufInfo.u32Status != MI_SYS_DMABUF_STATUS_DONE)
            {
                add_tail_node(&g_HeadListSclOutput, &GfxReadListNode->list);
                //printf("MI_SYS_ChnOutputPortDequeueDmabuf Invail=%d \n",stDmaOutputBufInfo.u32Status);
                continue;
            }

            ListVideoOutput = get_first_node(&g_HeadListVideoOutput);//get commit buf
            if(ListVideoOutput)
            {

                if(ListVideoOutput->pGraphicBuffer && g_stdHdmiRxGpuGfx)
                {
                    g_stdHdmiRxGpuGfx->releaseBuffer(ListVideoOutput->pGraphicBuffer);
                    ListVideoOutput->pGraphicBuffer = NULL;
                }
                if(!g_bGpuProcessNeed)
                {
                    if(g_stdHdmiRxGpuGfx)
                    {
                        g_stdHdmiRxGpuGfx->deinit();
                        g_stdHdmiRxGpuGfx = NULL;
                    }

#ifdef USE_GPUGFX_ROCESS   //use gpu bitblit to scl up/down

                    _g_MediaPlayer.video_info.pos_x = 0;
                    _g_MediaPlayer.video_info.pos_y = 0;
                    _g_MediaPlayer.video_info.out_width = GfxInputBuffer->getWidth() > g_stDrmCfg.width ? g_stDrmCfg.width:GfxInputBuffer->getWidth();
                    _g_MediaPlayer.video_info.out_height = GfxInputBuffer->getHeight() > g_stDrmCfg.height ? g_stDrmCfg.height:GfxInputBuffer->getHeight();
                    ListVideoOutput->pGraphicBuffer = sstar_frame_process(GfxInputBuffer);

#else   //use drm to scl up

                    _g_MediaPlayer.video_info.pos_x = 0;
                    _g_MediaPlayer.video_info.pos_y = 0;
                    _g_MediaPlayer.video_info.out_width = GfxInputBuffer->getWidth();
                    _g_MediaPlayer.video_info.out_height = GfxInputBuffer->getHeight();
                    ListVideoOutput->pGraphicBuffer = GfxInputBuffer;
                    if((GfxInputBuffer->getWidth() > g_stDrmCfg.width) || (GfxInputBuffer->getHeight() > g_stDrmCfg.height))
                    {
                        printf("Error: src buf is large than panel size ,please check:getWidth=%d  getHeight=%d\n",GfxInputBuffer->getWidth(),GfxInputBuffer->getHeight());
                        add_tail_node(&g_HeadListSclOutput, &GfxReadListNode->list);
                        add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);
                        continue;
                    }

                    if(memcmp(&g_CurVideoInfo, &_g_MediaPlayer.video_info, sizeof(plane_info_t)))
                    {
                        memcpy(&g_CurVideoInfo, &_g_MediaPlayer.video_info, sizeof(plane_info_t));
                        ListVideoOutput->planeNeedUpdate = true;
                    }

#endif
                }
                else
                {
                    pthread_mutex_lock(&g_HdmiRXProcessMutex);
                    if(!g_stdHdmiRxGpuGfx)
                    {
                        if(g_stdGfxGpuRender)
                        {
                            g_stdGfxGpuRender->deinit();
                            g_stdGfxGpuRender = NULL;
                        }
                        g_stdHdmiRxGpuGfx = sstar_gpugfx_context_init(u32Width, u32Height, u32GpuInputFormat, false);
                        ListVideoOutput->planeNeedUpdate = true;
                        memset(&g_CurVideoInfo, 0x00, sizeof(plane_info_t));
                    }
                    s32Ret = g_stdHdmiRxGpuGfx->process(GfxInputBuffer, g_RectDisplayRegion[1], ListVideoOutput->pGraphicBuffer);
                    if (s32Ret) {
                        printf("g_stdHdmiRxGpuGfx Gpu graphic effect process failed\n");
                        add_tail_node(&g_HeadListSclOutput, &GfxReadListNode->list);
                        add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);
                        pthread_mutex_unlock(&g_HdmiRXProcessMutex);
                        continue;
                    }
                    pthread_mutex_unlock(&g_HdmiRXProcessMutex);
                }
                add_tail_node(&g_HeadListSclOutput, &GfxReadListNode->list);
                add_tail_node(&g_HeadListVideoCommit, &ListVideoOutput->list);
            }
            else
            {
                if(!_g_HdmiRxPlayer.pbConnected)
                {
                    //sstar_clear_plane(MOPG_ID0);
                }
                add_tail_node(&g_HeadListSclOutput, &GfxReadListNode->list);
                usleep(5 * 1000);
            }

        }
    }

    printf("sstar_HdmiRxProcess_Thread exit\n");
    if(g_stdHdmiRxGpuGfx)
    {
        g_stdHdmiRxGpuGfx->deinit();
        g_stdHdmiRxGpuGfx = NULL;
    }
    if(g_stdGfxGpuRender)
    {
        g_stdGfxGpuRender->deinit();
        g_stdGfxGpuRender = NULL;
    }
    MI_SYS_CloseFd(s32FdSclPort0);
    FD_ZERO(&ReadFdsSclPort0);

    return NULL;
}

void *sstar_VideoProcess_Thread(void * arg)
{
    St_ListNode_t *ListVideoOutput;
    std::shared_ptr<GpuGraphicBuffer> outputBuffer;
    frame_info_t frame_info;
    uint32_t frame_offset[3];
    int ret = -1;
    int try_count;
    std::shared_ptr<GpuGraphicBuffer> GfxInputBuffer;
    std::shared_ptr<GpuGraphicRender> gpuGfxRender;
    /************************************************
    step :init GPU
    *************************************************/
    uint32_t u32Width  = g_stDrmCfg.width;
    uint32_t u32Height = g_stDrmCfg.height;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;

    sstar_clear_plane(MOPG_ID0);

    while(g_bThreadExitGfx)
    {

        if(_g_MediaPlayer.player_working)
        {
            ListVideoOutput = get_first_node(&g_HeadListVideoOutput);
            if(ListVideoOutput)
            {
                if(ListVideoOutput->pGraphicBuffer && g_stdVideoGpuGfx)
                {
                    g_stdVideoGpuGfx->releaseBuffer(ListVideoOutput->pGraphicBuffer);
                    ListVideoOutput->pGraphicBuffer = NULL;
                }
                try_count = 5;
                ret = mm_player_get_video_frame(&frame_info);
                while(ret != 0 && try_count && _g_MediaPlayer.player_working)
                {
                    ret = mm_player_get_video_frame(&frame_info);
                    if(ret != 0)
                    {
                        try_count --;
                        usleep(1 * 1000);
                    }
                }
                if(ret != 0)
                {
                    printf("warn: Failed to mm_player_get_video_frame\n");
                    add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);//put list back
                    continue;
                }

                if (frame_info.format == AV_PIXEL_FMT_NV12)
                {
                    frame_offset[0] = 0;  //y data
                    frame_offset[1] = frame_info.data[1] - frame_info.data[0];   //uv offset

                    GfxInputBuffer = std::make_shared<GpuGraphicBuffer>(frame_info.dma_buf_fd[0], frame_info.width, frame_info.height, u32GpuInputFormat, frame_info.stride, frame_offset);
                    if(!GfxInputBuffer)
                    {
                        printf("Failed to turn dmabuf to GpuGraphicBuffer\n");
                        add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);//put list back
                        continue;
                    }

                    if(!g_bGpuProcessNeed) //do not need keystone
                    {

                        //printf("[x,y,w,h]=[%d,%d,%d,%d] frame_width,height=%d,%d \n",_g_MediaPlayer.video_info.pos_x,_g_MediaPlayer.video_info.pos_y,
                        //    _g_MediaPlayer.video_info.out_width,_g_MediaPlayer.video_info.out_height,frame_info.width,frame_info.height);
                        if(g_stdVideoGpuGfx)
                        {
                            g_stdVideoGpuGfx->deinit();
                            g_stdVideoGpuGfx = NULL;
                        }

#ifdef USE_GPUGFX_ROCESS   //use gpu bitblit to scl up
                        ListVideoOutput->pGraphicBuffer = sstar_frame_process(GfxInputBuffer);
                        memcpy(&ListVideoOutput->frame_info, &frame_info, sizeof(frame_info_t));
#else   //use drm to scl up

                        if((GfxInputBuffer->getWidth() > g_stDrmCfg.width) || (GfxInputBuffer->getHeight() > g_stDrmCfg.height))
                        {
                            printf("Error: src buf is large than panel size ,please check,or enable USE_GPUGFX_ROCESS \n");
                            add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);//put list back
                            continue;
                        }
                        ListVideoOutput->pGraphicBuffer = GfxInputBuffer;
                        memcpy(&ListVideoOutput->frame_info, &frame_info, sizeof(frame_info_t));
                        if(memcmp(&g_CurVideoInfo, &_g_MediaPlayer.video_info, sizeof(plane_info_t)))
                        {
                            memcpy(&g_CurVideoInfo, &_g_MediaPlayer.video_info, sizeof(plane_info_t));
                            ListVideoOutput->planeNeedUpdate = true;
                        }
#endif
                    }
                    else
                    {
                        pthread_mutex_lock(&g_VideoProcessMutex);
                        if(!g_stdVideoGpuGfx)
                        {
                            g_stdVideoGpuGfx = sstar_gpugfx_context_init(u32Width, u32Height, u32GpuInputFormat, false);
                            if(g_stdGfxGpuRender)
                            {
                                g_stdGfxGpuRender->deinit();
                                g_stdGfxGpuRender = NULL;
                            }
                            ListVideoOutput->planeNeedUpdate = true;
                            memset(&g_CurVideoInfo, 0x00, sizeof(plane_info_t));
                        }

                        //printf("[x,y,w,h]=[%d,%d,%d,%d] frame_width,height=%d,%d \n",_g_MediaPlayer.video_info.pos_x,_g_MediaPlayer.video_info.pos_y,
                        //    _g_MediaPlayer.video_info.out_width,_g_MediaPlayer.video_info.out_height,frame_info.width,frame_info.height);

                        g_RectDisplayRegion[1].left = _g_MediaPlayer.video_info.pos_x;
                        g_RectDisplayRegion[1].top = _g_MediaPlayer.video_info.pos_y;
                        g_RectDisplayRegion[1].right = _g_MediaPlayer.video_info.pos_x + _g_MediaPlayer.video_info.out_width;
                        g_RectDisplayRegion[1].bottom = _g_MediaPlayer.video_info.pos_y + _g_MediaPlayer.video_info.out_height;

                        ret = g_stdVideoGpuGfx->process(GfxInputBuffer, g_RectDisplayRegion[1], ListVideoOutput->pGraphicBuffer);
                        if (ret) {
                            printf("Video:Gpu graphic effect process failed,dma_buf_fd=%d getBufferSize=%d\n", frame_info.dma_buf_fd[0], GfxInputBuffer->getBufferSize());
                            add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);//put list back
                            pthread_mutex_unlock(&g_VideoProcessMutex);
                            continue;
                        }

                        pthread_mutex_unlock(&g_VideoProcessMutex);
                        memcpy(&ListVideoOutput->frame_info, &frame_info, sizeof(frame_info_t));
                    }
                    add_tail_node(&g_HeadListVideoCommit, &ListVideoOutput->list);
                }
            }
        }
        else
        {
            //printf("player_working=%d \n",_g_MediaPlayer.player_working);
            usleep(3*1000);
        }
    }

    printf("sstar_VideoProcess_Thread exit\n");
    if(g_stdVideoGpuGfx)
    {
        g_stdVideoGpuGfx->deinit();
        g_stdVideoGpuGfx = NULL;
    }
    if(g_stdGfxGpuRender)
    {
        g_stdGfxGpuRender->deinit();
        g_stdGfxGpuRender = NULL;
    }

    return NULL;
}

static void *sstar_PlayerMoniter_Thread(void *args)
{
    int ret;
    while (g_bThreadExitPlayer == TRUE) {
        ret = mm_player_get_status();
        if (ret < 0) {
            printf("mmplayer has been closed!\n");
            _g_MediaPlayer.player_working = false;
            continue;
        }
        if (ret & AV_PLAY_ERROR) {
            //mm_player_close();
            //_g_MediaPlayer.player_working = false;
            //g_bThreadExitPlayer = false;
            printf("mmplayer AV_PLAY_ERROR!\n");
        }
        else if ((ret & AV_PLAY_COMPLETE))
        {
            _g_MediaPlayer.player_working = false;
            ret = mm_player_close();
            if (ret < 0)
            {
                printf("[%s %d]mm_player_close fail!\n", __FUNCTION__, __LINE__);
            }
            printf("restart by user\n");
            sstar_media_context_init();
            ret = mm_player_open(&_g_MediaPlayer.param);
            if (ret < 0)
            {
                g_bThreadExitPlayer = false;
                printf("mm_player_reopen fail\n");
            }
            else
            {
                _g_MediaPlayer.player_working = true;
            }
        }
        usleep(10 * 1000);
    }
    printf("[%s %d]exit!\n", __FUNCTION__, __LINE__);

    return NULL;
}


static void *sstar_SensorFrame_Receive(void *param)
{
    int ret;
    MI_SYS_FrameData_t stFrameData;
    St_ListNode_t *ListVideoOutput;
    std::shared_ptr<GpuGraphicBuffer> GfxInputBuffer;
    uint32_t u32GpuInputFormat = DRM_FORMAT_NV12;
    void *pVaddr_In;
    int i = 0;
    MI_SYS_BUF_HANDLE stBufHandle;
    Sensor_Attr_t sensorAttr;
    sstar_get_sensor_attr(0, &sensorAttr);
#if 1  //for disp
    _g_MediaPlayer.pIsCreated = true;
    _g_MediaPlayer.player_working = true;
    _g_MediaPlayer.video_info.pos_x = 0;
    _g_MediaPlayer.video_info.pos_y = 0;
    _g_MediaPlayer.video_info.out_width = sensorAttr.u16Width;
    _g_MediaPlayer.video_info.out_height = sensorAttr.u16Height;
#else  //for dump

    int count = 3;
    FILE *out_fd = NULL;
    out_fd = open_dump_file("/customer/dump_snr.yuv");
#endif

    sstar_clear_plane(MOPG_ID0);

    while(g_bThreadExitReceive)
    {
        ListVideoOutput = get_first_node(&g_HeadListVideoOutput);//get commit buf
        if(ListVideoOutput == NULL)
        {
            //printf("Warn: Get output buf from list failed\n");
            usleep(1 * 1000);
            continue;
        }

        ret = sstar_get_snr_frame(&stFrameData, &stBufHandle);
        if(ret < 0)
        {
            usleep(1 * 1000);
            printf("sstar_get_snr_frame fail\n");
            continue;
        }


#if 1
        GfxInputBuffer = std::make_shared<GpuGraphicBuffer>(stFrameData.u16Width, stFrameData.u16Height, u32GpuInputFormat, ALIGN_UP(stFrameData.u16Width, 64));
        if (!GfxInputBuffer->initCheck())
        {
            printf("Error Create GpuGraphicBuffer failed\n");
            add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);//put list back
            continue;
        }
        pVaddr_In = GfxInputBuffer->map(READWRITE);

        for(i = 0; i < stFrameData.u16Height ;i++)
        {
            memcpy((char *)pVaddr_In + (i * GfxInputBuffer->getStride()[0]), (char *)stFrameData.pVirAddr[0] + (i * stFrameData.u32Stride[0]), stFrameData.u32Stride[0]);
        }
        for(i = 0; i < stFrameData.u16Height/2 ;i++)
        {
            memcpy((char *)pVaddr_In + ((i + stFrameData.u16Height) * GfxInputBuffer->getStride()[0]),
                (char *) stFrameData.pVirAddr[1] + (i * stFrameData.u32Stride[1]), stFrameData.u32Stride[1]);
        }

        GfxInputBuffer->flushCache(READWRITE);
        GfxInputBuffer->unmap(pVaddr_In, READWRITE);


#ifdef USE_GPUGFX_ROCESS   //use gpu bitblit to scl up

        ListVideoOutput->pGraphicBuffer = sstar_frame_process(GfxInputBuffer);
#else
        ListVideoOutput->pGraphicBuffer = GfxInputBuffer;
#endif
        add_tail_node(&g_HeadListVideoCommit, &ListVideoOutput->list);
#else //for dump
        if(count > 0)
        {
            if(out_fd)
            {
                wirte_dump_file(out_fd, (char *)stFrameData.pVirAddr[0], stFrameData.u16Height * stFrameData.u32Stride[0]);
                wirte_dump_file(out_fd, (char *)stFrameData.pVirAddr[1], stFrameData.u16Height * stFrameData.u32Stride[1]/2);
            }
            count --;
        }
        if(count == 0)
        {
            close_dump_file(out_fd);
        }
#endif
        sstar_put_snr_frame(stBufHandle);

    }
    printf("sstar_SensorFrame_Receive exit\n");
    return NULL;
}

void *sstar_Media_AudioProcess_Thread(void * arg)
{

    int ret;
    frame_info_t frame_info;
    while(g_bThreadExitGfx)
    {
        ret = mm_player_get_audio_frame(&frame_info);
        if(ret < 0)
        {
            printf("mm_player_get_audio_frame fail\n");
            usleep(3 * 1000);
        }
        else
        {
            //printf("mm_player_get_audio_frame success,size=%lld sample_rate=%d channel_layout=%d channels=%d format=%d\n",
            //    frame_info.size, frame_info.sample_rate, frame_info.channel_layout, frame_info.channels, frame_info.format );

            mm_player_write_audio_frame(frame_info.extended_data, (int)frame_info.size);
            mm_player_put_audio_frame(&frame_info);
        }

    }

    printf("sstar_Media_AudioProcess_Thread exit\n");
    return NULL;
}

void *sstar_HDMI_AudioProcess_Thread(void * arg)
{
    int ret;
    hdmi_player_audio_frame_t *frame = NULL;
    prctl(PR_SET_NAME, "sstar_HDMI_AudioProcess_Thread");
    printf("%s thread_id: %llx\n", __FUNCTION__, (uint64_t)pthread_self());
    while(g_bThreadExitHdmiAudio == true)
    {
        if(!_g_HdmiRxPlayer.player_working)
        {
            usleep(100 * 1000);
            continue;
        }
        ret = hdmi_player_get_pcm(&frame, 100);
        if (0 != ret || !frame) {
            printf("%s hdmi_player_get_pcm failed\n", __FUNCTION__);
            continue;
        }

        //printf("%s data:%p size:%d channels:%d sample_rate:%d pts:%lld\n", __FUNCTION__, frame->data, frame->size, frame->channels, frame->sample_rate, frame->pts);

        ret = hdmi_player_write_pcm(frame);
        if (0 != ret) {
            printf("%s hdmi_player_write_pcm failed\n", __FUNCTION__);
            break;
        }

        ret = hdmi_player_put_pcm(frame);
        if (0 != ret) {
            printf("%s hdmi_player_put_pcm failed\n", __FUNCTION__);
            break;
        }

    }

    printf("sstar_HDMI_AudioProcess_Thread exit\n");
    return NULL;
}

static MI_S32 sstar_hdmirx_init()
{
    MI_S32 ret = MI_SUCCESS;

    /************************************************
    step :init HDMI_RX
    *************************************************/
    MI_HDMIRX_PortId_e u8HdmirxPort = _g_HdmiRxPlayer.u8HdmirxPort;
    MI_HDMIRX_Hdcp_t stHdmirxHdcpKey;
    MI_HDMIRX_Edid_t stHdmirxEdid;

    ret = MI_HDMIRX_Init();
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_Init error: %d\n", ret);
    }

    stHdmirxHdcpKey.u64HdcpDataVirAddr = (MI_U64)HDCP_KEY;
    stHdmirxHdcpKey.u32HdcpLength = sizeof(HDCP_KEY);
    ret = MI_HDMIRX_LoadHdcp(u8HdmirxPort, &stHdmirxHdcpKey);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_LoadHdcp error: %d\n", ret);
    }

    stHdmirxEdid.u64EdidDataVirAddr = (MI_U64)EDID_Test;
    stHdmirxEdid.u32EdidLength = 256;
    ret = MI_HDMIRX_UpdateEdid(u8HdmirxPort, &stHdmirxEdid);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_UpdateEdid error: %d\n", ret);
    }

    ret = MI_HDMIRX_Connect(u8HdmirxPort);
    if (ret != MI_SUCCESS) {
        printf("MI_HDMIRX_Connect error: %d\n", ret);
        return ret;
    }

    ret = MI_HDMIRX_GetHDMILineDetStatus(u8HdmirxPort, &_g_HdmiRxPlayer.pbConnected);
    if(ret == MI_SUCCESS && _g_HdmiRxPlayer.pbConnected)
    {
        printf("MI HdmiRx Connected\n");
    }
    else
    {
        printf("MI HdmiRx not Connect\n");
        //sstar_clear_plane(MOPG_ID0);
    }


    printf("MI HdmiRx Init done\n");
    return MI_SUCCESS;
}
static MI_S32 sstar_hdmirx_deinit()
{
    MI_S32 ret = MI_SUCCESS;
    MI_HDMIRX_PortId_e u8HdmirxPort = E_MI_HDMIRX_PORT0;
    /************************************************
    step :deinit HDMI_RX
    *************************************************/
    ret = MI_HDMIRX_DisConnect(u8HdmirxPort);
    if (ret != MI_SUCCESS) {
        printf("Deinit hdmirx fail");
        return ret;
    }

    ret = MI_HDMIRX_DeInit();
    if (ret != MI_SUCCESS) {
        printf("DeInit CEC error");
    }


    return MI_SUCCESS;
}

static MI_S32 sstar_hvp_init()
{
    MI_S32 ret = MI_SUCCESS;
    /************************************************
    step :init HVP
    *************************************************/
    MI_HVP_DEV u32HvpDevId = _g_HdmiRxPlayer.stHvpChnPort.u32DevId;
    MI_HVP_CHN u32HvpChnId = _g_HdmiRxPlayer.stHvpChnPort.u32ChnId;
    MI_HVP_DeviceAttr_t stHvpDevAttr;
    MI_HVP_ChannelAttr_t stHvpChnAttr;
    MI_HVP_ChannelParam_t stHvpChnParam;


    memset(&stHvpDevAttr, 0, sizeof(MI_HVP_DeviceAttr_t));
    stHvpDevAttr.enSrcType = (MI_HVP_SourceType_e)E_MI_HVP_SRC_TYPE_HDMI;
    ret = MI_HVP_CreateDevice(u32HvpDevId, &stHvpDevAttr);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateDevice error_ret:%d\n", ret);
        return ret;
    }

    stHvpChnAttr.enFrcMode = (MI_HVP_FrcMode_e)E_MI_HVP_FRC_MODE_FBL;

    stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount = 1;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount = ceil(_g_HdmiRxPlayer.hvpFrameRate / 60) + 1;

    printf("[HVP]pqbuf=%d\n", stHvpChnAttr.stPqBufModeConfig.u16BufMaxCount);

    stHvpChnAttr.stPqBufModeConfig.eDmaColor        = E_MI_HVP_COLOR_FORMAT_YUV444;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxWidth   = VEDIO_WIDTH;
    stHvpChnAttr.stPqBufModeConfig.u16BufMaxHeight  = VEDIO_HEIGHT;
    stHvpChnAttr.stPqBufModeConfig.eBufCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stHvpChnAttr.stPqBufModeConfig.eFieldType       = E_MI_SYS_FIELDTYPE_NONE;

    ret = MI_HVP_CreateChannel(u32HvpDevId, u32HvpChnId, &stHvpChnAttr);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateChannel error:%d\n",ret);
        return ret;//err
    }

    memset(&stHvpChnParam, 0x0, sizeof(MI_HVP_ChannelParam_t));
    stHvpChnParam.stChnSrcParam.enPixRepType = E_MI_HVP_PIX_REP_TYPE_1X;
    stHvpChnParam.stChnSrcParam.enInputColor = E_MI_HVP_COLOR_FORMAT_RGB444;
    stHvpChnParam.stChnSrcParam.enColorDepth = E_MI_HVP_COLOR_DEPTH_8;
    stHvpChnParam.stChnSrcParam.enColorRange = E_MI_HVP_COLOR_RANGE_TYPE_FULL;
    stHvpChnParam.stChnSrcParam.stCropWin.u16X = 0;
    stHvpChnParam.stChnSrcParam.stCropWin.u16Y = 0;
    //stHvpChnParam.stChnSrcParam.stCropWin.u16Width = VEDIO_WIDTH;
    //stHvpChnParam.stChnSrcParam.stCropWin.u16Height = VEDIO_HEIGHT;

    stHvpChnParam.stChnDstParam.bFlip = 0;
    stHvpChnParam.stChnDstParam.bMirror = 0;
    stHvpChnParam.stChnDstParam.enColor = E_MI_HVP_COLOR_FORMAT_YUV444;
    stHvpChnParam.stChnDstParam.u16Fpsx100 = 60;
    stHvpChnParam.stChnDstParam.u16Width = VEDIO_WIDTH;
    stHvpChnParam.stChnDstParam.u16Height = VEDIO_HEIGHT;
    //stHvpChnParam.stChnDstParam.stDispWin.u16X = 0;
    //stHvpChnParam.stChnDstParam.stDispWin.u16Y = 0;
    //stHvpChnParam.stChnDstParam.stDispWin.u16Width = VEDIO_WIDTH;
    //stHvpChnParam.stChnDstParam.stDispWin.u16Height = VEDIO_HEIGHT;
    stHvpChnParam.stChnDstParam.stCropWin.u16X = 0;
    stHvpChnParam.stChnDstParam.stCropWin.u16Y = 0;
    //stHvpChnParam.stChnDstParam.stCropWin.u16Width = VEDIO_WIDTH;
    //stHvpChnParam.stChnDstParam.stCropWin.u16Height = VEDIO_HEIGHT;
    ret = MI_HVP_SetChannelParam(u32HvpDevId, u32HvpChnId, &stHvpChnParam);
    if (MI_SUCCESS != ret)
    {
        return ret;
    }

    ret = MI_HVP_StartChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_CreateChannel error\n");
        return ret;
    }

    printf("MI Hvp Init done\n");
    return MI_SUCCESS;
}

static MI_S32 sstar_hvp_deinit()
{
    MI_S32 ret = MI_SUCCESS;
    MI_HVP_DEV u32HvpDevId = _g_HdmiRxPlayer.stHvpChnPort.u32DevId;
    MI_HVP_CHN u32HvpChnId = _g_HdmiRxPlayer.stHvpChnPort.u32ChnId;

    ret = MI_HVP_StopChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_StopChannel error");
        return ret;
    }

    ret = MI_HVP_DestroyChannel(u32HvpDevId, u32HvpChnId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_DestroyChannel error");
        return ret;
    }

    ret = MI_HVP_DestroyDevice(u32HvpDevId);
    if (ret != MI_SUCCESS) {
        printf("MI_HVP_DestroyDevice error");
        return ret;
    }
    return MI_SUCCESS;
}


static void hdmi_player_event_cb(hdmi_player_event_e event)
{
    printf("%s: event:%d\n", __FUNCTION__, event);
}

void hdmi_player_video_frame_drop_cb(hdmi_player_video_frame_t frame)
{
    if (!frame.buf_handle) {
        printf("%s: frame buf_handle:%p pts:%" PRIi64 "\n", __FUNCTION__, frame.buf_handle, frame.pts);
        return;
    }
    //St_ListNode_t *ListVideoOutput = (St_ListNode_t *)frame.buf_handle;
    //add_tail_node(&g_HeadListVideoOutput, &ListVideoOutput->list);
    printf("%s: frame buf_handle:%p pts:%" PRIi64 "\n", __FUNCTION__, frame.buf_handle, frame.pts);
}

void hdmi_player_video_frame_flip_cb(hdmi_player_video_frame_t frame)
{
    if (!frame.buf_handle) {
        printf("%s: frame buf_handle:%p pts:%" PRIi64 "\n", __FUNCTION__, frame.buf_handle, frame.pts);
        return;
    }
    //St_ListNode_t *ListVideoOutput = (St_ListNode_t *)frame.buf_handle;
    //add_tail_node(&g_HeadListVideoCommit, &ListVideoOutput->list);
    //printf("%s: frame buf_handle:%p pts:%" PRIi64 "\n", __FUNCTION__, frame.buf_handle, frame.pts);
}

static MI_S32 sstar_HdmiPipeLine_Creat()
{
    MI_S32 ret = MI_SUCCESS;

    g_bThreadExitHdmi = true;
    g_bThreadExitScl = true;
    g_bThreadExitHdmiAudio = true;
    hvp_event_thread_running = true;

    ret = sstar_HdmiList_Init();
    if (ret < 0)
    {
        printf("Error: sstar_hdmirx_context_init fail \n");
        return -1;
    }

    sstar_hdmirx_context_init();

    ret = sstar_hdmirx_init();
    if (ret != MI_SUCCESS) {
        printf("sstar_hdmirx_init error: %d\n", ret);
        return ret;
    }
    ret = sstar_hvp_init();
    if (ret != MI_SUCCESS) {
        printf("sstar_hdmirx_init error: %d\n", ret);
        return ret;
    }
    _g_HdmiRxPlayer.pIsCreated = true;
    pthread_create(&hdmi_detect_thread, NULL, sstar_HdmiPlugsDetect_Thread, NULL);
    pthread_create(&hvp_event_thread, NULL, sstar_HvpEventMoniter_Thread, NULL);

    ret = sstar_scl_init();
    if (ret != MI_SUCCESS) {
        printf("sstar_hdmirx_init error: %d\n", ret);
        return ret;
    }

    hdmi_player_config_t hdmi_cfg;
    hdmi_cfg.event_cb = hdmi_player_event_cb;
    hdmi_cfg.ao_dev = E_HDMI_PLAYER_AO_DEV_SPEAKER;
    hdmi_cfg.ao_format = E_HDMI_PLAYER_AO_FORMAT_PCM;
    hdmi_cfg.game_mode = false;
    hdmi_cfg.mute = false;
    hdmi_cfg.volume = 60;
    hdmi_cfg.ao_sample_rate = E_HDMI_PLAYER_AIO_SAMPLE_RATE_ORIGINAL;
    hdmi_cfg.drm_info.fd = g_stDrmCfg.fd;
    hdmi_cfg.drm_info.panel_refresh_rate = g_stDrmCfg.Connector->modes->vrefresh;
    hdmi_cfg.video_frame_drop_cb = hdmi_player_video_frame_drop_cb;
    hdmi_cfg.video_frame_flip_cb = hdmi_player_video_frame_flip_cb;
    ret = hdmi_player_open(hdmi_cfg);
    if (ret < 0)
    {
        printf("hdmi_player_open error: %d\n", ret);
        return ret;
    }
    STCHECKRESULT(MI_SYS_BindChnPort2(0, &_g_HdmiRxPlayer.stHvpChnPort, &_g_HdmiRxPlayer.stSclChnPort, _g_HdmiRxPlayer.hvpFrameRate, _g_HdmiRxPlayer.sclFrameRate, _g_HdmiRxPlayer.eBindType, 0));
    pthread_create(&g_pThreadScl,  NULL, sstar_SclEnqueue_thread, &_g_HdmiRxPlayer.stSclChnPort);
    pthread_create(&g_pThreadHdmi, NULL, sstar_HdmiRxProcess_Thread, &_g_HdmiRxPlayer.stSclChnPort);
    pthread_create(&g_pThreadHdmiAudio, NULL, sstar_HDMI_AudioProcess_Thread, NULL);

    return MI_SUCCESS;
}

static MI_S32 sstar_HdmiPipeLine_Destory()
{
    _g_HdmiRxPlayer.player_working = false;
    _g_HdmiRxPlayer.pIsCreated = false;
    g_bThreadExitHdmiAudio = false;
    g_bThreadExitHdmi = false;
    g_bThreadExitScl = false;
    hvp_event_thread_running = false;

    if(g_pThreadHdmiAudio)//hdmi or video
    {
        pthread_join(g_pThreadHdmiAudio, NULL);
        g_pThreadHdmiAudio = NULL;
    }

    if(g_pThreadHdmi)//hdmi or video
    {
        pthread_join(g_pThreadHdmi, NULL);
        g_pThreadHdmi = NULL;
    }

    if(g_pThreadScl)
    {
        pthread_join(g_pThreadScl, NULL);
        g_bThreadExitScl = NULL;
    }

    if(hvp_event_thread)
    {
        hvp_event_thread_running = false;
        pthread_join(hvp_event_thread, NULL);
        hvp_event_thread = NULL;

    }

    if(hdmi_detect_thread)
    {
        pthread_join(hdmi_detect_thread, NULL);
        hdmi_detect_thread = NULL;
    }

    hdmi_player_close();

    /************************************************
    step :unbind HVP -> SCL
    *************************************************/
    STCHECKRESULT(MI_SYS_UnBindChnPort(0, &_g_HdmiRxPlayer.stHvpChnPort, &_g_HdmiRxPlayer.stSclChnPort));

    /************************************************
    step :deinit SCL
    *************************************************/
    sstar_scl_deinit();

    /************************************************
    step :deinit HVP
    *************************************************/
    sstar_hvp_deinit();

    /************************************************
    step :deinit HDMI_RX
    *************************************************/
    sstar_hdmirx_deinit();

    sstar_HdmiList_DeInit();

    return MI_SUCCESS;
}

static MI_S32 sstar_MediaPipeLine_Creat()
{
    int ret;
    g_bThreadExitGfx = true;
    g_bThreadExitPlayer = true;
    ret = sstar_MediaList_Init();
    if (ret < 0)
    {
        printf("Error: sstar_MediaList_Init fail \n");
        return NULL;
    }
    sstar_media_context_init();
    ret = mm_player_open(&_g_MediaPlayer.param);
    if (ret < 0)
    {
        printf("mm_player_open fail \n");
        return -1;
    }
    //_g_MediaPlayer.player_working = true;

    _g_MediaPlayer.pIsCreated = true;
    pthread_create(&g_player_thread, NULL, sstar_PlayerMoniter_Thread, NULL);
    pthread_create(&g_pThreadGfx, NULL, sstar_VideoProcess_Thread, NULL);
    pthread_create(&g_pThreadAudio, NULL, sstar_Media_AudioProcess_Thread, NULL);

    return MI_SUCCESS;
}


static MI_S32 sstar_MediaPipeLine_Destroy()
{

    _g_HdmiRxPlayer.player_working = false;
    _g_MediaPlayer.pIsCreated = false;
    g_bThreadExitGfx = false;
    g_bThreadExitPlayer = false;

    if(g_pThreadGfx)//video
    {
        pthread_join(g_pThreadGfx, NULL);
        g_pThreadGfx = NULL;
    }

    if(g_player_thread)
    {
        pthread_join(g_player_thread, NULL);
        g_player_thread = NULL;
    }
    if(g_pThreadAudio)
    {
        pthread_join(g_pThreadAudio, NULL);
        g_pThreadAudio = NULL;
    }
    mm_player_close();
    sstar_MediaList_DeInit();
    return MI_SUCCESS;

}

MI_S32 sstar_set_volume(int volume)
{
    int ret = 0;
    if(_g_MediaPlayer.pIsCreated)
    {
        ret = mm_player_set_volume(volume);
    }
    else if(_g_HdmiRxPlayer.pIsCreated)
    {
        ret = hdmi_player_set_volume(volume);
    }
    return ret;
}

MI_S32 sstar_set_mute(bool mute)
{
    int ret = 0;
    if(_g_MediaPlayer.pIsCreated)
    {
        ret =mm_player_set_mute(mute);
    }
    else if(_g_HdmiRxPlayer.pIsCreated)
    {
        ret =hdmi_player_set_mute(mute);
    }
    return ret;
}


static MI_S32 sstar_BaseModule_Init()
{
    sstar_sys_init();

    if(bShowUi)
    {
        g_CurOsdInfo.pos_x = 0;
        g_CurOsdInfo.pos_y = 0;
        g_CurOsdInfo.out_width = g_stDrmCfg.width;
        g_CurOsdInfo.out_height = g_stDrmCfg.height;
        sstar_draw_canvas(g_CurOsdInfo);
        pthread_create(&g_pThreadUiDrm, NULL, sstar_OsdProcess_Thread, NULL);
    }

    if (g_u8PipelineMode == 0)
    {
        sstar_HdmiPipeLine_Creat();
        //HDMI_RX -> HVP -> SCL
    }
    else if (g_u8PipelineMode == 1)
    {
        sstar_MediaPipeLine_Creat();
        //File -> FFMPEG -> SCL
    }
    else if (g_u8PipelineMode == 2)
    {
        sstar_MediaList_Init();
        sstar_SnrPipeLine_Creat(0, g_InputFile);
        g_bThreadExitReceive = true;
        pthread_create(&g_pThreadSnrReceive, NULL, sstar_SensorFrame_Receive, NULL);
    }

    pthread_create(&g_pThreadUpdatePoint, NULL, sstar_PointOffsetMoniter_Thread, NULL);
    pthread_create(&g_pThreadCommit,      NULL, sstar_DrmCommit_Thread, NULL);
    return MI_SUCCESS;
}


static MI_S32 sstar_BaseModule_DeInit()
{

    if(g_pThreadUpdatePoint)
    {
        g_bThreadExitUpdatePoint = false;
        pthread_join(g_pThreadUpdatePoint, NULL);
        g_pThreadUpdatePoint = NULL;
    }

    if(g_pThreadCommit)
    {
        g_bThreadExitCommit = false;
        pthread_join(g_pThreadCommit, NULL);
        g_pThreadCommit = NULL;
    }

    if(g_pThreadUiDrm)
    {
        g_bThreadExitUiDrm = false;
        pthread_join(g_pThreadUiDrm, NULL);
        g_pThreadUiDrm = NULL;
    }
    if(_g_MainSorCanvas.pIsCreated)
    {
        sttar_Cursor_Destory();
    }

    if (g_u8PipelineMode == 0)
    {
        sstar_HdmiPipeLine_Destory();
        //HDMI_RX -> HVP -> SCL -> GPU -> Drm
    }
    else if (g_u8PipelineMode == 1)
    {
        sstar_MediaPipeLine_Destroy();
    }
    else if (g_u8PipelineMode == 2)
    {
        if(g_pThreadSnrReceive)
        {
            g_bThreadExitReceive = false;
            pthread_join(g_pThreadSnrReceive, NULL);
            g_pThreadSnrReceive = NULL;
        }
        sstar_SnrPipeLine_Destory(0);
        sstar_MediaList_DeInit();
    }

    STCHECKRESULT(MI_SYS_Exit(0));

    return MI_SUCCESS;

}


void usage_help(void)
{
    printf("Usage:./prog_keystone -m 0 -u xxx -w xxx -h xxx ) HDMI_RX->HVP->SCL->GPU->DRM\n");
    printf("Usage:./prog_keystone -m 1 -i xxx -u xxx -w xxx -h xxx ) FILE->FFMPEG->SCL->GPU->DRM\n");
}

MI_S32 parse_args(int argc, char **argv)
{
    MI_BOOL bInputVedioFile = false;
    g_u8PipelineMode = 0xff;
    for (int i = 0; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "-m"))
        {
            g_u8PipelineMode = atoi(argv[i+1]);
            if((g_u8PipelineMode != 0) && (g_u8PipelineMode != 1) && (g_u8PipelineMode != 2))
            {
                printf("<error>[-m]:Only supports mode0 ~ mode3\n");
                return -1;
            }
        }
        if (0 == strcmp(argv[i], "-i"))
        {
            g_InputFile = (char*)malloc(128);
            if(g_InputFile != NULL)
            {
                strcpy(g_InputFile, argv[i + 1]);
            }
            bInputVedioFile = true;
        }
        if (0 == strcmp(argv[i], "-l"))
        {
            _g_MediaPlayer.loop_mode = atoi(argv[i+1]);
            if(_g_MediaPlayer.loop_mode < 0 || _g_MediaPlayer.loop_mode > 1)
            {
                printf("<error>[-l]:Only support 0-1\n");
                return -1;
            }

        }
        if (0 == strcmp(argv[i], "-r"))
        {
            _g_rotate_mode  = atoi(argv[i+1]);
            //_g_MediaPlayer.rotate  = atoi(argv[i+1]);
            _g_MediaPlayer.video_info.rotate = _g_rotate_mode;

            if(_g_rotate_mode > 3)
            {
                printf("<error>[-r]:Only supports 0-3\n");

                return -1;
            }
            if(_g_rotate_mode != 0)
            {
                g_bGpuProcessNeed = true;
            }
        }
        if (0 == strcmp(argv[i], "-u"))
        {
            bShowUi = 1;
            strcpy(g_InputUiPath, argv[i + 1]);
        }
        if (0 == strcmp(argv[i], "-w"))
        {
            if(bShowUi == 0)
            {
                printf("<error>[-w]:should first input - u to select the input UI image\n");
                return -1;
            }
            _g_MediaPlayer.ui_info.in_width = atoi(argv[i+1]);  //UI SRC SIZE

        }

        if (0 == strcmp(argv[i], "-h"))
        {
            if(bShowUi == 0)
            {
                printf("<error>[-h]:should first input - u to select the input UI image\n");
                return -1;
            }
            _g_MediaPlayer.ui_info.in_height = atoi(argv[i+1]);  //UI SRC SIZE
        }

        if (0 == strcmp(argv[i], "-p"))
        {
            _g_MediaPlayer.video_info.ratio = atoi(argv[i+1]);
            _g_MediaPlayer.video_info.ratio = _g_MediaPlayer.video_info.ratio > AV_AUTO_MODE ? AV_SCREEN_MODE : _g_MediaPlayer.video_info.ratio;
            //mm_player_set_opts("video_ratio", "", _g_MediaPlayer.video_info.ratio);
            printf("video_ratio:%d\n", _g_MediaPlayer.video_info.ratio);
        }
    }

    if(g_u8PipelineMode == 1)
    {
        if(bInputVedioFile != true)
        {
            printf("<error>[-i]:mode 1 requires input of video stream file\n");
            return -1;
        }
    }
    if(bShowUi)
    {
        if((_g_MediaPlayer.ui_info.in_height == 0) || (_g_MediaPlayer.ui_info.in_width == 0))
        {
            printf("<error>[-u]:Should Input '-w/-h' To Set UI Image Size\n");
            return -1;
        }
    }
    return MI_SUCCESS;
}

void sstar_CmdParse_Pause(void)
{
    char ch;
    MI_U16 u16GpuMaxW = g_stDrmCfg.width;
    MI_U16 u16GpuMaxH = g_stDrmCfg.height;
    printf("press q to exit\n");
    while ((ch = getchar()) != 'q')
    {
        switch(ch)
        {
            case 'a':
                printf("======== GpuLT[+] =======\n");
                if((g_u32GpuLT_X + 10 + g_u32GpuRT_X >= u16GpuMaxW) || (g_u32GpuLT_Y + 10 + g_u32GpuLB_Y >= u16GpuMaxH))
                {
                    printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
                }
                else
                {
                        g_u32GpuLT_X = g_u32GpuLT_X + 10;
                        g_u32GpuLT_Y = g_u32GpuLT_Y + 10;
                        g_bChangePointOffset = true;
                }
                break;
            case 'b':
                {
                    printf("======== GpuLT[-] =======\n");
                    if(g_u32GpuLT_X < 10)
                    {
                        g_u32GpuLT_X = 0;
                        g_u32GpuLT_Y = 0;
                        printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
                    }
                    else
                    {
                        g_u32GpuLT_X = g_u32GpuLT_X - 10;
                        g_u32GpuLT_Y = g_u32GpuLT_Y - 10;
                        g_bChangePointOffset = true;
                    }
                }

                break;

            case 'c':
                {
                    printf("======== GpuLB[+] =======\n");
                    if((g_u32GpuLB_X + 10 + g_u32GpuRB_X >= u16GpuMaxW) || (g_u32GpuLB_Y + 10 + g_u32GpuLT_Y >= u16GpuMaxH))
                    {
                        printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
                    }
                    else
                    {
                            g_u32GpuLB_X = g_u32GpuLB_X + 10;
                            g_u32GpuLB_Y = g_u32GpuLB_Y + 10;
                            g_bChangePointOffset = true;
                    }
                }

                break;
            case 'd':
                {
                    printf("======== GpuLB[-] =======\n");
                    if(g_u32GpuLB_X < 10)
                    {
                        g_u32GpuLB_X = 0;
                        g_u32GpuLB_Y = 0;
                        printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
                    }
                    else
                    {
                        g_u32GpuLB_X = g_u32GpuLB_X - 10;
                        g_u32GpuLB_Y = g_u32GpuLB_Y - 10;
                        g_bChangePointOffset = true;
                    }
                }

                break;
            case 'e':
                {
                    printf("======== GpuRT[+] =======\n");
                    if((g_u32GpuRT_X + 10 + g_u32GpuLT_X >= u16GpuMaxW) || (g_u32GpuRT_Y + 10 + g_u32GpuRB_X >= u16GpuMaxH))
                    {
                        printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
                    }
                    else
                    {
                            g_u32GpuRT_X = g_u32GpuRT_X + 10;
                            g_u32GpuRT_Y = g_u32GpuRT_Y + 10;
                            g_bChangePointOffset = true;
                    }
                }

                break;
            case 'f':
                {
                    printf("======== GpuRT[-] =======\n");
                    if(g_u32GpuRT_X < 10)
                    {
                        g_u32GpuRT_X = 0;
                        g_u32GpuRT_Y = 0;
                        printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
                    }
                    else
                    {
                        g_u32GpuRT_X = g_u32GpuRT_X - 10;
                        g_u32GpuRT_Y = g_u32GpuRT_Y - 10;
                        g_bChangePointOffset = true;
                    }
                }

                break;

            case 'g':
                {
                    printf("======== GpuRB[+] =======\n");
                    if((g_u32GpuRB_X + 10 + g_u32GpuLB_X >= u16GpuMaxW) || (g_u32GpuRB_Y + 10 + g_u32GpuRT_X >= u16GpuMaxH))
                    {
                        printf("Set Gpu PointOffse Fail, the PointOffse can not More than disp w/3 h/3 !!!\n");
                    }
                    else
                    {
                            g_u32GpuRB_X = g_u32GpuRB_X + 10;
                            g_u32GpuRB_Y = g_u32GpuRB_Y + 10;
                            g_bChangePointOffset = true;
                    }
                }

                break;
            case 'h':
                {
                    printf("======== GpuRB[-] =======\n");
                    if(g_u32GpuRB_X < 10)
                    {
                        g_u32GpuRB_X = 0;
                        g_u32GpuRB_Y = 0;
                        printf("Set Gpu PointOffse Fail, the PointOffse can not Less than zero !!!\n");
                    }
                    else
                    {
                        g_u32GpuRB_X = g_u32GpuRB_X - 10;
                        g_u32GpuRB_Y = g_u32GpuRB_Y - 10;
                        g_bChangePointOffset = true;
                    }
                }
                break;
            case 'l':
                {
                    g_picQuality.u32Contrast --;
                    g_picQuality.u32Hue --;
                    g_picQuality.u32Saturation --;
                    g_picQuality.u32Sharpness --;
                    sstar_set_pictureQuality(g_picQuality);
                }
                break;
            case 'k':
                {
                    g_picQuality.u32Contrast ++;
                    g_picQuality.u32Hue ++;
                    g_picQuality.u32Saturation ++;
                    g_picQuality.u32Sharpness ++;
                    sstar_set_pictureQuality(g_picQuality);
                }
                break;

            case 't'://exit video
                {
                    if(g_player_thread)
                    {
                        g_bThreadExitPlayer = false;
                        pthread_join(g_player_thread, NULL);
                        g_player_thread = NULL;
                    }
                    if(g_pThreadGfx)
                    {
                        g_bThreadExitGfx = false;
                        pthread_join(g_pThreadGfx, NULL);
                        g_pThreadGfx = NULL;
                    }

                }
                break;
            case 'y'://exit osd
                {
                    if(g_pThreadUiDrm)
                    {
                        g_bThreadExitUiDrm = false;
                        pthread_join(g_pThreadUiDrm, NULL);
                        g_pThreadUiDrm = NULL;
                    }

                }
                break;

            case 'u'://set ratio
                {
                    _g_MediaPlayer.video_info.ratio++;
                    _g_MediaPlayer.video_info.ratio = _g_MediaPlayer.video_info.ratio > AV_AUTO_MODE ? AV_ORIGIN_MODE : _g_MediaPlayer.video_info.ratio;
                    sstar_set_ratio(_g_MediaPlayer.video_info.ratio);
                }
                break;
            case 's'://Change Source
                {
                    if(g_u8PipelineMode == 0)
                    {
                        sstar_HdmiPipeLine_Destory();
                        g_u8PipelineMode = 1;
                        sstar_MediaPipeLine_Creat();
                    }
                    else if(g_u8PipelineMode == 1)
                    {
                        sstar_MediaPipeLine_Destroy();
                        g_u8PipelineMode = 0;
                        sstar_HdmiPipeLine_Creat();
                    }
                    #if 0
                    if(g_u8PipelineMode == 0)
                    {
                        sstar_HdmiPipeLine_Destory();
                        g_u8PipelineMode ++;
                        sstar_MediaPipeLine_Creat();
                    }
                    else if(g_u8PipelineMode == 1)
                    {
                        sstar_MediaPipeLine_Destroy();
                        g_u8PipelineMode ++;
                        sstar_MediaList_Init();
                        sstar_SnrPipeLine_Creat(0, NULL);
                        g_bThreadExitReceive = true;
                        pthread_create(&g_pThreadSnrReceive, NULL, sstar_SensorFrame_Receive, NULL);
                    }
                    else if(g_u8PipelineMode == 2)
                    {
                        if(g_pThreadSnrReceive)
                        {
                            g_bThreadExitReceive = false;
                            pthread_join(g_pThreadSnrReceive, NULL);
                            g_pThreadSnrReceive = NULL;
                        }
                        sstar_SnrPipeLine_Destory(0);
                        sstar_MediaList_DeInit();
                        g_u8PipelineMode = 0;
                        sstar_HdmiPipeLine_Creat();
                    }
                    #endif
                }
                break;
            case 'p':
            {
                g_tmp_x += 30;
                g_tmp_y += 30;
                sttar_Cursor_Creat();
                sstar_set_cursorxy(g_tmp_x, g_tmp_y);
            }
            break;
            case 'o':
            {
                /*drm_sstar_get_luma luma;
                sstar_set_DLC(1);
                sstar_get_luma(luma);
                printf("11avgVal = %d,maxVal = %d,minVal = %d\n",luma.avgVal,luma.maxVal,luma.minVal);
                uint16_t test[256];
                memset(test, 0x00 , sizeof(uint16_t) * 256);
                sstar_get_histInfo(test);
                for (int i=0; i<256; i++){
                    if(0 == (i%16))
                    {
                        printf("\n");
                    }
                    printf("ox%x ",test[i]);
                }
                printf("\n");*/
                sstar_clear_plane(GOP_CURSOR_ID);
            }

            break;
            //default:
               // break;
        }
        printf("press q to exit\n");
        usleep(10 * 1000);
        continue;
    }
}


MI_S32 main(int argc, char **argv)
{
    int ret;
    g_u32GpuLT_X = 0;
    g_u32GpuLT_Y = 0;

    g_u32GpuLB_X = 0;
    g_u32GpuLB_Y = 0;

    g_u32GpuRT_X = 0;
    g_u32GpuRT_Y = 0;

    g_u32GpuRB_X = 0;
    g_u32GpuRB_Y = 0;

    memset(&_g_MediaPlayer, 0, sizeof(media_player_t));
    memset(&g_picQuality, 0, sizeof(St_Csc_t));
    ret = parse_args(argc, argv);
    if(ret != MI_SUCCESS)
    {
        printf("<error>:Get Invalid Parameter,Please Check The Parameters!\n");
        usage_help();
        return -1;
    }
    ret = sstar_init_drm();
    if(ret != 0)
    {
        printf("init_drm fail \n");
        return -1;
    }
    //Get pictureQuality
    sstar_get_pictureQuality(&g_picQuality);
    STCHECKRESULT(sstar_BaseModule_Init());

    sstar_set_mute(false);
    
    sstar_set_volume(30);
    sstar_CmdParse_Pause();

    STCHECKRESULT(sstar_BaseModule_DeInit());
    sstar_sorcanvas_deinit();
    sstar_deinit_drm();

    return 0;
}
