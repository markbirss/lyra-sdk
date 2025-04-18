/* Copyright 2020 Rockchip Electronics Co. LTD
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
 *
 * author: rimon.xu@rock-chips.com
 *   date: 2020-11-06
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "rk_debug.h"
#include "rk_mpi_mb.h"
#include "rk_comm_tde.h"
#include "rk_comm_video.h"
#include "rk_mpi_tde.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_cal.h"
#include "test_comm_argparse.h"
#include "test_comm_utils.h"
#include "test_comm_tde.h"
#include "rk_osal.h"

typedef struct _rkTDEOpMap {
    RK_S32        op;
    const char*   strOp;
} TDE_OP_MAP_S;

const static TDE_OP_MAP_S gstTdeOpMaps[] = {
    { TDE_OP_QUICK_COPY,        "quick_copy" },
    { TDE_OP_QUICK_RESIZE,      "quick_resize" },
    { TDE_OP_QUICK_FILL,        "quick_fill" },
    { TDE_OP_ROTATION,          "rotation" },
    { TDE_OP_MIRROR,            "mirror" },
    { TDE_OP_COLOR_KEY,         "color_key" },
    { TDE_OP_COLOR_BLEND,       "blend" }
};

typedef struct _rkMpiTdeCtx {
    const char     *srcLoadFilePath;
    const char     *dstLoadFilePath;
    const char     *dstFilePath;
    const char     *backgroundFile;
    RK_S32          s32LoopCount;
    RK_S32          s32JobNum;
    RK_S32          s32TaskNum;
    RK_S32          s32Rotation;
    RK_S32          s32Operation;
    TDE_SURFACE_S   stSrcSurface;
    TDE_SURFACE_S   stDstSurface;
    RECT_S          stSrcRect;
    RECT_S          stDstRect;
    RK_U32          u32SrcVirWidth;
    RK_U32          u32SrcVirHeight;
    RK_U32          u32DstVirWidth;
    RK_U32          u32DstVirHeight;
    RK_S32          s32SrcCompressMode;
    RK_S32          s32DstCompressMode;
    RK_S32          s32Color;
    RK_S32          s32ColorKey;
    RK_S32          s32Mirror;
    RK_S32          s32ProcessTime;
    RK_U32          u32BlendCmd;
    RK_BOOL         bPerformace;
} TEST_TDE_CTX_S;

static const char *test_tde_str_op(RK_S32 op) {
    RK_S32 s32ElemLen = sizeof(gstTdeOpMaps) / sizeof(gstTdeOpMaps[0]);
    for (size_t i = 0; i < s32ElemLen; i++) {
        if (op == gstTdeOpMaps[i].op) {
            return gstTdeOpMaps[i].strOp;
        }
    }
    return RK_NULL;
}

RK_S32 test_tde_save_result(TEST_TDE_CTX_S *ctx, TDE_SURFACE_S  *pstDst, RK_S32 u32TaskId) {
    char yuv_out_path[1024] = {0};
    RK_S32 s32Ret = RK_SUCCESS;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    MB_BLK dstBlk = pstDst->pMbBlk;
    RK_VOID *pstFrame = RK_MPI_MB_Handle2VirAddr(dstBlk);
    stPicBufAttr.u32Width = ctx->stDstSurface.u32Width;
    stPicBufAttr.u32Height = ctx->stDstSurface.u32Height;
    stPicBufAttr.enPixelFormat = ctx->stDstSurface.enColorFmt;
    stPicBufAttr.enCompMode = (COMPRESS_MODE_E)(ctx->s32DstCompressMode);

    s32Ret = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("get picture buffer size failed. err 0x%x", s32Ret);
        return s32Ret;
    }

    snprintf(yuv_out_path, sizeof(yuv_out_path), "%s%s_%dx%d_%d.bin",
        ctx->dstFilePath, test_tde_str_op(ctx->s32Operation),
        stPicBufAttr.u32Width, stPicBufAttr.u32Height, u32TaskId);

    FILE *file = fopen(yuv_out_path, "wb+");
    if (file == RK_NULL) {
        RK_LOGE("open path %s failed because %s.", yuv_out_path, strerror(errno));
        return RK_FAILURE;
    }

    if (pstFrame) {
        RK_LOGI("get frame:%p, size:%d, bBlk:%p", pstFrame, stMbPicCalResult.u32MBSize, dstBlk);
        RK_MPI_SYS_MmzFlushCache(dstBlk, RK_TRUE);
        fwrite(pstFrame, 1, stMbPicCalResult.u32MBSize, file);
        fflush(file);
    }
    fclose(file);
    file = NULL;

    return RK_SUCCESS;
}

RK_S32 unit_test_tde_get_src_size(TEST_TDE_CTX_S *ctx, RK_U32 *u32SrcSize) {
    RK_S32 s32Ret = RK_SUCCESS;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    stPicBufAttr.u32Width = ctx->stSrcSurface.u32Width;
    stPicBufAttr.u32Height = ctx->stSrcSurface.u32Height;
    stPicBufAttr.enPixelFormat = (PIXEL_FORMAT_E)ctx->stSrcSurface.enColorFmt;
    stPicBufAttr.enCompMode = (COMPRESS_MODE_E)ctx->s32SrcCompressMode;
    s32Ret = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("get picture buffer size failed. err 0x%x", s32Ret);
        return s32Ret;
    }
    *u32SrcSize = stMbPicCalResult.u32MBSize;
    return s32Ret;
}

RK_S32 unit_test_tde_get_dst_size(TEST_TDE_CTX_S *ctx, RK_U32 *u32DstSize) {
    RK_S32 s32Ret = RK_SUCCESS;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    stPicBufAttr.u32Width = ctx->stDstSurface.u32Width;
    stPicBufAttr.u32Height = ctx->stDstSurface.u32Height;
    stPicBufAttr.enPixelFormat = (PIXEL_FORMAT_E)ctx->stDstSurface.enColorFmt;
    stPicBufAttr.enCompMode = (COMPRESS_MODE_E)ctx->s32DstCompressMode;
    s32Ret = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("get picture buffer size failed. err 0x%x", s32Ret);
        return s32Ret;
    }
    *u32DstSize = stMbPicCalResult.u32MBSize;
    return s32Ret;
}

RK_S32 test_tde_read_file(const char *path,  void *pu8Data, RK_U32 u32ImgSize) {
    FILE *pFile = RK_NULL;
    RK_S32 s32Ret = RK_SUCCESS;
    RK_U32 u32ReadSize = 0;

    pFile = fopen(path, "rb+");
    if (pFile == RK_NULL) {
        RK_LOGE("open path %s failed because %s.", path, strerror(errno));
        return RK_FAILURE;
    }
    if (pFile) {
        u32ReadSize = fread(pu8Data, 1, u32ImgSize, pFile);
        fflush(pFile);
        fclose(pFile);
    }
    return s32Ret;
}

RK_S32 test_tde_fill_src(
        TEST_TDE_CTX_S *ctx, TDE_SURFACE_S *pstSrcSurface, TDE_RECT_S *pstSrcRect) {
    pstSrcSurface->u32Width         = ctx->stSrcSurface.u32Width;
    pstSrcSurface->u32Height        = ctx->stSrcSurface.u32Height;
    pstSrcSurface->enColorFmt       = ctx->stSrcSurface.enColorFmt;
    pstSrcSurface->enComprocessMode = ctx->stSrcSurface.enComprocessMode;
    pstSrcRect->s32Xpos             = ctx->stSrcRect.s32X;
    pstSrcRect->s32Ypos             = ctx->stSrcRect.s32Y;
    pstSrcRect->u32Width            = ctx->stSrcRect.u32Width;
    pstSrcRect->u32Height           = ctx->stSrcRect.u32Height;
    return RK_SUCCESS;
}

RK_S32 test_tde_fill_dst(
        TEST_TDE_CTX_S *ctx, TDE_SURFACE_S *pstDstSurface, TDE_RECT_S *pstDstRect) {
    pstDstSurface->u32Width         = ctx->stDstSurface.u32Width;
    pstDstSurface->u32Height        = ctx->stDstSurface.u32Height;
    pstDstSurface->enColorFmt       = ctx->stDstSurface.enColorFmt;
    pstDstSurface->enComprocessMode = ctx->stDstSurface.enComprocessMode;
    pstDstRect->s32Xpos             = ctx->stDstRect.s32X;
    pstDstRect->s32Ypos             = ctx->stDstRect.s32Y;
    pstDstRect->u32Width            = ctx->stDstRect.u32Width;
    pstDstRect->u32Height           = ctx->stDstRect.u32Height;
    return RK_SUCCESS;
}

RK_S32 test_tde_quick_copy_resize_rotate_task(TEST_TDE_CTX_S *ctx,
        TDE_SURFACE_S *pstSrc, TDE_RECT_S *pstSrcRect,
        TDE_SURFACE_S *pstDst, TDE_RECT_S *pstDstRect) {
    test_tde_fill_src(ctx, pstSrc, pstSrcRect);
    test_tde_fill_dst(ctx, pstDst, pstDstRect);
    return RK_SUCCESS;
}

RK_S32 test_tde_quick_fill_task(TEST_TDE_CTX_S *ctx,
        TDE_SURFACE_S *pstSrc, TDE_SURFACE_S *pstDst,
        TDE_RECT_S *pstDstRect) {
    test_tde_fill_dst(ctx, pstDst, pstDstRect);
    memcpy(RK_MPI_MB_Handle2VirAddr(pstDst->pMbBlk),
           RK_MPI_MB_Handle2VirAddr(pstSrc->pMbBlk),
           RK_MPI_MB_GetSize(pstSrc->pMbBlk));
    return RK_SUCCESS;
}

RK_S32 test_tde_bit_blit_task(TEST_TDE_CTX_S *ctx,
        TDE_SURFACE_S *pstSrc, TDE_RECT_S *pstSrcRect,
        TDE_SURFACE_S *pstDst, TDE_RECT_S *pstDstRect,
        TDE_OPT_S *stOpt, RK_S32 s32Operation,
        RK_U32 u32BlendCmd) {
    if (s32Operation == TDE_OP_MIRROR) {
        // test case : mirror and clip
        stOpt->enMirror = (MIRROR_E)ctx->s32Mirror;
        stOpt->stClipRect.s32Xpos = ctx->stSrcRect.s32X;
        stOpt->stClipRect.s32Ypos = ctx->stSrcRect.s32Y;
        stOpt->stClipRect.u32Width = ctx->stSrcRect.u32Width;
        stOpt->stClipRect.u32Height = ctx->stSrcRect.u32Height;

        test_tde_fill_src(ctx, pstSrc, pstSrcRect);
        test_tde_fill_dst(ctx, pstDst, pstDstRect);
    } else if (s32Operation == TDE_OP_COLOR_KEY) {
        // test case : colorkey
        test_tde_fill_src(ctx, pstSrc, pstSrcRect);
        test_tde_fill_dst(ctx, pstDst, pstDstRect);
        stOpt->unColorKeyValue = ctx->s32ColorKey;
        stOpt->enColorKeyMode = TDE_COLORKEY_MODE_FOREGROUND;
    } else if (s32Operation == TDE_OP_COLOR_BLEND) {
        stOpt->eBlendCmd = (TDE_BLENDCMD_E)u32BlendCmd;
        test_tde_fill_src(ctx, pstSrc, pstSrcRect);
        test_tde_fill_dst(ctx, pstDst, pstDstRect);
    }
    return RK_SUCCESS;
}

RK_S32 test_tde_add_task(TEST_TDE_CTX_S *ctx, TDE_HANDLE hHandle,
        TDE_SURFACE_S *pstSrc, TDE_RECT_S *pstSrcRect,
        TDE_SURFACE_S *pstDst, TDE_RECT_S *pstDstRect) {
    RK_S32 s32Ret = RK_SUCCESS;
    ROTATION_E enRotateAngle = (ROTATION_E)ctx->s32Rotation;
    RK_U32 u32FillData = ctx->s32Color;
    RK_S32 s32Operation = ctx->s32Operation;
    RK_U32 u32BlendCmd = ctx->u32BlendCmd;
    TDE_OPT_S stOpt;
    memset(&stOpt, 0, sizeof(TDE_OPT_S));

    ctx->stSrcSurface.enComprocessMode = (COMPRESS_MODE_E)ctx->s32SrcCompressMode;
    ctx->stDstSurface.enComprocessMode = (COMPRESS_MODE_E)ctx->s32DstCompressMode;

    switch (s32Operation) {
        case TDE_OP_QUICK_COPY: {
          s32Ret = test_tde_quick_copy_resize_rotate_task(ctx,
                    pstSrc, pstSrcRect, pstDst, pstDstRect);
          if (s32Ret)
            RK_LOGE("test_tde_quick_copy_resize_rotate_task fail");

          s32Ret = RK_TDE_QuickCopy(hHandle,
                  pstSrc, pstSrcRect, pstDst, pstDstRect);
          if (s32Ret)
            RK_LOGE("RK_TDE_QuickCopy fail");
        } break;
        case TDE_OP_QUICK_RESIZE: {
          s32Ret = test_tde_quick_copy_resize_rotate_task(ctx,
                  pstSrc, pstSrcRect, pstDst, pstDstRect);
          s32Ret = RK_TDE_QuickResize(hHandle,
                  pstSrc, pstSrcRect, pstDst, pstDstRect);
        } break;
        case TDE_OP_QUICK_FILL: {
          s32Ret = test_tde_quick_fill_task(ctx,
                  pstSrc, pstDst, pstDstRect);
          s32Ret = RK_TDE_QuickFill(hHandle,
                  pstDst, pstDstRect, u32FillData);
        } break;
        case TDE_OP_ROTATION: {
          s32Ret = test_tde_quick_copy_resize_rotate_task(ctx,
                  pstSrc, pstSrcRect, pstDst, pstDstRect);
          s32Ret = RK_TDE_Rotate(hHandle,
                  pstSrc, pstSrcRect, pstDst, pstDstRect,
                  enRotateAngle);
        } break;
        case TDE_OP_COLOR_KEY:
        case TDE_OP_MIRROR:
        case TDE_OP_COLOR_BLEND: {
          s32Ret = test_tde_bit_blit_task(ctx,
                  pstSrc, pstSrcRect, pstDst, pstDstRect,
                  &stOpt, s32Operation, u32BlendCmd);
          s32Ret = RK_TDE_Bitblit(hHandle,
                  pstDst, pstDstRect, pstSrc, pstSrcRect,
                  pstDst, pstDstRect, &stOpt);
        } break;
        default: {
          RK_LOGE("unknown operation type %d", ctx->s32Operation);
        break;
        }
    }
    if (s32Ret != RK_SUCCESS) {
        RK_TDE_CancelJob(hHandle);
        return RK_FAILURE;
    }
    return s32Ret;
}

RK_S32 test_tde_load_src_file(TEST_TDE_CTX_S *ctx, MB_BLK *pstSrcBlk) {
    RK_S32 s32Ret          = RK_SUCCESS;
    RK_U32 u32SrcSize      = 0;
    void  *pSrcData        = RK_NULL;
    MB_BLK srcBlk;

    s32Ret = unit_test_tde_get_src_size(ctx, &u32SrcSize);
    if (s32Ret != RK_SUCCESS) {
        return s32Ret;
    }

    s32Ret = RK_MPI_SYS_MmzAlloc(pstSrcBlk, RK_NULL, RK_NULL, u32SrcSize);
    if (s32Ret != RK_SUCCESS) {
        return s32Ret;
    }

    srcBlk = *pstSrcBlk;

    pSrcData = RK_MPI_MB_Handle2VirAddr(srcBlk);
    s32Ret = test_tde_read_file(ctx->srcLoadFilePath, pSrcData, u32SrcSize);
    if (s32Ret != RK_SUCCESS) {
        return s32Ret;
    }
    RK_MPI_SYS_MmzFlushCache(srcBlk, RK_FALSE);

    RK_U32 u32SrcVirWidth = ctx->u32SrcVirWidth;
    PIXEL_FORMAT_E srcFmt = ctx->stSrcSurface.enColorFmt;
    RK_U32 u32HorStride = RK_MPI_CAL_COMM_GetHorStride(u32SrcVirWidth, srcFmt);
    RK_U32 u32VerStride = ctx->u32SrcVirHeight;
    RK_S32 s32SrcCompressMode = ctx->s32SrcCompressMode;
    if (s32SrcCompressMode != COMPRESS_AFBC_16x16) {
        s32Ret = RK_MPI_MB_SetBufferStride(srcBlk, u32HorStride, u32VerStride);
    }
    return s32Ret;
}

RK_S32 test_tde_load_dst_file(TEST_TDE_CTX_S *ctx, MB_BLK *pstDstBlk) {
    RK_S32 s32Ret          = RK_SUCCESS;
    RK_U32 u32DstSize      = 0;
    void  *pDstData        = RK_NULL;
    MB_BLK dstBlk;

    s32Ret = unit_test_tde_get_dst_size(ctx, &u32DstSize);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("unit_test_tde_get_dst_size fail");
        return s32Ret;
    }

    s32Ret = RK_MPI_SYS_MmzAlloc(pstDstBlk, RK_NULL, RK_NULL, u32DstSize);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("RK_MPI_SYS_MmzAlloc fail");
        return s32Ret;
    }

    dstBlk = *pstDstBlk;

    pDstData = RK_MPI_MB_Handle2VirAddr(dstBlk);

    if (ctx->dstLoadFilePath) {
        s32Ret = test_tde_read_file(ctx->dstLoadFilePath, pDstData, u32DstSize);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("test_tde_read_file fail");
            return s32Ret;
        }
    }

    RK_MPI_SYS_MmzFlushCache(dstBlk, RK_FALSE);

    RK_U32 u32DstVirWidth = ctx->u32DstVirWidth;
    PIXEL_FORMAT_E dstFmt = ctx->stDstSurface.enColorFmt;
    RK_U32 u32HorStride = RK_MPI_CAL_COMM_GetHorStride(u32DstVirWidth, dstFmt);
    RK_U32 u32VerStride = ctx->u32DstVirHeight;
    RK_S32 s32DstCompressMode = ctx->s32DstCompressMode;
    if (s32DstCompressMode != COMPRESS_AFBC_16x16) {
        s32Ret = RK_MPI_MB_SetBufferStride(dstBlk, u32HorStride, u32VerStride);
    }

    if (s32Ret)
        RK_LOGE("RK_MPI_MB_SetBufferStride fail");

    return s32Ret;
}

RK_S32 test_tde_create_dstBlk(TEST_TDE_CTX_S *ctx, MB_BLK *pstDstBlk) {
    RK_S32 s32Ret = RK_SUCCESS;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    stPicBufAttr.u32Width = ctx->stDstSurface.u32Width;
    stPicBufAttr.u32Height = ctx->stDstSurface.u32Height;
    stPicBufAttr.enPixelFormat = ctx->stDstSurface.enColorFmt;
    stPicBufAttr.enCompMode = (COMPRESS_MODE_E)(ctx->s32DstCompressMode);
    s32Ret = RK_MPI_CAL_TDE_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        return s32Ret;
    }
    s32Ret = RK_MPI_SYS_MmzAlloc(pstDstBlk, RK_NULL, RK_NULL, stMbPicCalResult.u32MBSize);

    if (ctx->backgroundFile) {
        void *pDstData = RK_MPI_MB_Handle2VirAddr(*pstDstBlk);
        test_tde_read_file(ctx->backgroundFile, pDstData, stMbPicCalResult.u32MBSize);
        RK_MPI_SYS_MmzFlushCache(*pstDstBlk, RK_FALSE);
    }

    return s32Ret;
}

RK_S32 test_tde_job(TEST_TDE_CTX_S *ctx) {
    RK_S32 s32Ret = RK_SUCCESS;
    MB_BLK srcBlk = RK_NULL;
    MB_BLK dstBlk = RK_NULL;
    RK_U32 u32TaskIndex = 0;
    TDE_HANDLE hHandle[TDE_MAX_JOB_NUM];
    TDE_SURFACE_S pstSrc[TDE_MAX_TASK_NUM];
    TDE_RECT_S pstSrcRect[TDE_MAX_TASK_NUM];
    TDE_SURFACE_S pstDst[TDE_MAX_TASK_NUM];
    TDE_RECT_S pstDstRect[TDE_MAX_TASK_NUM];

    s32Ret = test_tde_load_src_file(ctx, &srcBlk);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("test_tde_load_src_file fail");
        goto __FAILED;
    }

    s32Ret = test_tde_load_dst_file(ctx, &dstBlk);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("test_tde_load_dst_file fail");
        goto __FAILED;
    }

    for (RK_S32 u32JobIdx = 0; u32JobIdx < ctx->s32JobNum; u32JobIdx++) {
        hHandle[u32JobIdx] = RK_TDE_BeginJob();
        if (RK_ERR_TDE_INVALID_HANDLE == hHandle[u32JobIdx]) {
            RK_LOGE("start job fail");
            goto __FAILED;
        }
        for (u32TaskIndex = 0; u32TaskIndex < ctx->s32TaskNum; u32TaskIndex++) {
            pstSrc[u32TaskIndex].pMbBlk = srcBlk;
            pstDst[u32TaskIndex].pMbBlk = dstBlk;
            s32Ret = test_tde_add_task(ctx, hHandle[u32JobIdx],
                    &pstSrc[u32TaskIndex], &pstSrcRect[u32TaskIndex],
                    &pstDst[u32TaskIndex], &pstDstRect[u32TaskIndex]);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("test_tde_add_task fail");
                goto __FAILED;
            }
        }
        s32Ret = RK_TDE_EndJob(hHandle[u32JobIdx], RK_FALSE, RK_TRUE, 10);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_TDE_EndJob fail");
            RK_TDE_CancelJob(hHandle[u32JobIdx]);
            goto __FAILED;
        }
        RK_TDE_WaitForDone(hHandle[u32JobIdx]);
    }

    for (RK_S32 u32TaskIdx = 0; u32TaskIdx < ctx->s32TaskNum; u32TaskIdx++) {
        s32Ret = test_tde_save_result(ctx, &(pstDst[u32TaskIdx]), u32TaskIdx);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("test_tde_save_result fail");
            goto __FAILED;
        }
    }

__FAILED:
    if (dstBlk) {
        RK_MPI_SYS_Free(dstBlk);
    }

    if (srcBlk) {
        RK_MPI_SYS_Free(srcBlk);
    }
    return s32Ret;
}

RK_S32 unit_test_mpi_tde(TEST_TDE_CTX_S *ctx) {
    RK_S32 s32Ret = RK_SUCCESS;

    s32Ret = RK_TDE_Open();
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("RK_TDE_Open failed, ret = %X\n", s32Ret);
        return RK_FAILURE;
    }
    for (RK_S32 i = 0; i < ctx->s32LoopCount; i++) {
        s32Ret = test_tde_job(ctx);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("test_tde_job failed, ret = %X\n", s32Ret);
            return s32Ret;
        }
        RK_LOGI("Running mpi tde test loop count %d.", i + 1);
    }
    RK_TDE_Close();
    return s32Ret;
}

RK_S32 test_tde_set_default_parameter(TEST_TDE_CTX_S *ctx) {
    if (ctx->stSrcRect.u32Width == 0) {
        ctx->stSrcRect.u32Width = ctx->stSrcSurface.u32Width;
    }
    if (ctx->stSrcRect.u32Height == 0) {
        ctx->stSrcRect.u32Height = ctx->stSrcSurface.u32Height;
    }
    if (ctx->stDstRect.u32Width == 0) {
        ctx->stDstRect.u32Width = ctx->stDstSurface.u32Width;
    }
    if (ctx->stDstRect.u32Height == 0) {
        ctx->stDstRect.u32Height = ctx->stDstSurface.u32Height;
    }
    if (ctx->u32SrcVirWidth == 0) {
        ctx->u32SrcVirWidth = ctx->stSrcSurface.u32Width;
    }
    if (ctx->u32SrcVirHeight == 0) {
        ctx->u32SrcVirHeight = ctx->stSrcSurface.u32Height;
    }
    ctx->stSrcSurface.enComprocessMode = (COMPRESS_MODE_E)ctx->s32SrcCompressMode;
    ctx->stDstSurface.enComprocessMode = (COMPRESS_MODE_E)ctx->s32DstCompressMode;
    return RK_SUCCESS;
}

RK_S32 test_tde_set_proc_parameter(TEST_TDE_CTX_S *ctx, TEST_TDE_PROC_CTX_S *tdeTestCtx) {
    memset(tdeTestCtx, 0, sizeof(TEST_TDE_PROC_CTX_S));
    tdeTestCtx->pstSrc.u32Width = ctx->stSrcSurface.u32Width;
    tdeTestCtx->pstSrc.u32Height = ctx->stSrcSurface.u32Height;
    tdeTestCtx->pstSrc.enColorFmt = ctx->stSrcSurface.enColorFmt;
    tdeTestCtx->pstSrc.enComprocessMode = (COMPRESS_MODE_E)ctx->s32SrcCompressMode;
    tdeTestCtx->pstSrcRect.s32Xpos = ctx->stSrcRect.s32X;
    tdeTestCtx->pstSrcRect.s32Ypos = ctx->stSrcRect.s32Y;
    tdeTestCtx->pstSrcRect.u32Width = ctx->stSrcRect.u32Width;
    tdeTestCtx->pstSrcRect.u32Height = ctx->stSrcRect.u32Height;
    tdeTestCtx->pstDst.u32Width = ctx->stDstSurface.u32Width;
    tdeTestCtx->pstDst.u32Height = ctx->stDstSurface.u32Height;
    tdeTestCtx->pstDst.enColorFmt = ctx->stDstSurface.enColorFmt;
    tdeTestCtx->pstDst.enComprocessMode = (COMPRESS_MODE_E)ctx->s32DstCompressMode;
    tdeTestCtx->pstDstRect.s32Xpos = ctx->stDstRect.s32X;
    tdeTestCtx->pstDstRect.s32Ypos = ctx->stDstRect.s32Y;
    tdeTestCtx->pstDstRect.u32Width = ctx->stDstRect.u32Width;
    tdeTestCtx->pstDstRect.u32Height = ctx->stDstRect.u32Height;
    tdeTestCtx->opType = ctx->s32Operation;
    tdeTestCtx->s32ProcessTimes = ctx->s32ProcessTime;
    tdeTestCtx->s32JobNum = ctx->s32JobNum;
    tdeTestCtx->s32TaskNum = ctx->s32TaskNum;
    return RK_SUCCESS;
}

static const char *const usages[] = {
    "./rk_mpi_tde_test [-i SRC_PATH] [-w SRC_WIDTH] [-h SRC_HEIGHT] [-W DST_WIDTH] [-H DST_HEIGHT]...",
    NULL,
};

static void mpi_tde_test_show_options(const TEST_TDE_CTX_S *ctx) {
    RK_PRINT("cmd parse result: \n");
    RK_PRINT("input src file name    : %s\n", ctx->srcLoadFilePath);
    RK_PRINT("input dst file name    : %s\n", ctx->dstLoadFilePath);
    RK_PRINT("output file name       : %s\n", ctx->dstFilePath);
    RK_PRINT("loop count             : %ld\n", ctx->s32LoopCount);
    RK_PRINT("job number             : %ld\n", ctx->s32JobNum);
    RK_PRINT("task number            : %ld\n", ctx->s32TaskNum);
    RK_PRINT("input width            : %ld\n", ctx->stSrcSurface.u32Width);
    RK_PRINT("input height           : %ld\n", ctx->stSrcSurface.u32Height);
    RK_PRINT("input vir width        : %ld\n", ctx->u32SrcVirWidth);
    RK_PRINT("input vir height       : %ld\n", ctx->u32SrcVirHeight);
    RK_PRINT("src compress mode      : %ld\n", ctx->s32SrcCompressMode);
    RK_PRINT("dst compress mode      : %ld\n", ctx->s32DstCompressMode);
    RK_PRINT("input pixel format     : %ld\n", ctx->stSrcSurface.enColorFmt);
    RK_PRINT("output width           : %ld\n", ctx->stDstSurface.u32Width);
    RK_PRINT("output height          : %ld\n", ctx->stDstSurface.u32Height);
    RK_PRINT("output pixel format    : %ld\n", ctx->stDstSurface.enColorFmt);
    RK_PRINT("operation type         : %ld\n", ctx->s32Operation);
}

int rk_mpi_tde_test_enter(int argc, const char **argv) {
    TEST_TDE_CTX_S ctx;
    RK_S32 s32Ret = RK_SUCCESS;

    memset(&ctx, 0, sizeof(TEST_TDE_CTX_S));

    //  set default params.
    ctx.dstFilePath     = RK_NULL;
    ctx.s32LoopCount    = 1;
    ctx.s32JobNum       = 1;
    ctx.s32TaskNum      = 1;
    ctx.s32Operation    = 0;
    ctx.u32BlendCmd     = 1;
    ctx.stSrcSurface.enColorFmt = RK_FMT_YUV420SP;
    ctx.stSrcSurface.enComprocessMode = COMPRESS_MODE_NONE;
    ctx.stDstSurface.enColorFmt = RK_FMT_YUV420SP;
    ctx.stDstSurface.enComprocessMode = COMPRESS_MODE_NONE;
    ctx.bPerformace = RK_FALSE;
    ctx.s32ProcessTime = 800;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("basic options:"),
        OPT_STRING('i', "input_src",  &(ctx.srcLoadFilePath),
                    "input file name. e.g.(/userdata/1080p.nv12). <required>", NULL, 0, 0),
        OPT_STRING('I', "input_dst",  &(ctx.dstLoadFilePath),
                    "input file name. e.g.(/userdata/1080p.nv12). <required>", NULL, 0, 0),
        OPT_INTEGER('w', "src_width", &(ctx.stSrcSurface.u32Width),
                    "src width. e.g.(1920). <required>", NULL, 0, 0),
        OPT_INTEGER('h', "src_height", &(ctx.stSrcSurface.u32Height),
                    "src height. e.g.(1080). <required>", NULL, 0, 0),
        OPT_INTEGER('\0', "src_vir_width", &(ctx.u32SrcVirWidth),
                    "src vir width. e.g.(1920).", NULL, 0, 0),
        OPT_INTEGER('\0', "src_vir_height", &(ctx.u32SrcVirHeight),
                    "src vir height. e.g.(1080).", NULL, 0, 0),
        OPT_INTEGER('\0', "src_compress", &(ctx.s32SrcCompressMode),
                    "src compress mode. e.g.(1).", NULL, 0, 0),
        OPT_INTEGER('W', "dst_width", &(ctx.stDstSurface.u32Width),
                    "dst width. e.g.(1920). <required>", NULL, 0, 0),
        OPT_INTEGER('H', "dst_height", &(ctx.stDstSurface.u32Height),
                    "dst height. e.g.(1080). <required>", NULL, 0, 0),
        OPT_INTEGER('\0', "dst_compress", &(ctx.s32DstCompressMode),
                    "dst compress mode. e.g.(1).", NULL, 0, 0),
        OPT_STRING('o', "output", &(ctx.dstFilePath),
                    "output file path. e.g.(/userdata/tde/). default(NULL).", NULL, 0, 0),
        OPT_STRING('\0', "background", &(ctx.backgroundFile),
                    "background file. e.g.(/userdata/tde/xxx.bin). default(NULL).", NULL, 0, 0),
        OPT_INTEGER('n', "loop_count", &(ctx.s32LoopCount),
                    "loop running count. can be any count. default(1)", NULL, 0, 0),
        OPT_INTEGER('j', "job_number", &(ctx.s32JobNum),
                    "the job number of vgs. default(1).", NULL, 0, 0),
        OPT_INTEGER('t', "task_number", &(ctx.s32TaskNum),
                    "the task number of one job. default(1).", NULL, 0, 0),
        OPT_INTEGER('f', "src_format", &(ctx.stSrcSurface.enColorFmt),
                    "src pixel format. default(0. 0 is NV12).", NULL, 0, 0),
        OPT_INTEGER('F', "dst_format", &(ctx.stDstSurface.enColorFmt),
                    "dst pixel format. default(0. 0 is NV12).", NULL, 0, 0),
        OPT_INTEGER('p', "operation", &(ctx.s32Operation),
                    "operation type. default(0).\n\t\t\t\t\t0: quick copy.\n\t\t\t\t\t1: quick resize."
                     "\n\t\t\t\t\t2: quick fill.\n\t\t\t\t\t3: rotation.\n\t\t\t\t\t4: mirror and flip."
                     "\n\t\t\t\t\t5: colorkey. \n\t\t\t\t\t6: blend.", NULL, 0, 0),
        OPT_INTEGER('\0', "blend_cmd", &(ctx.u32BlendCmd),
                    "blend cmd. default(3). 2: src, 3: src_over, 4: dst_over, 13: dst", NULL, 0, 0),
        OPT_INTEGER('\0', "src_rect_x", &(ctx.stSrcRect.s32X),
                    "src rect x. default(0).", NULL, 0, 0),
        OPT_INTEGER('\0', "src_rect_y", &(ctx.stSrcRect.s32Y),
                    "src rect y. default(0).", NULL, 0, 0),
        OPT_INTEGER('\0', "src_rect_w", &(ctx.stSrcRect.u32Width),
                    "src rect width. default(src_width).", NULL, 0, 0),
        OPT_INTEGER('\0', "src_rect_h", &(ctx.stSrcRect.u32Height),
                    "src rect height. default(src_height).", NULL, 0, 0),
        OPT_INTEGER('\0', "dst_rect_x", &(ctx.stDstRect.s32X),
                    "dst rect x. default(0).", NULL, 0, 0),
        OPT_INTEGER('\0', "dst_rect_y", &(ctx.stDstRect.s32Y),
                    "dst rect y. default(0).", NULL, 0, 0),
        OPT_INTEGER('\0', "dst_rect_w", &(ctx.stDstRect.u32Width),
                    "dst rect width. default(dst_width).", NULL, 0, 0),
        OPT_INTEGER('\0', "dst_rect_h", &(ctx.stDstRect.u32Height),
                    "dst rect height. default(dst_height).", NULL, 0, 0),
        OPT_INTEGER('\0', "performace", &(ctx.bPerformace),
                    "test performace mode. default(0).", NULL, 0, 0),
        OPT_INTEGER('\0', "proc_time", &(ctx.s32ProcessTime),
                    "ProcessTime. default(800).", NULL, 0, 0),
        OPT_INTEGER('\0', "colorkey", &(ctx.s32ColorKey),
                    "colorkey value. default(0).", NULL, 0, 0),
        OPT_INTEGER('c', "fill color", &(ctx.s32Color),
                    "fill color. default(0).", NULL, 0, 0),
        OPT_INTEGER('r', "rotation", &(ctx.s32Rotation),
                    "rotation angle. default(0). 0: 0. 1: 90. 2: 180. 3: 270", NULL, 0, 0),
        OPT_INTEGER('m', "mirror", &(ctx.s32Mirror),
                    "mirror or flip. default(0). 0: none. 1: flip. 2: mirror. 3: both", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nselect a test case to run.",
                                 "\nuse --help for details.");

    argc = argparse_parse(&argparse, argc, argv);
    if (argc < 0)
        return 0;

    test_tde_set_default_parameter(&ctx);
    mpi_tde_test_show_options(&ctx);

    if (ctx.stSrcSurface.u32Width <= 0
        || ctx.stSrcSurface.u32Height <= 0
        || ctx.stDstSurface.u32Width <= 0
        || ctx.stDstSurface.u32Height <= 0) {
        argparse_usage(&argparse);
        goto __FAILED;
    }

    s32Ret = RK_MPI_SYS_Init();
    if (s32Ret != RK_SUCCESS) {
        goto __FAILED;
    }

    if (ctx.bPerformace) {
        TEST_TDE_PROC_CTX_S tdeTestCtx;
        test_tde_set_proc_parameter(&ctx, &tdeTestCtx);
        s32Ret = TEST_TDE_MultiTest(&tdeTestCtx);
    } else {
        s32Ret = unit_test_mpi_tde(&ctx);
    }

    if (s32Ret != RK_SUCCESS) {
        goto __FAILED;
    }

    s32Ret = RK_MPI_SYS_Exit();
    if (s32Ret != RK_SUCCESS) {
        goto __FAILED;
    }

    RK_LOGI("test running ok!");
    return 0;
__FAILED:
    RK_LOGI("test runnning failed!");
    return -1;
}

#ifdef OS_LINUX
int main(int argc, const char **argv) {
    return rk_mpi_tde_test_enter(argc, argv);
}
#endif

#ifdef OS_RTT
#include <finsh.h>
int rk_mpi_tde_test(int argc, char **argv)
{
    argparse_excute_main(argc, argv, rk_mpi_tde_test_enter);
    return 0;
}

MSH_CMD_EXPORT(rk_mpi_tde_test, rockit tde module test);
#endif