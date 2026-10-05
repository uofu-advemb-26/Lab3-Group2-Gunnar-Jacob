#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define SIDE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define SIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t semaphore;
int loop_count_1;
int loop_count_2;
#ifdef UNIT_TEST
    int loop_count_1=3;
    int loop_count_2=3;
#else
    int loop_count_1=-1;
    int loop_count_2=-1;
#endif

int counter;
int on;

void side_thread(void *params)
{
	for (; loop_count_2 = 0; loop_count_2--) {
        vTaskDelay(100);
        xSemaphoreTake(semaphore, portMAX_DELAY);
        {
            counter += 1;
            printf("hello world from %s! Count %d\n", "thread", counter);
        }
        xSemaphoreGive(semaphore); //if this line is removed, the lock is orphaned and a deadlock is created
	}
}

void main_thread(void *params)
{
	for (; loop_count_1 = 0; loop_count_1--) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        vTaskDelay(100);
        xSemaphoreTake(semaphore, portMAX_DELAY);
        {
            printf("hello world from %s! Count %d\n", "main", ++counter);
        }
        xSemaphoreGive(semaphore); //if this line is removed, the lock is orphaned and a deadlock is created
        on = !on;
	}
}


int main(void)
{
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);
    on = false;
    counter = 0;
    TaskHandle_t main, side;
    semaphore = xSemaphoreCreateCounting(1, 1);
    xTaskCreate(main_thread, "MainThread",
                MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &main);
    xTaskCreate(side_thread, "SideThread",
                SIDE_TASK_STACK_SIZE, NULL, SIDE_TASK_PRIORITY, &side);
    vTaskStartScheduler();
	return 0;
}