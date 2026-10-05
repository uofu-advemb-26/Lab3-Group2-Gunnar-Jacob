#define UNIT_TEST 1

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

extern void main_thread(void *params);
extern void side_thread(void *params);

void setUp(void) {}

void tearDown(void) {}

int lock_depth = 0;

static TaskHandle_t h_main = NULL;
static TaskHandle_t h_side = NULL;

int __wrap_xSemaphoreTake(SemaphoreHandle_t sem, uint32_t delay) {
    lock_depth++;
    return 1; // pdTRUE
}

int __wrap_xSemaphoreGive(SemaphoreHandle_t sem) {
    lock_depth--;
    return 1; // pdTRUE
}

void __wrap_vTaskDelay(uint32_t ticks) {
    //nothing
}

#define CYW43_WL_GPIO_LED_PIN 0

 //-----------------------------------------------------------------------------------------------------------------------

void test_for_orphaned_lock_main(void) {
    main_thread(NULL);
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, lock_depth, "deadlock detected, test failed");
}

void test_for_orphaned_lock_side(void) {
    side_thread(NULL);
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, lock_depth, "deadlock detected, test failed");
}

void deadlock_supervisor_task(void *pvParameters)
{
    TaskHandle_t test_task_handle = (TaskHandle_t) pvParameters;
    uint32_t result;

    vTaskDelay(pdMS_TO_TICKS(5000));

    if (eTaskGetState(h_main) == eBlocked &&
        eTaskGetState(h_side) == eBlocked) {
        result = 2;
    } else {
        result = 1;
    }

    vTaskDelete(h_main);
    vTaskDelete(h_side);
    h_main = NULL;
    h_side = NULL;

    xTaskNotify(test_task_handle, result, eSetValueWithOverwrite);

    vTaskDelete(NULL);
}

void test_concurrent_deadlock_detection(void)
{
    uint32_t result = 0;
    TaskHandle_t test_task_handle = xTaskGetCurrentTaskHandle();

        xTaskCreate(main_thread, "MainThread", configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 1, &h_main);
        xTaskCreate(side_thread, "SideThread", configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 1, &h_side);

        xTaskCreate(deadlock_supervisor_task, "Supervisor", configMINIMAL_STACK_SIZE, test_task_handle, tskIDLE_PRIORITY + 2, NULL);

    // Blocks only test_runner_task. The two workers and supervisor still run.
    TEST_ASSERT_EQUAL(
        pdTRUE,
        xTaskNotifyWait(0, UINT32_MAX, &result,
                        pdMS_TO_TICKS(6000))
    );

    TEST_ASSERT_EQUAL_MESSAGE(
        1,
        result,
        "deadlock detected"
    );
}

void test_runner_task() {
    cyw43_arch_init();
    stdio_init_all();
    bool on = false;
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("\n--- Starting Unit Tests ---\n");
    while (1) {
        printf("Start tests\n");
        UNITY_BEGIN();
        on = !on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        RUN_TEST(test_concurrent_deadlock_detection);
        RUN_TEST(test_for_orphaned_lock_side);
        RUN_TEST(test_for_orphaned_lock_main);
        sleep_ms(2000);
        UNITY_END();
    }
}
// ---------------------------------------------------------------------------------------------------------

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

void test_test(void) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, 0, "working properly");
}

int main (void)
{
    xTaskCreate(test_runner_task, "test_runner", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);
    vTaskStartScheduler();
}

