/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "rkadk_common.h"
#include "rkadk_media_comm.h"
#include "rkadk_disp.h"
#include "rkadk_log.h"
#include "rkadk_param.h"
#include "isp/sample_isp.h"
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int optind;
extern char *optarg;

static bool is_quit = false;
static RKADK_CHAR optstr[] = "a:I:p:h";

#define IQ_FILE_PATH "/etc/iqfiles"

static void print_usage(const RKADK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-a /etc/iqfiles] [-I 0]\n", name);
  printf("\t-a: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, Default /etc/iqfiles,"
         "without this option aiq should run in other application\n");
  printf("\t-I: camera id, Default 0\n");
  printf("\t-p: param ini directory path, Default:/data/rkadk\n");
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  is_quit = true;
}

int main(int argc, char *argv[]) {
  int c, ret;
  RKADK_U32 u32CamId = 0;
  RKADK_DISP_ATTR_S stDispAttr;
  const char *iniPath = NULL;
  char path[RKADK_PATH_LEN];
  char sensorPath[RKADK_MAX_SENSOR_CNT][RKADK_PATH_LEN];

#ifdef RKAIQ
  RKADK_PARAM_FPS_S stFps;
  const char *tmp_optarg = optarg;
  SAMPLE_ISP_PARAM stIspParam;

  memset(&stIspParam, 0, sizeof(SAMPLE_ISP_PARAM));
  stIspParam.iqFileDir = IQ_FILE_PATH;
#endif

  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
#ifdef RKAIQ
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }

      if (tmp_optarg)
        stIspParam.iqFileDir = (char *)tmp_optarg;
      break;
#endif
    case 'I':
      u32CamId = atoi(optarg);
      break;
    case 'p':
      iniPath = optarg;
      RKADK_LOGP("iniPath: %s", iniPath);
      break;
    case 'h':
    default:
      print_usage(argv[0]);
      optind = 0;
      return 0;
    }
  }
  optind = 0;

  RKADK_LOGP("#camera id: %d", u32CamId);

  memset(&stDispAttr, 0, sizeof(RKADK_DISP_ATTR_S));
  stDispAttr.stVpssCropRect.u32X = 120;
  stDispAttr.stVpssCropRect.u32Y = 120;
  stDispAttr.stVpssCropRect.u32Width = 720;
  stDispAttr.stVpssCropRect.u32Height = 480;
  stDispAttr.stVoRect.u32X = 120;
  stDispAttr.stVoRect.u32Y = 120;
  stDispAttr.stVoRect.u32Width = 480;
  stDispAttr.stVoRect.u32Height = 720;

  RKADK_MPI_SYS_Init();

  if (iniPath) {
    memset(path, 0, RKADK_PATH_LEN);
    memset(sensorPath, 0, RKADK_MAX_SENSOR_CNT * RKADK_PATH_LEN);
    sprintf(path, "%s/rkadk_setting.ini", iniPath);
    for (int i = 0; i < RKADK_MAX_SENSOR_CNT; i++)
      sprintf(sensorPath[i], "%s/rkadk_setting_sensor_%d.ini", iniPath, i);

    /*
    lg:
      char *sPath[] = {"/data/rkadk/rkadk_setting_sensor_0.ini",
      "/data/rkadk/rkadk_setting_sensor_1.ini", NULL};
    */
    char *sPath[] = {sensorPath[0], sensorPath[1], NULL};

    RKADK_PARAM_Init(path, sPath);
  } else {
    RKADK_PARAM_Init(NULL, NULL);
  }

#ifdef RKAIQ
  stFps.enStreamType = RKADK_STREAM_TYPE_SENSOR;
  ret = RKADK_PARAM_GetCamParam(u32CamId, RKADK_PARAM_TYPE_FPS, &stFps);
  if (ret) {
    RKADK_LOGE("RKADK_PARAM_GetCamParam fps failed");
    return -1;
  }

  stIspParam.WDRMode = RK_AIQ_WORKING_MODE_NORMAL;
  stIspParam.bMultiCam = false;
  stIspParam.fps = stFps.u32Framerate;
  SAMPLE_ISP_Start(u32CamId, stIspParam);
  RKADK_BUFINFO("isp[%d] init", u32CamId);
#endif
  ret = RKADK_DISP_Init(u32CamId);
  if (ret) {
    RKADK_LOGE("RKADK_DISP_Init failed(%d)", ret);
#ifdef RKAIQ
    SAMPLE_ISP_Stop(u32CamId);
#endif
    return -1;
  }

  signal(SIGINT, sigterm_handler);
  char cmd[64];
  printf("\n#Usage: input 'quit' to exit programe!\n"
         "peress any other key to quit\n");

  while (!is_quit) {
    fgets(cmd, sizeof(cmd), stdin);
    if (strstr(cmd, "quit") || is_quit) {
      RKADK_LOGP("#Get 'quit' cmd!");
      break;
    } else if (strstr(cmd, "rs")) { //crop and adjust the display area
      stDispAttr.stVpssCropRect.u32X = 120;
      stDispAttr.stVpssCropRect.u32Y = 120;
      stDispAttr.stVpssCropRect.u32Width = 720;
      stDispAttr.stVpssCropRect.u32Height = 480;
      stDispAttr.stVoRect.u32X = 120;
      stDispAttr.stVoRect.u32Y = 120;
      stDispAttr.stVoRect.u32Width = 480;
      stDispAttr.stVoRect.u32Height = 720;
      RKADK_DISP_SetAttr(u32CamId, &stDispAttr);
    } else if (strstr(cmd, "nm")) {  // full screen display
      stDispAttr.stVpssCropRect.u32X = 0;
      stDispAttr.stVpssCropRect.u32Y = 0;
      stDispAttr.stVpssCropRect.u32Width = 1280;
      stDispAttr.stVpssCropRect.u32Height = 720;
      stDispAttr.stVoRect.u32X = 0;
      stDispAttr.stVoRect.u32Y = 0;
      stDispAttr.stVoRect.u32Width = 720;
      stDispAttr.stVoRect.u32Height = 1280;
      RKADK_DISP_SetAttr(u32CamId, &stDispAttr);
    }
  }

  RKADK_DISP_DeInit(u32CamId);

#ifdef RKAIQ
  SAMPLE_ISP_Stop(u32CamId);
#endif
  RKADK_MPI_SYS_Exit();
  return 0;
}
