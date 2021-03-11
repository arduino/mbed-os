/*
 * Copyright 2020 Arduino SA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 *  Provides cyhal porting to generic mbed APIs
 */

#include <stddef.h>
#include "cyhal_sdio.h"
//#include "cyhal_spi.h"
#include "cyhal_gpio.h"
#include <stdio.h>
#include "sockets.h"

void Cy_SysLib_Delay(uint32_t milliseconds) {
	thread_sleep_for(milliseconds);
}

void Cy_SysLib_DelayUs(uint16_t microseconds) {
	wait_us(microseconds);
}

static bool filesystem_mounted = false;
extern bool wiced_filesystem_mount();

int wiced_filesystem_file_open(int* fd, const char* filename) {
	if (!filesystem_mounted) {
		filesystem_mounted = wiced_filesystem_mount();
	}
	if (!filesystem_mounted) {
		return WHD_BADARG;
	}
	*fd = open(filename, O_RDONLY);
	if (*fd == -1) {
		return WHD_BADARG;
	}
	return WHD_SUCCESS;
}

int wiced_filesystem_file_seek(int* fd, uint32_t offset) {
	if (*fd == -1) {
		return WHD_BADARG;
	}
	lseek(*fd, offset, SEEK_SET);
	return WHD_SUCCESS;
}

int wiced_filesystem_file_read(int* fd, void *buffer, uint32_t maxsize, uint32_t* size) {
	if (*fd == -1) {
		return WHD_BADARG;
	}
	*size = read(*fd, buffer, maxsize);
	return WHD_SUCCESS;
}

int wiced_filesystem_file_close(int* fd) {
	if (*fd == -1) {
		return WHD_BADARG;
	}
	close(*fd);
	return WHD_SUCCESS;
}
