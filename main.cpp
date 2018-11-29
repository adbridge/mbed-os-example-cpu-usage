/* Copyright (c) 2018 Arm Limited
*
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
#include "mbed.h"

#if !defined(MBED_CPU_STATS_ENABLED) || !defined(DEVICE_LPTICKER) || !defined(DEVICE_SLEEP)
#error [NOT_SUPPORTED] test not supported
#endif

DigitalOut led1(LED1);

#define MAX_THREAD_STACK        384
#define SAMPLE_TIME             2000    // msec
#define LOOP_TIME               3000    // msec

uint64_t prev_idle_time = 0;
int32_t wait_time = 5000;      // usec

void busy_thread()
{
    volatile uint64_t i = ~0;

    while(i--) {
        led1 = !led1;
        wait_us(wait_time);
    }
}

void calc_cpu_usage()
{
    mbed_stats_cpu_t stats;
    mbed_stats_cpu_get(&stats);

    uint64_t diff = (stats.idle_time - prev_idle_time);
    uint8_t idle = (diff * 100) / (SAMPLE_TIME*1000);    // usec;
    uint8_t usage = 100 - ((diff * 100) / (SAMPLE_TIME*1000));    // usec;;
    prev_idle_time = stats.idle_time;

    printf("Idle: %d Usage: %d \n", idle, usage);
}

int main()
{
    // Request the shared queue
    EventQueue *stats_queue = mbed_event_queue();
    Thread *thread;
    int id;

    id = stats_queue->call_every(SAMPLE_TIME, calc_cpu_usage);
    thread = new Thread(osPriorityNormal, MAX_THREAD_STACK);
    thread->start(busy_thread);

    // Steadily increase the system load
    for (int count = 1; ; count++) {
        Thread::wait(LOOP_TIME);
        if (wait_time <= 0) {
            break;
        }
        wait_time -= 1000;  // usec
    }
    thread->terminate();
    stats_queue->cancel(id);
    return 0;
}
