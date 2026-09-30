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

#define THREAD1_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define THREAD1_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define THREAD2_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define THREAD2_TASK_STACK_SIZE configMINIMAL_STACK_SIZE


void setUp(void) {}

void tearDown(void) {}

// This is to test that a semaphore will work by giving and taking it correctly
void test_semaphore(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE, "Failed to create semaphore.");
    TEST_ASSERT_TRUE_MESSAGE(xSemaphoreGive(semaphore) == pdTRUE, "Failed to give semaphore.");
}

void thread1_thread(void *pvParameters)
{
    xSemaphoreTake((SemaphoreHandle_t)pvParameters, 100);
    sleep_ms(1000);
    xSemaphoreTake((SemaphoreHandle_t)pvParameters, 100);
}

void thread2_thread(void *pvParameters)
{
    xSemaphoreTake((SemaphoreHandle_t)pvParameters, 100);
    sleep_ms(1000);
    xSemaphoreTake((SemaphoreHandle_t)pvParameters, 100);
}

void test_semaphore_block(void)
{
    SemaphoreHandle_t semaphore1 = xSemaphoreCreateCounting(1, 1);
    SemaphoreHandle_t semaphore2 = xSemaphoreCreateCounting(1, 1);
    TaskHandle_t thread1;
    TaskHandle_t thread2;
    xTaskCreate(thread1_thread, "Thread1",
                THREAD1_TASK_STACK_SIZE, (void *)semaphore1, THREAD1_TASK_PRIORITY, &thread1);
    xTaskCreate(thread2_thread, "Thread2",
                THREAD2_TASK_STACK_SIZE, (void *)semaphore2, THREAD2_TASK_PRIORITY, &thread2);
    vTaskStartScheduler();

    TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore1, 10) == pdTRUE, "Failed to take semaphore1.");
    TEST_ASSERT_TRUE_MESSAGE(xSemaphoreTake(semaphore2, 10) == pdTRUE, "Failed to take semaphore2.");
    xSemaphoreGive(semaphore1);
    xSemaphoreGive(semaphore2);
    // Do not give the semaphore back to simulate blocking.
}

int main (void)
{
    cyw43_arch_init();
    stdio_init_all();
    bool on = false;
    while (1) {
        sleep_ms(2000); // Give time for TTY to attach.
        printf("Start tests\n");
        UNITY_BEGIN();
        on = !on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        RUN_TEST(test_semaphore);
        RUN_TEST(test_semaphore_block);
        sleep_ms(2000);
        UNITY_END();
    }
}
