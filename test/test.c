#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>



void setUp(void) {}

void tearDown(void) {}


void test_semaphore(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE, "Failed to create semaphore.");
    TEST_ASSERT_NOT_NULL_MESSAGE(xSemaphoreGive(semaphore) == pdTRUE, "Failed to give semaphore.");
}

int main (void)
{
    stdio_init_all();
    while (1) {
        sleep_ms(2000); // Give time for TTY to attach.
        printf("Start tests\n");
        UNITY_BEGIN();
        RUN_TEST(test_semaphore);
        sleep_ms(2000);
        UNITY_END();
    }
}
