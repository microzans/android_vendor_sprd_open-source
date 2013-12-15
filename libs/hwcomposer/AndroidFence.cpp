
/*
 * Copyright (C) 2010 The Android Open Source Project
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


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 22/09/2013    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: AndroidFence.h              DESCRIPTION                             *
 **                                   Handle Android Framework fence          *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include <sys/mman.h>
#include <sys/types.h>
#include <hardware/hardware.h>
#include <ion/ion.h>
#include <linux/ion.h>
#include "ion_sprd.h"
#include "AndroidFence.h"
#include <cutils/log.h>

using namespace android;

#define ION_DEVICE "/dev/ion"

static int ion_device_fd = -1;

int sprd_fence_create(char *name, int value)
{
    if (ion_device_fd < 0)
    {
        ALOGE("get ion device failed");
        return -1;
    }

    struct ion_custom_data  custom_data;
    struct ion_fence_data data;

    memset(&data, 0, sizeof(struct ion_fence_data));

    strncpy(data.name, name, sizeof(data.name));
    data.value = value;

    custom_data.cmd = ION_SPRD_CUSTOM_FENCE_CREATE;
    custom_data.arg = (unsigned long)&data;

    int ret = ioctl(ion_device_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
    {
        ALOGE("sprd_fence_create failed");
        return -1;
    }

    return data.fence_fd;
}

int sprd_fence_signal()
{
    if (ion_device_fd < 0)
    {
        ALOGE("get ion device failed");
        return -1;
    }

    struct ion_custom_data  custom_data;
    struct ion_fence_data data;

    memset(&data, 0, sizeof(struct ion_fence_data));

    custom_data.cmd = ION_SPRD_CUSTOM_FENCE_SIGNAL;
    custom_data.arg = (unsigned long)&data;

    int ret = ioctl(ion_device_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
    {
        ALOGE("sprd_fence_signal failed");
        return -1;
    }

    return 0;
}

int openSprdFence()
{
    ion_device_fd = open(ION_DEVICE, O_RDWR);

    if (ion_device_fd < 0)
    {
        ALOGE("open ION_DEVICE failed");
        return -1;
    }

    return 0;
}

void closeSprdFence()
{
    if (ion_device_fd >= 0)
    {
        close(ion_device_fd);
    }
}

void closeAcquireFDs(hwc_display_contents_1_t *list)
{
    if (list)
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l->acquireFenceFd >= 0)
            {
                close(l->acquireFenceFd);
                l->acquireFenceFd = -1;
            }
        }
    }
}

void createRetiredFence(hwc_display_contents_1_t *list)
{
    unsigned val = 1;
    char *name = "HWCRetired";

    if (list)
    {
        list->retireFenceFd = sprd_fence_create(name, val);

        sprd_fence_signal();
    }
}
