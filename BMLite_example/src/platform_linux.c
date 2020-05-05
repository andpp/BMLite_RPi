/*
 * Copyright (c) 2020 Fingerprint Cards AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    platform_linux.c
 * @brief   Linux platform specific functions
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <sys/time.h>
#include <unistd.h>

#include "platform.h"

uint64_t platform_get_time(void)
{
    struct timeval current_time;
    uint64_t time_in_ms;

    gettimeofday(&current_time, NULL);
    /* Seconds and microseconds are converted to milliseconds */
    time_in_ms = current_time.tv_usec / 1000 + current_time.tv_sec * 1000;

    return time_in_ms;
}

void platform_clear_screen(void)
{
    system("clear");
}

void hal_timebase_busy_wait(uint32_t ms)
{
    usleep(ms * 1000);
}