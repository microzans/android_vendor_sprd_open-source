/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
/* SPRD: add for apct functions @{ */
#include <sys/time.h>
#include <stdbool.h>
/* @} */
#include "common.h"
#include "cutils/properties.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "battery.h"

#define ENG_FACOTRYMODE_FILE	"/productinfo/factorymode.file"
#define ENG_FACOTRYSYNC_FILE	"/factorysync.file"


extern pthread_mutex_t gBatteryMutex ;
void *charge_thread(void *cookie);
void *power_thread(void *cookie);
void *input_thread(void *write_fd);
void *backlight_thread(void *read_fd);
void log_init(void);
int is_exit = 0;

#define MODE_CHECK_MAX 10
/* SPRD: add for apct functions @{ */
bool gIsApctPowerOffCharge = false;
bool gIsApctRead  = false;
bool getApctChargeSupport()
{
    if (gIsApctRead)
    {
        return gIsApctPowerOffCharge;
    }
    gIsApctRead = true;

    char str[10] = {'\0'};
    char *FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";

    FILE *f = fopen(FILE_NAME, "r");

    if (NULL != f)
    {
        fseek(f, 0, 0);
        fread(str, 5, 1, f);
        fclose(f);

        long apct_config = atol(str);

        gIsApctPowerOffCharge =  (apct_config & 0x8200) == 0x8200 ? true : false;
    }
    return gIsApctPowerOffCharge;
}

//for apct power off to charge time calculation.
void RecordChargeTime()
{
	/*add for Sprd Android performance check tool*/
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &t);
	long charge_animt_time = t.tv_sec*1000 + t.tv_nsec/1000000;

	struct timeval tv;
	tv.tv_sec = tv.tv_usec = 0;
	gettimeofday(&tv, NULL);
	int cmdline_fd = open("/proc/cmdline", O_RDONLY);
	long uboot_time = 0;
	if (cmdline_fd >=0 ) {
		char *p;

		char cmd[1024] = {0};
		read(cmdline_fd, cmd, sizeof(cmd) - 1);
		p = strstr(cmd, "pl_t=");
		if (p!=NULL) {
			p = p+5;
			//LOGE("charge time pl_t=%s", p);
			while(1) {
				char c = *p++;
				if ( c >= '0' && c <= '9') {
					uboot_time = uboot_time*10 + (c - '0');
				} else {
					break;
				}
			}
		}
		close(cmdline_fd);
	}

    int apct_dir_fd = open("/data/apct", O_CREAT, 0777);

	if (apct_dir_fd >= 0)
	{
	    fchmod(apct_dir_fd, 0777);
	    close(apct_dir_fd);
	}

	int chargetime_fd = open("/data/apct/chargetime", O_CREAT | O_RDWR | O_TRUNC, 0666); //umaks make it to 0600
        if (chargetime_fd >=0) {
			char buf[100] = {0};
			//LOGE("charge time: uboot=%ldms  ", uboot_time);
			//LOGE("charge time: user=%ldms  ", charge_animt_time);
			sprintf(buf, "charge time: %ldms  ", charge_animt_time + uboot_time);
			write(chargetime_fd, buf, strlen(buf));
			fchmod(chargetime_fd, 0666);
			close(chargetime_fd);
	}
}
/* @} */

int
main(int argc, char **argv) {
	time_t start = time(NULL);
	int ret, fd=0;
	char charge_prop[PROPERTY_VALUE_MAX]={0};
	int pipe_fds[2];
	int is_factory=0;
	char factory_prop[10]={0};
	struct stat s;
	int i = 0;

	log_init();
	LOGD("\n charge start\n");
	ret = battery_status_init();
	if(ret < 0)
		return -1;

	LOGD("\n charge detecting\n");
	property_get("ro.bootmode", charge_prop, "");
	LOGD("charge_prop: %s\n", charge_prop);
	if(!charge_prop[0] || strncmp(charge_prop, "charge", 6)){
		LOGE("exit 1\n");
		return EXIT_SUCCESS;
	}

#ifdef CONFIG_SYNC_FILE
	do{
		i++;
		if(i>MODE_CHECK_MAX){
			is_factory = 0;
			LOGE("has try %d times, give up , non factory\n", i);
			break;
		}
		sleep(1);
		if ((fd=open(ENG_FACOTRYSYNC_FILE, O_RDWR))<=0) {
			LOGD("%s not exist\n",ENG_FACOTRYSYNC_FILE);
			continue;
		}
		close(fd);
		fd = open(ENG_FACOTRYMODE_FILE, O_RDWR);
		LOGD("open %s fd=%d\n",ENG_FACOTRYMODE_FILE, fd);

		if(fd > 0){
			is_factory = 1;
			close(fd);
			break;
		}else{
			is_factory = 0;
			break;
		}
	}while(1);
#else
	char buf[5]={0};
	do{
		i++;
		if(i>MODE_CHECK_MAX){
			is_factory = 0;
			break;
		}
		sleep(1);
		if (stat(ENG_FACOTRYMODE_FILE, &s) != 0) {
			LOGE("cannot find '%s'\n", ENG_FACOTRYMODE_FILE);
			continue;
		}
		fd = open(ENG_FACOTRYMODE_FILE, O_RDWR);
		if(fd < 0){
			LOGE("open %s error %s\n", ENG_FACOTRYMODE_FILE, strerror(errno));
			continue;
		}

		lseek(fd, 0, SEEK_SET);
		ret = read(fd, buf, 2);
		buf[4] = '\0';
		if(ret <= 0){
			LOGD("factory file read %d error %s\n", ret, strerror(errno));
			close(fd);
			continue;
		}

		ret = atoi(buf);
		if(ret == 1){
			LOGD("fatcotry mode\n");
			is_factory = 1;
			close(fd);
			break;
		}else if(ret == 0){
			LOGD("not fatcotry mode\n");
			is_factory = 0;
			close(fd);
			break;
		}else{
			LOGD("factory mode get %s\n", buf);
			close(fd);
			continue;
		}
	}while(1);

#endif

	if(!is_factory){
		system("echo 0 > /sys/class/android_usb/android0/enable");
	}

	ui_init();

	LOGD("ui_init\n");

	ret = pipe(pipe_fds);
	if(ret){
		LOGE("creat pipe failed\n");
		return -1;
	}

	ui_set_background(BACKGROUND_ICON_NONE);
	ui_show_indeterminate_progress();

    /* SPRD: add for apct functions @{ */
    if (getApctChargeSupport())
    {
        RecordChargeTime();
    }
    /* @} */

	pthread_t t_1, t_2, t_3, t_4;
	ret = pthread_create(&t_1, NULL, charge_thread, NULL);
	if(ret){
		LOGE("thread:charge_thread creat failed\n");
		return -1;
	}
	ret = pthread_create(&t_2, NULL, input_thread, (void *)pipe_fds[1]);
	if(ret){
		LOGE("thread:input_thread creat failed\n");
		return -1;
	}
	ret = pthread_create(&t_3, NULL, backlight_thread, (void *)pipe_fds[0]);
	if(ret){
		LOGE("thread: backlight_thread creat failed\n");
		return -1;
	}
	ret = pthread_create(&t_4, NULL, power_thread, NULL);
	if(ret){
		LOGE("thread: power_thread creat failed\n");
		return -1;
	}

	LOGD("all thread start\n");

	int result;
	pthread_join(t_1, NULL);
	pthread_join(t_2, NULL);
	pthread_join(t_3, NULL);
	pthread_join(t_4, NULL);
	if(!is_factory){
		system("echo 1 > /sys/class/android_usb/android0/enable");
	}
	LOGD("charge app exit\n");

	return EXIT_SUCCESS;
}
