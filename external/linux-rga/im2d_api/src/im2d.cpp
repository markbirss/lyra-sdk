/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd.
 * Authors:
 *  PutinLee <putin.lee@rock-chips.com>
 *  Cerf Yu <cerf.yu@rock-chips.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "im2d_rga"
#else
#define LOG_TAG "im2d_rga"
#endif

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "RgaUtils.h"
#include "utils.h"
#include "core/rga_sync.h"
#include "im2d_hardware.h"
#include "im2d_context.h"
#include "im2d_impl.h"
#include "im2d_log.h"

#ifdef __cplusplus
#include <sstream>

#include "im2d.hpp"

#ifdef ANDROID
#include "core/NormalRga.h"
#include "RockchipRga.h"

using namespace android;
#endif /* #ifdef ANDROID */

using namespace std;
#else
#include "im2d.h"

#undef imresize
#undef imcrop
#undef imrotate
#undef imcopy
#undef imflip
#undef imcomposite
#undef imblend
#undef imcolorkey
#undef imfill
#undef impalette
#undef imtranslate
#undef imcvtcolor
#undef imquantize
#undef imrop
#undef imgaussianBlur
#endif /* #ifdef __cplusplus */

extern __thread im_context_t g_im2d_context;
extern __thread char g_rga_err_str[IM_ERR_MSG_LEN];

IM_API const char* imStrError_t(IM_STATUS status) {
    const char *error_type[] = {
        "Fatal error",
        "Unsupported function",
        "Memory overflow",
        "Invalid parameters",
        "Illegal parameters",
        "Version verification failed",
        "No session",
    };
    static __thread char error_str[IM_ERR_MSG_LEN] = "The current error message is empty!";
    const char *ptr = NULL;

    switch(status) {
        case IM_STATUS_NOERROR :
            return "No errors during operation";

        case IM_STATUS_SUCCESS :
            return "Run successfully";

        case IM_STATUS_FAILED :
            ptr = error_type[IM_ERROR_FAILED];
            break;

        case IM_STATUS_NOT_SUPPORTED :
            ptr = error_type[IM_ERROR_NOT_SUPPORTED];
            break;

        case IM_STATUS_OUT_OF_MEMORY :
            ptr = error_type[IM_ERROR_OUT_OF_MEMORY];
            break;

        case IM_STATUS_INVALID_PARAM :
            ptr = error_type[IM_ERROR_INVALID_PARAM];
            break;

        case IM_STATUS_ILLEGAL_PARAM :
            ptr = error_type[IM_ERROR_ILLEGAL_PARAM];
            break;

        case IM_STATUS_ERROR_VERSION :
            ptr = error_type[IM_ERROR_ERROR_VERSION];
            break;

        case IM_STATUS_NO_SESSION :
            ptr = error_type[IM_ERROR_NO_SESSION];
            break;

        default :
            return "unkown status";
    }

    snprintf(error_str, IM_ERR_MSG_LEN, "%s: %s", ptr, g_rga_err_str);
    rga_error_msg_set("No error message, it has been cleared.");

    return error_str;
}

IM_API rga_buffer_handle_t importbuffer_fd(int fd, im_handle_param_t *param) {
    return rga_import_buffer_param((uint64_t)fd, RGA_DMA_BUFFER, param);
}

#ifdef __cplusplus
IM_API rga_buffer_handle_t importbuffer_fd(int fd, int size) {
    return rga_import_buffer((uint64_t)fd, RGA_DMA_BUFFER, (uint32_t)size);
}

IM_API rga_buffer_handle_t importbuffer_fd(int fd, int width, int height, int format) {
    im_handle_param_t param = {(uint32_t)width, (uint32_t)height, (uint32_t)format};
    return rga_import_buffer_param((uint64_t)fd, RGA_DMA_BUFFER, &param);
}
#endif

IM_API rga_buffer_handle_t importbuffer_virtualaddr(void *va, im_handle_param_t *param) {
    return rga_import_buffer_param(ptr_to_u64(va), RGA_VIRTUAL_ADDRESS, param);
}

#ifdef __cplusplus
IM_API rga_buffer_handle_t importbuffer_virtualaddr(void *va, int size) {
    return rga_import_buffer(ptr_to_u64(va), RGA_VIRTUAL_ADDRESS, (uint32_t)size);
}

IM_API rga_buffer_handle_t importbuffer_virtualaddr(void *va, int width, int height, int format) {
    im_handle_param_t param = {(uint32_t)width, (uint32_t)height, (uint32_t)format};
    return rga_import_buffer_param(ptr_to_u64(va), RGA_VIRTUAL_ADDRESS, &param);
}
#endif

IM_API rga_buffer_handle_t importbuffer_physicaladdr(uint64_t pa, im_handle_param_t *param) {
    return rga_import_buffer_param(pa, RGA_PHYSICAL_ADDRESS, param);
}

#ifdef __cplusplus
IM_API rga_buffer_handle_t importbuffer_physicaladdr(uint64_t pa, int size) {
    return rga_import_buffer(pa, RGA_PHYSICAL_ADDRESS, (uint32_t)size);
}

IM_API rga_buffer_handle_t importbuffer_physicaladdr(uint64_t pa, int width, int height, int format) {
    im_handle_param_t param = {(uint32_t)width, (uint32_t)height, (uint32_t)format};
    return rga_import_buffer_param(pa, RGA_PHYSICAL_ADDRESS, &param);
}
#endif

IM_API IM_STATUS releasebuffer_handle(rga_buffer_handle_t handle) {
    return rga_release_buffer(handle);
}

static inline void set_default_rga_buffer(rga_buffer_t *buffer,
                                          int width, int height, int format,
                                          int wstride, int hstride) {

    buffer->width = width;
    buffer->height = height;
    buffer->wstride = wstride;
    buffer->hstride = hstride;
    buffer->format = format;

    buffer->global_alpha = 0xff;
    buffer->color_space_mode = IM_COLOR_SPACE_DEFAULT;
    buffer->rd_mode = IM_RASTER_MODE;
}

#undef wrapbuffer_virtualaddr
static rga_buffer_t wrapbuffer_virtualaddr(void* vir_addr,
                                           int width, int height, int format,
                                           int wstride, int hstride) {
    rga_buffer_t buffer;

    memset(&buffer, 0, sizeof(rga_buffer_t));

    buffer.vir_addr = vir_addr;
    set_default_rga_buffer(&buffer, width, height, format,
                           wstride ? wstride : width,
                           hstride ? hstride : height);

    return buffer;
}

#undef wrapbuffer_physicaladdr
static rga_buffer_t wrapbuffer_physicaladdr(void* phy_addr,
                                            int width, int height, int format,
                                            int wstride, int hstride) {
    rga_buffer_t buffer;

    memset(&buffer, 0, sizeof(rga_buffer_t));

    buffer.phy_addr = phy_addr;
    set_default_rga_buffer(&buffer, width, height, format,
                           wstride ? wstride : width,
                           hstride ? hstride : height);

    return buffer;
}

#undef wrapbuffer_fd
static rga_buffer_t wrapbuffer_fd(int fd,
                                  int width, int height, int format,
                                  int wstride, int hstride) {
    rga_buffer_t buffer;

    memset(&buffer, 0, sizeof(rga_buffer_t));

    buffer.fd = fd;
    set_default_rga_buffer(&buffer, width, height, format,
                           wstride ? wstride : width,
                           hstride ? hstride : height);

    return buffer;
}

#undef wrapbuffer_handle
IM_API rga_buffer_t wrapbuffer_handle(rga_buffer_handle_t  handle,
                                      int width, int height, int format,
                                      int wstride, int hstride) {
    rga_buffer_t buffer;

    memset(&buffer, 0, sizeof(rga_buffer_t));

    buffer.handle = handle;
    set_default_rga_buffer(&buffer, width, height, format,
                           wstride ? wstride : width,
                           hstride ? hstride : height);

    return buffer;
}

#ifdef __cplusplus
IM_API rga_buffer_t wrapbuffer_handle(rga_buffer_handle_t  handle,
                                      int width, int height,
                                      int format) {
    return wrapbuffer_handle(handle, width, height, format, width, height);
}
#endif

#ifdef ANDROID
IM_API rga_buffer_handle_t importbuffer_GraphicBuffer_handle(buffer_handle_t hnd) {
    int ret = 0;
    int fd = -1;
    void *virt_addr = NULL;
    std::vector<int> dstAttrs;
    im_handle_param_t param;

    RockchipRga& rkRga(RockchipRga::get());

    ret = RkRgaGetHandleAttributes(hnd, &dstAttrs);
    if (ret) {
        IM_LOGE("handle get Attributes fail ret = %d,hnd=%p", ret, &hnd);
        return -1;
    }

    param.width = dstAttrs.at(ASTRIDE);
    param.height = dstAttrs.at(AHEIGHT);
    param.format = dstAttrs.at(AFORMAT);

    ret = rkRga.RkRgaGetBufferFd(hnd, &fd);
    if (ret)
        IM_LOGE("rga_im2d: get buffer fd fail: %s, hnd=%p", strerror(errno), (void*)(hnd));

    if (fd <= 0) {
        ret = rkRga.RkRgaGetHandleMapCpuAddress(hnd, &virt_addr);
        if(!virt_addr) {
            IM_LOGE("invaild GraphicBuffer, can not get fd and virtual address, hnd = %p", (void *)hnd);
            return -1;
        } else {
            return importbuffer_virtualaddr(virt_addr, &param);
        }
    } else {
        return importbuffer_fd(fd, &param);
    }
}

IM_API rga_buffer_handle_t importbuffer_GraphicBuffer(sp<GraphicBuffer> buf) {
    return importbuffer_GraphicBuffer_handle(buf->handle);
}

/*When wrapbuffer_GraphicBuffer and wrapbuffer_AHardwareBuffer are used, */
/*it is necessary to check whether fd and virtual address of the return rga_buffer_t are valid parameters*/
IM_API rga_buffer_t wrapbuffer_handle(buffer_handle_t hnd) {
    int ret = 0;
    rga_buffer_t buffer;
    std::vector<int> dstAttrs;

    RockchipRga& rkRga(RockchipRga::get());

    memset(&buffer, 0, sizeof(rga_buffer_t));

    ret = rkRga.RkRgaGetBufferFd(hnd, &buffer.fd);
    if (ret)
        IM_LOGE("rga_im2d: get buffer fd fail: %s, hnd=%p", strerror(errno), (void*)(hnd));

    if (buffer.fd <= 0) {
        ret = rkRga.RkRgaGetHandleMapCpuAddress(hnd, &buffer.vir_addr);
        if(!buffer.vir_addr) {
            IM_LOGE("invaild GraphicBuffer, can not get fd and virtual address, hnd = %p", (void *)hnd);
            goto INVAILD;
        }
    }

    ret = RkRgaGetHandleAttributes(hnd, &dstAttrs);
    if (ret) {
        IM_LOGE("handle get Attributes fail, ret = %d,hnd = %p", ret, (void *)hnd);
        goto INVAILD;
    }

    set_default_rga_buffer(&buffer,
                           dstAttrs.at(AWIDTH), dstAttrs.at(AHEIGHT), dstAttrs.at(AFORMAT),
                           dstAttrs.at(ASTRIDE), dstAttrs.at(AHEIGHT));

    if (buffer.wstride % 16) {
        IM_LOGE("Graphicbuffer wstride needs align to 16, please align to 16 or use other buffer types, wstride = %d", buffer.wstride);
        goto INVAILD;
    }

INVAILD:
    return buffer;
}

IM_API rga_buffer_t wrapbuffer_GraphicBuffer(sp<GraphicBuffer> buf) {
    return wrapbuffer_handle(buf->handle);
}

#if USE_AHARDWAREBUFFER
#include <android/hardware_buffer.h>
IM_API rga_buffer_handle_t importbuffer_AHardwareBuffer(AHardwareBuffer *buf) {
    GraphicBuffer *gbuffer = reinterpret_cast<GraphicBuffer*>(buf);

    return importbuffer_GraphicBuffer_handle(gbuffer->handle);
}

IM_API rga_buffer_t wrapbuffer_AHardwareBuffer(AHardwareBuffer *buf) {
    GraphicBuffer *gbuffer = reinterpret_cast<GraphicBuffer*>(buf);

    return wrapbuffer_handle(gbuffer->handle);
}
#endif
#endif

void imsetAlphaBit(rga_buffer_t *buf, uint8_t alpha0, uint8_t alpha1) {
    buf->alpha_bit.alpha0 = alpha0;
    buf->alpha_bit.alpha1 = alpha1;
}

void imsetOpacity(rga_buffer_t *buf, uint8_t alpha) {
    buf->global_alpha = alpha;
}

void imsetColorSpace(rga_buffer_t *buf, IM_COLOR_SPACE_MODE mode) {
    buf->color_space_mode = mode;
}

void imsetOptGaussianBlur(im_opt_t *opt,
                          int gauss_width, int gauss_height,
                          int sigma_x, int sigma_y) {
    if (opt->version == 0)
        opt->version = RGA_CURRENT_API_HEADER_VERSION;

    opt->gauss_config.ksize.width = gauss_width;
    opt->gauss_config.ksize.height = gauss_height;
    opt->gauss_config.matrix = NULL;
    opt->gauss_config.sigma_x = sigma_x;
    opt->gauss_config.sigma_y = sigma_y;
}

void imsetOptGaussianBlurMatrix(im_opt_t *opt,
                                int gauss_width, int gauss_height,
                                double *matrix) {
    if (opt->version == 0)
        opt->version = RGA_CURRENT_API_HEADER_VERSION;

    opt->gauss_config.ksize.width = gauss_width;
    opt->gauss_config.ksize.height = gauss_height;
    opt->gauss_config.matrix = matrix;
}

#ifdef __cplusplus
IM_API const char* querystring(int name) {
    bool all_output = 0, all_output_prepared = 0;
    int rga_version = 0;
    long usage = 0;
    enum {
        RGA_API = 0,
    };
    const char *temp;
    const char *output_vendor = "Rockchip Electronics Co.,Ltd.";
    const char *output_name[] = {
        "RGA vendor            : ",
        "RGA version           : ",
        "Max input             : ",
        "Max output            : ",
        "Byte stride           : ",
        "Scale limit           : ",
        "Input support format  : ",
        "output support format : ",
        "RGA feature           : ",
        "expected performance  : ",
    };
    const char *version_name[] = {
        "RGA_api version       : ",
    };
    const char *output_version[] = {
        "unknown ",
        "RGA_1 ",
        "RGA_1_plus ",
        "RGA_2 ",
        "RGA_2_lite0 ",
        "RGA_2_lite1 ",
        "RGA_2_Enhance ",
        "RGA_2_PRO ",
        "RGA_2_lite2 ",
        "RGA_3 ",
    };
    const char *output_resolution[] = {
        "unknown",
        "2048x2048",
        "4096x4096",
        "8192x8192",
        "8128x8128",
        "2880x1620",
        "1280x1280",
    };
    const char *output_scale_limit[] = {
        "unknown",
        "0.125 ~ 8",
        "0.0625 ~ 16"
    };
    const char *output_format[] = {
        "unknown",
        "RGBA/ARGB_8888 RGB_888 RGB_565 ",
        "ARGB_4444 ARGB_5551 ",
        "RGBA_4444 RGBA_5551 ",
        "BPP8 BPP4 BPP2 BPP1 ",
        "YUV420_sp_8bit ",
        "YUV420_sp_10bit ",
        "YUV420_p_8bit ",
        "YUV420_p_10bit ",
        "YUV422_sp_8bit ",
        "YUV422_sp_10bit ",
        "YUV422_p_8bit ",
        "YUV422_p_10bit ",
        "YUYV420 ",
        "YUYV422 ",
        "YUV400 ",
        "Y4 ",
        "RGB2BPP ",
        "ALPHA-8bit ",
        "YUV444_sp_8bit ",
        "Y8 ",
    };
    const char *feature[] = {
        "unknown ",
        "color_fill ",
        "color_palette ",
        "ROP ",
        "quantize ",
        "src1_r2y_csc ",
        "dst_full_csc ",
        "FBC_mode ",
        "blend_in_YUV ",
        "BT.2020 ",
        "mosaic ",
        "OSD ",
        "early_interruption ",
        "alpha_bit_map ",
        "gauss ",
    };
    const char *performance[] = {
        "unknown",
        "max 1 pixel/cycle ",
        "max 2 pixel/cycle ",
        "max 4 pixel/cycle ",
    };
    ostringstream out;
    static string info;

    rga_session_t *session;
    rga_info_table_entry *rga_info;

    session = get_rga_session();
    if (session == NULL)
        return imStrError(IM_STATUS_NO_SESSION);

    rga_info = &session->hardware_info;

    do {
        switch(name) {
            case RGA_VENDOR :
                out << output_name[name] << output_vendor << endl;
                break;

            case RGA_VERSION :
                out << version_name[RGA_API] << "v" << RGA_API_VERSION << endl;

                out << output_name[name];
                if (rga_info->version == IM_RGA_HW_VERSION_RGA_V_ERR) {
                    out << output_version[IM_RGA_HW_VERSION_RGA_V_ERR_INDEX];
                } else {
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_1)
                        out << output_version[IM_RGA_HW_VERSION_RGA_1_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_1_PLUS)
                        out << output_version[IM_RGA_HW_VERSION_RGA_1_PLUS_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2_LITE0)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_LITE0_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2_LITE1)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_LITE1_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2_ENHANCE)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_ENHANCE_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2_PRO)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_PRO_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_2_LITE2)
                        out << output_version[IM_RGA_HW_VERSION_RGA_2_LITE2_INDEX];
                    if (rga_info->version & IM_RGA_HW_VERSION_RGA_3)
                        out << output_version[IM_RGA_HW_VERSION_RGA_3_INDEX];
                }
                out << endl;
                break;

            case RGA_MAX_INPUT :
                switch (rga_info->input_resolution.width) {
                    case 2048 :
                        out << output_name[name] << output_resolution[1] << endl;
                        break;
                    case 4096 :
                        out << output_name[name] << output_resolution[2] << endl;
                        break;
                    case 8192 :
                        out << output_name[name] << output_resolution[3] << endl;
                        break;
                    case 8128 :
                        out << output_name[name] << output_resolution[4] << endl;
                        break;
                    case 2880 :
                        out << output_name[name] << output_resolution[5] << endl;
                        break;
                    case 1280 :
                        out << output_name[name] << output_resolution[6] << endl;
                        break;
                    default :
                        out << output_name[name] << output_resolution[IM_RGA_HW_VERSION_RGA_V_ERR_INDEX] << endl;
                        break;
                }
                break;

            case RGA_MAX_OUTPUT :
                switch(rga_info->output_resolution.width) {
                    case 2048 :
                        out << output_name[name] << output_resolution[1] << endl;
                        break;
                    case 4096 :
                        out << output_name[name] << output_resolution[2] << endl;
                        break;
                    case 8192 :
                        out << output_name[name] << output_resolution[3] << endl;
                        break;
                    case 8128 :
                        out << output_name[name] << output_resolution[4] << endl;
                        break;
                    case 2880 :
                        out << output_name[name] << output_resolution[5] << endl;
                        break;
                    case 1280 :
                        out << output_name[name] << output_resolution[6] << endl;
                        break;
                    default :
                        out << output_name[name] << output_resolution[IM_RGA_HW_VERSION_RGA_V_ERR_INDEX] << endl;
                        break;
                }
                break;

            case RGA_BYTE_STRIDE :
                if (rga_info->byte_stride > 0)
                    out << output_name[name] << rga_info->byte_stride << " byte" << endl;
                else
                    out << output_name[name] << "unknown" << endl;

                break;

            case RGA_SCALE_LIMIT :
                switch(rga_info->scale_limit) {
                    case 8 :
                        out << output_name[name] << output_scale_limit[1] << endl;
                        break;
                    case 16 :
                        out << output_name[name] << output_scale_limit[2] << endl;
                        break;
                    default :
                        out << output_name[name] << output_scale_limit[IM_RGA_HW_VERSION_RGA_V_ERR_INDEX] << endl;
                        break;
                }
                break;

            case RGA_INPUT_FORMAT :
                out << output_name[name];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_RGB)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGB_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_ARGB_16BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ARGB_16BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_RGBA_16BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGBA_16BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_BPP)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_BPP_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_10_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_10_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_10_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_10_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUYV_420)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUYV_420_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUYV_422)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUYV_422_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_400)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_400_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_Y4)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_Y4_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_RGBA2BPP)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGBA2BPP_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_ALPHA_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ALPHA_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_YUV_444_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_444_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_Y8)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_Y8_INDEX];
                if(!(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_MASK))
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ERROR_INDEX];
                out << endl;
                break;

            case RGA_OUTPUT_FORMAT :
                out << output_name[name];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_RGB)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGB_INDEX];
                if(rga_info->input_format & IM_RGA_SUPPORT_FORMAT_ARGB_16BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ARGB_16BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_RGBA_16BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGBA_16BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_BPP)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_BPP_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_SEMI_PLANNER_10_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_420_PLANNER_10_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_SEMI_PLANNER_10_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_10_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_422_PLANNER_10_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUYV_420)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUYV_420_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUYV_422)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUYV_422_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_400)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_400_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_Y4)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_Y4_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_RGBA2BPP)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_RGBA2BPP_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_ALPHA_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ALPHA_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_YUV_444_SEMI_PLANNER_8_BIT)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_YUV_444_SEMI_PLANNER_8_BIT_INDEX];
                if(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_Y8)
                    out << output_format[IM_RGA_SUPPORT_FORMAT_Y8_INDEX];
                if(!(rga_info->output_format & IM_RGA_SUPPORT_FORMAT_MASK))
                    out << output_format[IM_RGA_SUPPORT_FORMAT_ERROR_INDEX];
                out << endl;
                break;

            case RGA_FEATURE :
                out << output_name[name];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_COLOR_FILL)
                    out << feature[IM_RGA_SUPPORT_FEATURE_COLOR_FILL_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_COLOR_PALETTE)
                    out << feature[IM_RGA_SUPPORT_FEATURE_COLOR_PALETTE_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_ROP)
                    out << feature[IM_RGA_SUPPORT_FEATURE_ROP_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_QUANTIZE)
                    out << feature[IM_RGA_SUPPORT_FEATURE_QUANTIZE_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_SRC1_R2Y_CSC)
                    out << feature[IM_RGA_SUPPORT_FEATURE_SRC1_R2Y_CSC_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_DST_FULL_CSC)
                    out << feature[IM_RGA_SUPPORT_FEATURE_DST_FULL_CSC_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_FBC)
                    out << feature[IM_RGA_SUPPORT_FEATURE_FBC_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_BLEND_YUV)
                    out << feature[IM_RGA_SUPPORT_FEATURE_BLEND_YUV_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_BT2020)
                    out << feature[IM_RGA_SUPPORT_FEATURE_BT2020_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_MOSAIC)
                    out << feature[IM_RGA_SUPPORT_FEATURE_MOSAIC_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_OSD)
                    out << feature[IM_RGA_SUPPORT_FEATURE_OSD_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_PRE_INTR)
                    out << feature[IM_RGA_SUPPORT_FEATURE_PRE_INTR_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_ALPHA_BIT_MAP)
                    out << feature[IM_RGA_SUPPORT_FEATURE_ALPHA_BIT_MAP_INDEX];
                if(rga_info->feature & IM_RGA_SUPPORT_FEATURE_GAUSS)
                    out << feature[IM_RGA_SUPPORT_FEATURE_GAUSS_INDEX];
                out << endl;
                break;

            case RGA_EXPECTED :
                switch(rga_info->performance) {
                    case 1 :
                        out << output_name[name] << performance[1] << endl;
                        break;
                    case 2 :
                        out << output_name[name] << performance[2] << endl;
                        break;
                    case 4 :
                        out << output_name[name] << performance[3] << endl;
                        break;
                    default :
                        out << output_name[name] << performance[IM_RGA_HW_VERSION_RGA_V_ERR_INDEX] << endl;
                        break;
                }
                break;

            case RGA_ALL :
                if (!all_output) {
                    all_output = 1;
                    name = 0;
                } else
                    all_output_prepared = 1;
                break;

            default:
                return "Invalid instruction";
        }

        info = out.str();

        if (all_output_prepared)
            break;
        else if (all_output && strcmp(info.c_str(),"0")>0)
            name++;

    } while(all_output);

    temp = info.c_str();

    return temp;
}
#else
IM_API const char* querystring(int name) {
    return "C API unsupport!";
}
#endif /* #ifdef __cplusplus */

IM_API IM_STATUS imcheckHeader(im_api_version_t header_version) {
    return rga_check_header(RGA_GET_API_VERSION(header_version));
}

void rga_check_perpare(rga_buffer_t *src, rga_buffer_t *dst, rga_buffer_t *pat,
                       im_rect *src_rect, im_rect *dst_rect, im_rect *pat_rect, int mode_usage) {
    (void)(src);
    (void)(dst);
    (void)(pat);
    (void)(src_rect);
    (void)(dst_rect);
    (void)(pat_rect);
    (void)(mode_usage);
}

IM_API IM_STATUS imsync(int fence_fd) {
    int ret = 0;

    if (fence_fd <= 0) {
        IM_LOGE("illegal fence_fd[%d]", fence_fd);
        return IM_STATUS_ILLEGAL_PARAM;
    }

    ret = rga_sync_wait(fence_fd, -1);
    if (ret) {
        IM_LOGE("Failed to wait for out fence = %d, ret = %d", fence_fd, ret);
        return IM_STATUS_FAILED;
    }

    close(fence_fd);

    return IM_STATUS_SUCCESS;
}

IM_API IM_STATUS imconfig(IM_CONFIG_NAME name, uint64_t value) {

    switch (name) {
        case IM_CONFIG_SCHEDULER_CORE :
            if (value & IM_SCHEDULER_MASK) {
                g_im2d_context.core = (IM_SCHEDULER_CORE)value;
            } else {
                IM_LOGE("IM2D: It's not legal rga_core[0x%lx], it needs to be a 'IM_SCHEDULER_CORE'.", (unsigned long)value);
                return IM_STATUS_ILLEGAL_PARAM;
            }
            break;
        case IM_CONFIG_PRIORITY :
            if (value <= 6) {
                g_im2d_context.priority = (int)value;
            } else {
                IM_LOGE("IM2D: It's not legal priority[0x%lx], it needs to be a 'int', and it should be in the range of 0~6.", (unsigned long)value);
                return IM_STATUS_ILLEGAL_PARAM;
            }
            break;
        case IM_CONFIG_CHECK :
            if (value == false || value == true) {
                g_im2d_context.check_mode = (bool)value;
            } else {
                IM_LOGE("IM2D: It's not legal check config[0x%lx], it needs to be a 'bool'.", (unsigned long)value);
                return IM_STATUS_ILLEGAL_PARAM;
            }
            break;
        default :
            IM_LOGE("IM2D: Unsupported config name!");
            return IM_STATUS_NOT_SUPPORTED;
    }

    return IM_STATUS_SUCCESS;
}

/* Start single task api */
IM_API IM_STATUS imcopy(const rga_buffer_t src, rga_buffer_t dst, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("imcopy cannot support scale, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imresize(const rga_buffer_t src, rga_buffer_t dst, double fx, double fy, int interpolation, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if (fx > 0 || fy > 0) {
        if (fx == 0) fx = 1;
        if (fy == 0) fy = 1;

        dst.width = (int)(src.width * fx);
        dst.height = (int)(src.height * fy);

        int format = convert_to_rga_format(dst.format);
        if (format == RK_FORMAT_UNKNOWN) {
            IM_LOGE("Invaild dst format [0x%x]!\n", dst.format);
            return IM_STATUS_NOT_SUPPORTED;
        }

        if(is_yuv_format(format)) {
            int width = dst.width;
            int height = dst.height;
            dst.width = DOWN_ALIGN(dst.width, 2);
            dst.height = DOWN_ALIGN(dst.height, 2);

            ret = imcheck(src, dst, srect, drect, usage);
            if (ret != IM_STATUS_NOERROR) {
                IM_LOGE("imresize error, factor[fx,fy]=[%lf,%lf], ALIGN[dw,dh]=[%d,%d][%d,%d]", fx, fy, width, height, dst.width, dst.height);
                return ret;
            }
        }
    }

    opt.version = RGA_CURRENT_API_VERSION;
    opt.interp = interpolation;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imcvtcolor(rga_buffer_t src, rga_buffer_t dst, int sfmt, int dfmt, int mode, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    src.format = sfmt;
    dst.format = dfmt;

    if (dst.color_space_mode == 0)
        dst.color_space_mode = mode;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imcrop(const rga_buffer_t src, rga_buffer_t dst, im_rect rect, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, NULL, &drect, &prect, &opt);

    drect.width = rect.width;
    drect.height = rect.height;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, rect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imtranslate(const rga_buffer_t src, rga_buffer_t dst, int x, int y, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("The width and height of src and dst need to be equal, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    srect.width = src.width - x;
    srect.height = src.height - y;
    drect.x = x;
    drect.y = y;
    drect.width = src.width - x;
    drect.height = src.height - y;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imrotate(const rga_buffer_t src, rga_buffer_t dst, int rotation, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= rotation;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imflip(const rga_buffer_t src, rga_buffer_t dst, int mode, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= mode;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imcomposite(const rga_buffer_t srcA, const rga_buffer_t srcB, rga_buffer_t dst, int mode, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, NULL, &srect, &drect, &prect, &opt);

    usage |= mode;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(srcA, dst, srcB, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imblend(const rga_buffer_t src, rga_buffer_t dst, int mode, int sync, int *release_fence_fd) {
    rga_buffer_t pat;

    memset(&pat, 0x0, sizeof(pat));

    return imcomposite(src, pat, dst, mode, sync, release_fence_fd);
}

IM_API IM_STATUS imosd(const rga_buffer_t osd,const rga_buffer_t dst,
                       const im_rect osd_rect, im_osd_t *osd_info,
                       int sync, int *release_fence_fd) {
    int usage = 0;
    im_opt_t opt;
    im_rect tmp_rect;

    memset(&opt, 0x0, sizeof(opt));
    memset(&tmp_rect, 0x0, sizeof(tmp_rect));

    opt.version = RGA_CURRENT_API_VERSION;
    memcpy(&opt.osd_config, osd_info, sizeof(im_osd_t));

    usage |= IM_ALPHA_BLEND_DST_OVER | IM_OSD;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    return rga_single_task_submit(dst, dst, osd, osd_rect, osd_rect, tmp_rect, -1, release_fence_fd, &opt, usage);
}

IM_API IM_STATUS imcolorkey(const rga_buffer_t src, rga_buffer_t dst, im_colorkey_range range, int mode, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= mode;

    opt.colorkey_range = range;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imquantize(const rga_buffer_t src, rga_buffer_t dst, im_nn_t nn_info, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= IM_NN_QUANTIZE;

    opt.nn = nn_info;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imrop(const rga_buffer_t src, rga_buffer_t dst, int rop_code, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= IM_ROP;

    opt.rop_code = rop_code;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS immosaic(const rga_buffer_t image, im_rect rect, int mosaic_mode, int sync, int *release_fence_fd) {
    IM_STATUS ret = IM_STATUS_NOERROR;
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t tmp_image;
    im_rect tmp_rect;

    memset(&opt, 0x0, sizeof(opt));
    memset(&tmp_image, 0x0, sizeof(tmp_image));
    memset(&tmp_rect, 0x0, sizeof(tmp_rect));

    usage |= IM_MOSAIC;

    opt.version = RGA_CURRENT_API_VERSION;
    opt.mosaic_mode = mosaic_mode;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    return rga_single_task_submit(image, image, tmp_image, rect, rect, tmp_rect, -1, release_fence_fd, &opt, usage);
}

IM_STATUS immosaicArray(rga_buffer_t dst, im_rect *rect_array, int array_size, int mosaic_mode, int sync, int *release_fence_fd) {
    IM_STATUS ret;
    int tmp_fence_fd = -1;

    for (int i = 0; i < array_size; i++) {
        ret = immosaic(dst, rect_array[i], mosaic_mode, sync, release_fence_fd);
        if (ret != IM_STATUS_SUCCESS)
            return ret;

        if (sync == 0 && release_fence_fd) {
            if (*release_fence_fd >= 0 && tmp_fence_fd >= 0) {
                tmp_fence_fd = rga_sync_merge("mosaic_array", tmp_fence_fd, *release_fence_fd);
            } else if (*release_fence_fd >= 0) {
                tmp_fence_fd = *release_fence_fd;
            }
        }
    }

    if (release_fence_fd)
        *release_fence_fd = tmp_fence_fd;

    return IM_STATUS_SUCCESS;
}

IM_API IM_STATUS imgaussianBlur(rga_buffer_t src, rga_buffer_t dst,
                                int gauss_width, int gauss_height,
                                int sigma_x, int sigma_y, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    im_opt_t opt;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    /* Scaling is not supported. */
    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("The width and height of src and dst need to be equal, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    usage |= IM_GAUSS;

    imsetOptGaussianBlur(&opt, gauss_width, gauss_height, sigma_x, sigma_y);

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    return rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, release_fence_fd, &opt, usage);
}

IM_API IM_STATUS impalette(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t lut, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    im_opt_t opt;

    empty_structure(NULL, NULL, NULL, &srect, &drect, &prect, &opt);

    /*Don't know if it supports zooming.*/
    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("The width and height of src and dst need to be equal, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    usage |= IM_COLOR_PALETTE;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, lut, srect, drect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_API IM_STATUS imfill(rga_buffer_t dst, im_rect rect, int color, int sync, int *release_fence_fd) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;

    im_opt_t opt;

    rga_buffer_t pat;
    rga_buffer_t src;

    im_rect srect;
    im_rect prect;

    empty_structure(&src, NULL, &pat, &srect, NULL, &prect, &opt);

    memset(&src, 0, sizeof(rga_buffer_t));

    usage |= IM_COLOR_FILL;

    opt.color = color;

    if (sync == 0)
        usage |= IM_ASYNC;
    else if (sync == 1)
        usage |= IM_SYNC;

    ret = rga_single_task_submit(src, dst, pat, srect, rect, prect, -1, release_fence_fd, &opt, usage);

    return ret;
}

IM_STATUS imfillArray(rga_buffer_t dst, im_rect *rect_array, int array_size, uint32_t color, int sync, int *release_fence_fd) {
    IM_STATUS ret;
    int tmp_fence_fd = -1;

    for (int i = 0; i < array_size; i++) {
        ret = imfill(dst, rect_array[i], color, sync, release_fence_fd);
        if (ret != IM_STATUS_SUCCESS)
            return ret;

        if (sync == 0 && release_fence_fd) {
            if (*release_fence_fd >= 0 && tmp_fence_fd >= 0) {
                tmp_fence_fd = rga_sync_merge("fill_array", tmp_fence_fd, *release_fence_fd);
            } else if (*release_fence_fd >= 0) {
                tmp_fence_fd = *release_fence_fd;
            }
        }
    }

    if (release_fence_fd)
        *release_fence_fd = tmp_fence_fd;

    return IM_STATUS_SUCCESS;
}

IM_STATUS imrectangle(rga_buffer_t dst, im_rect rect, uint32_t color, int thickness, int sync, int *release_fence_fd) {
    if (thickness < 0)
        return imfill(dst, rect, color, sync, release_fence_fd);

    int h_length = rect.width;
    int v_length = rect.height - 2 * thickness;
    im_rect fill_rect[4] = {};

    fill_rect[0] = (im_rect){rect.x, rect.y, h_length, thickness};
    fill_rect[1] = (im_rect){rect.x, rect.y + (rect.height - thickness), h_length, thickness};
    fill_rect[2] = (im_rect){rect.x, rect.y + thickness, thickness, v_length};
    fill_rect[3] = (im_rect){rect.x + (rect.width - thickness), rect.y + thickness, thickness, v_length};

    return imfillArray(dst, fill_rect, 4, color, sync, release_fence_fd);
}

IM_STATUS imrectangleArray(rga_buffer_t dst, im_rect *rect_array, int array_size, uint32_t color, int thickness, int sync, int *release_fence_fd) {
    IM_STATUS ret;
    int tmp_fence_fd = -1;

    for (int i = 0; i < array_size; i++) {
        ret = imrectangle(dst, rect_array[i], color, thickness, sync, release_fence_fd);
        if (ret != IM_STATUS_SUCCESS)
            return ret;

        if (sync == 0 && release_fence_fd) {
            if (*release_fence_fd >= 0 && tmp_fence_fd >= 0) {
                tmp_fence_fd = rga_sync_merge("rectangle_array", tmp_fence_fd, *release_fence_fd);
            } else if (*release_fence_fd >= 0) {
                tmp_fence_fd = *release_fence_fd;
            }
        }
    }

    if (release_fence_fd)
        *release_fence_fd = tmp_fence_fd;

    return IM_STATUS_SUCCESS;
}

IM_API IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                        im_rect srect, im_rect drect, im_rect prect, int usage) {
    return rga_single_task_submit(src, dst, pat, srect, drect, prect, -1, NULL, NULL, usage);
}

#ifdef __cplusplus
IM_API IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                           im_rect srect, im_rect drect, im_rect prect,
                           int acquire_fence_fd, int *release_fence_fd, im_opt_t *opt_ptr, int usage) {
    return rga_single_task_submit(src, dst, pat, srect, drect, prect,
                                  acquire_fence_fd, release_fence_fd,
                                  opt_ptr, usage);
}
/* End single task api */

IM_STATUS immakeBorder(rga_buffer_t src, rga_buffer_t dst,
                       int top, int bottom, int left, int right,
                       int border_type, int value,
                       int sync, int acquir_fence_fd, int *release_fence_fd) {
    IM_STATUS ret;
    im_job_handle_t job_handle;
    im_rect border_rect[4], border_src_rect[4];
    bool reflect;
    int copy_fence_fd = -1, tmp_fence_fd = -1;

    if (sync != 1 && release_fence_fd == NULL) {
        IM_LOGE("async mode, address of release_fence_fd cannot be NULL!\n");
        return IM_STATUS_INVALID_PARAM;
    }

    if (src.width + left + right != dst.width ||
        src.height + top + bottom != dst.height) {
        IM_LOGW("The width/height of dst must be equal to the width/height after making the border!"
                "src[w,h] = [%d, %d], dst[w,h] = [%d, %d], [t,b,l,r] = [%d, %d, %d, %d]\n",
                src.width, src.height, dst.width, dst.height, top, bottom, left, right);
        return IM_STATUS_ILLEGAL_PARAM;
    }

    ret = improcess(src, dst, {}, {}, {left, top, src.width, src.height}, {}, acquir_fence_fd, &copy_fence_fd, NULL, IM_ASYNC);
    if (ret != IM_STATUS_SUCCESS || copy_fence_fd <= 0)
        return ret;

    if (top == 0 && bottom == 0 && left == 0 && right == 0) {
        if (sync == 1) {
            ret = imsync(copy_fence_fd);
            if (ret != IM_STATUS_SUCCESS)
                return ret;
        } else {
            *release_fence_fd = copy_fence_fd;
            return IM_STATUS_SUCCESS;
        }
    }

    job_handle = imbeginJob();
    if (job_handle <= 0)
        return IM_STATUS_FAILED;

    /* top */
    border_rect[0] = {left, 0, src.width, top};
    /* bottom */
    border_rect[1] = {left, src.height + top, border_rect[0].width, bottom};
    /* left */
    border_rect[2] = {0, 0, left, src.height + top + bottom};
    /* right */
    border_rect[3] = {src.width + left, 0, right, border_rect[2].height};

    if (border_type == IM_BORDER_CONSTANT) {
        if (top) {
            ret = imfillTask(job_handle, dst, border_rect[0], value);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;
        }

        if (bottom) {
            ret = imfillTask(job_handle, dst, border_rect[1], value);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;
        }

        if (left) {
            ret = imfillTask(job_handle, dst, border_rect[2], value);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;
        }

        if (right) {
            ret = imfillTask(job_handle, dst, border_rect[3], value);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;
        }
    } else {
        switch (border_type) {
            case IM_BORDER_REFLECT:
                /* top */
                border_src_rect[0] = {0, 0, src.width, top};
                /* bottom */
                border_src_rect[1] = {0, src.height - bottom, src.width, bottom};
                /* left */
                border_src_rect[2] = {left, 0, left, border_rect[2].height};
                /* right */
                border_src_rect[3] = {border_rect[3].x - right, 0, right, border_rect[3].height};

                reflect = true;
                break;
            case IM_BORDER_WRAP:
                /* top */
                border_src_rect[0] = {0, src.height - top, src.width, top};
                /* bottom */
                border_src_rect[1] = {0, 0, src.width, bottom};
                /* left */
                border_src_rect[2] = {border_rect[3].x - left, 0, left, border_rect[3].height};
                /* right */
                border_src_rect[3] = {left, 0, right, border_rect[2].height};
                reflect = false;
                break;
            default:
                IM_LOGW("unknown border type 0x%x\n", border_type);
                return imcancelJob(job_handle);
        }

        if (top || bottom) {
            /* top */
            if (top) {
                ret = improcessTask(job_handle, src, dst, {}, border_src_rect[0], border_rect[0], {}, NULL, reflect ? IM_HAL_TRANSFORM_FLIP_V : 0);
                if (ret != IM_STATUS_SUCCESS)
                    goto cancel_job_handle;
            }

            /* bottom */
            if (bottom) {
                ret = improcessTask(job_handle, src, dst, {}, border_src_rect[1], border_rect[1], {}, NULL, reflect ? IM_HAL_TRANSFORM_FLIP_V : 0);
                if (ret != IM_STATUS_SUCCESS)
                    goto cancel_job_handle;
            }

            ret = imendJob(job_handle, IM_ASYNC, copy_fence_fd, &tmp_fence_fd);
            if (ret != IM_STATUS_SUCCESS || tmp_fence_fd <= 0)
                goto cancel_job_handle;

            copy_fence_fd = tmp_fence_fd;

            job_handle = 0;
            job_handle = imbeginJob();
            if (job_handle <= 0)
                return IM_STATUS_FAILED;
        }

        /* left */
        if (left || right) {
            ret = improcessTask(job_handle, dst, dst, {}, border_src_rect[2], border_rect[2], {}, NULL, reflect ? IM_HAL_TRANSFORM_FLIP_H : 0);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;

            /* right */
            ret = improcessTask(job_handle, dst, dst, {}, border_src_rect[3], border_rect[3], {}, NULL, reflect ? IM_HAL_TRANSFORM_FLIP_H : 0);
            if (ret != IM_STATUS_SUCCESS)
                goto cancel_job_handle;
        }
    }

    if (sync == 1) {
        ret = imsync(copy_fence_fd);
        if (ret != IM_STATUS_SUCCESS)
            goto cancel_job_handle;

        return imendJob(job_handle);
    } else {
        return imendJob(job_handle, IM_ASYNC, copy_fence_fd, release_fence_fd);
    }

cancel_job_handle:
    imcancelJob(job_handle);
    return ret;
}

/* Start task api */
IM_API im_job_handle_t imbeginJob(uint64_t flags) {
    return rga_job_create(flags);
}

IM_API IM_STATUS imcancelJob(im_job_handle_t job_handle) {
    return rga_job_cancel(job_handle);
}

IM_API IM_STATUS imendJob(im_job_handle_t job_handle, int sync_mode, int acquire_fence_fd, int *release_fence_fd) {
    return rga_job_submit(job_handle, sync_mode, acquire_fence_fd, release_fence_fd);
}

IM_API IM_STATUS imcopyTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("imcopy cannot support scale, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imresizeTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, double fx, double fy, int interpolation) {
    int usage = 0;
    IM_STATUS ret = IM_STATUS_NOERROR;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if (fx > 0 || fy > 0) {
        if (fx == 0) fx = 1;
        if (fy == 0) fy = 1;

        dst.width = (int)(src.width * fx);
        dst.height = (int)(src.height * fy);

        int format = convert_to_rga_format(dst.format);
        if (format == RK_FORMAT_UNKNOWN) {
            IM_LOGE("Invaild dst format [0x%x]!\n", dst.format);
            return IM_STATUS_NOT_SUPPORTED;
        }

        if(is_yuv_format(format)) {
            int width = dst.width;
            int height = dst.height;
            dst.width = DOWN_ALIGN(dst.width, 2);
            dst.height = DOWN_ALIGN(dst.height, 2);

            ret = imcheck(src, dst, srect, drect, usage);
            if (ret != IM_STATUS_NOERROR) {
                IM_LOGE("imresize error, factor[fx,fy]=[%lf,%lf], ALIGN[dw,dh]=[%d,%d][%d,%d]", fx, fy, width, height, dst.width, dst.height);
                return ret;
            }
        }
    }

    opt.version = RGA_CURRENT_API_VERSION;
    opt.interp = interpolation;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imcropTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, im_rect rect) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, NULL, &drect, &prect, &opt);

    drect.width = rect.width;
    drect.height = rect.height;

    return improcessTask(job_handle, src, dst, pat, rect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imcvtcolorTask(im_job_handle_t job_handle, rga_buffer_t src, rga_buffer_t dst, int sfmt, int dfmt, int mode) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    src.format = sfmt;
    dst.format = dfmt;

    dst.color_space_mode = mode;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imtranslateTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, int x, int y) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("The width and height of src and dst need to be equal, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    srect.width = src.width - x;
    srect.height = src.height - y;
    drect.x = x;
    drect.y = y;
    drect.width = src.width - x;
    drect.height = src.height - y;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imrotateTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, int rotation) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;

    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= rotation;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imflipTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, int mode) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= mode;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imcompositeTask(im_job_handle_t job_handle, const rga_buffer_t srcA, const rga_buffer_t srcB, rga_buffer_t dst, int mode) {
    int usage = 0;
    im_opt_t opt;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, NULL, &srect, &drect, &prect, &opt);

    usage |= mode;

    return improcessTask(job_handle, srcA, dst, srcB, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imblendTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, int mode) {
    rga_buffer_t pat;

    memset(&pat, 0x0, sizeof(pat));

    return imcompositeTask(job_handle, src, pat, dst, mode);
}

IM_API IM_STATUS imcolorkeyTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, im_colorkey_range range, int mode) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= mode;

    opt.colorkey_range = range;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imosdTask(im_job_handle_t job_handle,
                           const rga_buffer_t osd,const rga_buffer_t bg_image,
                           const im_rect osd_rect, im_osd_t *osd_config) {
    int usage = 0;
    im_opt_t opt;
    im_rect tmp_rect;

    memset(&opt, 0x0, sizeof(opt));
    memset(&tmp_rect, 0x0, sizeof(tmp_rect));

    opt.version = RGA_CURRENT_API_VERSION;
    memcpy(&opt.osd_config, osd_config, sizeof(im_osd_t));

    usage |= IM_ALPHA_BLEND_DST_OVER | IM_OSD;

    return improcessTask(job_handle, bg_image, bg_image, osd, osd_rect, osd_rect, tmp_rect, &opt, usage);
}

IM_API IM_STATUS imquantizeTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, im_nn_t nn_info) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= IM_NN_QUANTIZE;

    opt.nn = nn_info;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS imropTask(im_job_handle_t job_handle, const rga_buffer_t src, rga_buffer_t dst, int rop_code) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    empty_structure(NULL, NULL, &pat, &srect, &drect, &prect, &opt);

    usage |= IM_ROP;

    opt.rop_code = rop_code;

    return improcessTask(job_handle, src, dst, pat, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS immosaicTask(im_job_handle_t job_handle, const rga_buffer_t image, im_rect rect, int mosaic_mode) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t tmp_image;
    im_rect tmp_rect;

    memset(&opt, 0x0, sizeof(opt));
    memset(&tmp_image, 0x0, sizeof(tmp_image));
    memset(&tmp_rect, 0x0, sizeof(tmp_rect));

    usage |= IM_MOSAIC;

    opt.version = RGA_CURRENT_API_VERSION;
    opt.mosaic_mode = mosaic_mode;

    return improcessTask(job_handle, image, image, tmp_image, rect, rect, tmp_rect, &opt, usage);
}

IM_API IM_STATUS immosaicTaskArray(im_job_handle_t job_handle, rga_buffer_t dst, im_rect *rect_array, int array_size, int color) {
    IM_STATUS ret;

    for (int i = 0; i < array_size; i++) {
        ret = immosaicTask(job_handle, dst, rect_array[i], color);
        if (ret != IM_STATUS_SUCCESS)
            return ret;
    }

    return IM_STATUS_SUCCESS;
}

IM_API IM_STATUS imfillTask(im_job_handle_t job_handle, rga_buffer_t dst, im_rect rect, uint32_t color) {
    int usage = 0;
    im_opt_t opt;
    rga_buffer_t pat;
    rga_buffer_t src;
    im_rect srect;
    im_rect prect;

    empty_structure(&src, NULL, &pat, &srect, NULL, &prect, &opt);

    memset(&src, 0, sizeof(rga_buffer_t));

    usage |= IM_COLOR_FILL;

    opt.color = color;

    return improcessTask(job_handle, src, dst, pat, srect, rect, prect, &opt, usage);
}

IM_API IM_STATUS imfillTaskArray(im_job_handle_t job_handle, rga_buffer_t dst, im_rect *rect_array, int array_size, uint32_t color) {
    IM_STATUS ret;

    for (int i = 0; i < array_size; i++) {
        ret = imfillTask(job_handle, dst, rect_array[i], color);
        if (ret != IM_STATUS_SUCCESS)
            return ret;
    }

    return IM_STATUS_SUCCESS;
}

IM_STATUS imrectangleTask(im_job_handle_t job_handle, rga_buffer_t dst, im_rect rect, uint32_t color, int thickness) {
    if (thickness < 0)
        return imfillTask(job_handle, dst, rect, color);

    int h_length = rect.width;
    int v_length = rect.height - 2 * thickness;
    im_rect fill_rect[4] = {};

    fill_rect[0] = {rect.x, rect.y, h_length, thickness};
    fill_rect[1] = {rect.x, rect.y + (rect.height - thickness), h_length, thickness};
    fill_rect[2] = {rect.x, rect.y + thickness, thickness, v_length};
    fill_rect[3] = {rect.x + (rect.width - thickness), rect.y + thickness, thickness, v_length};

    return imfillTaskArray(job_handle, dst, fill_rect, 4, color);
}

IM_STATUS imrectangleTaskArray(im_job_handle_t job_handle, rga_buffer_t dst, im_rect *rect_array, int array_size, uint32_t color, int thickness) {
    IM_STATUS ret;

    for (int i = 0; i < array_size; i++) {
        ret = imrectangleTask(job_handle, dst, rect_array[i], color, thickness);
        if (ret != IM_STATUS_SUCCESS)
            return ret;
    }

    return IM_STATUS_SUCCESS;
}

IM_API IM_STATUS impaletteTask(im_job_handle_t job_handle, rga_buffer_t src, rga_buffer_t dst, rga_buffer_t lut) {
    int usage = 0;
    im_rect srect;
    im_rect drect;
    im_rect prect;

    im_opt_t opt;

    empty_structure(NULL, NULL, NULL, &srect, &drect, &prect, &opt);

    /*Don't know if it supports zooming.*/
    if ((src.width != dst.width) || (src.height != dst.height)) {
        IM_LOGW("The width and height of src and dst need to be equal, src[w,h] = [%d, %d], dst[w,h] = [%d, %d]",
                src.width, src.height, dst.width, dst.height);
        return IM_STATUS_INVALID_PARAM;
    }

    usage |= IM_COLOR_PALETTE;

    return improcessTask(job_handle, src, dst, lut, srect, drect, prect, &opt, usage);
}

IM_API IM_STATUS improcessTask(im_job_handle_t job_handle,
                               rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                               im_rect srect, im_rect drect, im_rect prect,
                               im_opt_t *opt_ptr, int usage) {
    return rga_task_submit(job_handle, src, dst, pat, srect, drect, prect, -1, NULL, opt_ptr, usage);
}
/* End task api */
#endif /* #ifdef __cplusplus */

/* for rockit-ko */
im_ctx_id_t imbegin(uint32_t flags) {
    return rga_job_create(flags);
}

IM_STATUS imcancel(im_ctx_id_t id) {
    return rga_job_cancel((im_job_handle_t)id);
}

IM_STATUS improcess_ctx(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                        im_rect srect, im_rect drect, im_rect prect,
                        int acquire_fence_fd, int *release_fence_fd,
                        im_opt_t *opt_ptr, int usage, im_ctx_id_t ctx_id) {
    int ret;
    int sync_mode;

    UNUSED(acquire_fence_fd);
    UNUSED(release_fence_fd);

    ret = rga_task_submit((im_job_handle_t)ctx_id, src, dst, pat, srect, drect, prect, -1, NULL, opt_ptr, usage);
    if (ret != IM_STATUS_SUCCESS)
        return (IM_STATUS)ret;

    if (usage & IM_ASYNC)
        sync_mode = IM_ASYNC;
    else
        sync_mode = IM_SYNC;

    return rga_job_config((im_job_handle_t)ctx_id, sync_mode, acquire_fence_fd, release_fence_fd);
}

#ifdef __cplusplus
IM_STATUS improcess(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                    im_rect srect, im_rect drect, im_rect prect,
                    int acquire_fence_fd, int *release_fence_fd,
                    im_opt_t *opt_ptr, int usage, im_ctx_id_t ctx_id) {
    return improcess_ctx(src, dst, pat, srect, drect, prect, acquire_fence_fd, release_fence_fd, opt_ptr, usage, ctx_id);
}
#endif /* #ifdef __cplusplus */

/* For the C interface */
IM_API rga_buffer_t wrapbuffer_handle_t(rga_buffer_handle_t  handle,
                                        int width, int height,
                                        int wstride, int hstride,
                                        int format) {
    return wrapbuffer_handle(handle, width, height, format, wstride, hstride);
}

IM_API rga_buffer_t wrapbuffer_virtualaddr_t(void* vir_addr,
                                             int width, int height,
                                             int wstride, int hstride,
                                             int format) {
    return wrapbuffer_virtualaddr(vir_addr, width, height, format, wstride, hstride);
}

IM_API rga_buffer_t wrapbuffer_physicaladdr_t(void* phy_addr,
                                              int width, int height,
                                              int wstride, int hstride,
                                              int format) {
    return wrapbuffer_physicaladdr(phy_addr, width, height, format, wstride, hstride);
}

IM_API rga_buffer_t wrapbuffer_fd_t(int fd,
                                    int width, int height,
                                    int wstride, int hstride,
                                    int format) {
    return wrapbuffer_fd(fd, width, height, format, wstride, hstride);
}

IM_API IM_STATUS imcheck_t(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t pat,
                           im_rect src_rect, im_rect dst_rect, im_rect pat_rect, int mode_usage) {
    return rga_check_external(src, dst, pat, src_rect, dst_rect, pat_rect, mode_usage);
}

IM_API IM_STATUS imresize_t(const rga_buffer_t src, rga_buffer_t dst, double fx, double fy, int interpolation, int sync) {
    return imresize(src, dst, fx, fy, interpolation, sync, NULL);
}

IM_API IM_STATUS imcrop_t(const rga_buffer_t src, rga_buffer_t dst, im_rect rect, int sync) {
    return imcrop(src, dst, rect, sync, NULL);
}

IM_API IM_STATUS imrotate_t(const rga_buffer_t src, rga_buffer_t dst, int rotation, int sync) {
    return imrotate(src, dst, rotation, sync, NULL);
}

IM_API IM_STATUS imflip_t (const rga_buffer_t src, rga_buffer_t dst, int mode, int sync) {
    return imflip(src, dst, mode, sync, NULL);
}

IM_API IM_STATUS imfill_t(rga_buffer_t dst, im_rect rect, int color, int sync) {
    return imfill(dst, rect, color, sync, NULL);
}

IM_API IM_STATUS impalette_t(rga_buffer_t src, rga_buffer_t dst, rga_buffer_t lut, int sync) {
    return impalette(src, dst, lut, sync, NULL);
}

IM_API IM_STATUS imtranslate_t(const rga_buffer_t src, rga_buffer_t dst, int x, int y, int sync) {
    return imtranslate(src, dst, x, y, sync, NULL);
}

IM_API IM_STATUS imcopy_t(const rga_buffer_t src, rga_buffer_t dst, int sync) {
    return imcopy(src, dst, sync, NULL);
}

IM_API IM_STATUS imcolorkey_t(const rga_buffer_t src, rga_buffer_t dst, im_colorkey_range range, int mode, int sync) {
    return imcolorkey(src, dst, range, mode, sync, NULL);
}

IM_API IM_STATUS imblend_t(const rga_buffer_t srcA, const rga_buffer_t srcB, rga_buffer_t dst, int mode, int sync) {
    return imcomposite(srcA, srcB, dst, mode, sync, NULL);
}

IM_API IM_STATUS imcvtcolor_t(rga_buffer_t src, rga_buffer_t dst, int sfmt, int dfmt, int mode, int sync) {
    return imcvtcolor(src, dst, sfmt, dfmt, mode, sync, NULL);
}

IM_API IM_STATUS imquantize_t(const rga_buffer_t src, rga_buffer_t dst, im_nn_t nn_info, int sync) {
    return imquantize(src, dst, nn_info, sync, NULL);
}

IM_API IM_STATUS imrop_t(const rga_buffer_t src, rga_buffer_t dst, int rop_code, int sync) {
    return imrop(src, dst, rop_code, sync, NULL);
}

IM_API IM_STATUS imgaussianBlur_t(rga_buffer_t src, rga_buffer_t dst,
                                  int gauss_width, int gauss_height,
                                  int sigma_x, int sigma_y, int sync) {
    return imgaussianBlur(src, dst, gauss_width, gauss_height, sigma_x, sigma_y, sync, NULL);
}

#ifdef __cplusplus
IM_API IM_STATUS immosaic(const rga_buffer_t image, im_rect rect, int mosaic_mode, int sync) {
    return immosaic(image, rect, mosaic_mode, sync, NULL);
}

IM_API IM_STATUS imosd(const rga_buffer_t osd,const rga_buffer_t dst, const im_rect osd_rect,
                       im_osd_t *osd_info, int sync) {
    return imosd(osd, dst, osd_rect, osd_info, sync, NULL);
}
#endif
