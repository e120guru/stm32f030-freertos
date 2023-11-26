#include "stm32f0xx.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_system.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void SystemInit(void) 
{ 
	/* TODO: enable clock security subsystem*/
	LL_RCC_HSI_Enable();
	while (!LL_RCC_HSI_IsReady());

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI);
	
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
	LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK1);

	LL_RCC_HSE_Enable();
	while (!LL_RCC_HSE_IsReady());

	LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

	LL_RCC_PLL_Disable();
	while (LL_RCC_PLL_IsReady());
	
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLL_MUL_6);
	
	LL_RCC_PLL_Enable();
	while (!LL_RCC_PLL_IsReady());
	
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

	LL_RCC_HSI_Disable();
	while (LL_RCC_HSI_IsReady());

	SystemCoreClock = 48000000;
}

uint32_t SystemCoreClock;


#define LED_STATUS LL_GPIO_PIN_13

void gpio_init(void) {
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	LL_GPIO_InitTypeDef init = { 0 };
	init.Pin = LED_STATUS;
	init.Mode = LL_GPIO_MODE_OUTPUT;
	init.Speed = LL_GPIO_SPEED_FREQ_LOW;
	init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;

	LL_GPIO_Init(GPIOC, &init);
	LL_GPIO_ResetOutputPin(GPIOC, LED_STATUS);
	LL_GPIO_SetOutputPin(GPIOC, 0);
}

QueueHandle_t led_queue;
xTaskHandle led_task_handle, timer_task_handle;

/* This tasks blinks the LED on the board, it receives new timer
 * value from the message queue */
void led_task(void* arg) {
	uint32_t led_delay = 100;

	while (1) {
		xQueueReceive(led_queue, &led_delay, 0);

		LL_GPIO_SetOutputPin(GPIOC, LED_STATUS);
		vTaskDelay(led_delay / portTICK_PERIOD_MS);

		LL_GPIO_ResetOutputPin(GPIOC, LED_STATUS);
		vTaskDelay(led_delay / portTICK_PERIOD_MS);
	}
}

/* This task sends a new timer value to the LED task every five seconds */
void timer_task(void* arg) {
	uint32_t counter = 100;

	while (1) {
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		counter += 100;
		if (counter > 1000) {
			counter = 100;
		}
		xQueueSend(led_queue, &counter, 0);
	}
}


int main(void) {
	gpio_init();
	/* SysTick clock is HCLK = 48M, to get 1000 Hz, we divide by 6000 */
	SysTick_Config(6000*8);

	/* Creating the queue to send/receive LED periods */
	led_queue = xQueueCreate(1, sizeof(uint32_t));

	/* Create the LED blinking task */
	xTaskCreate(
		led_task,
		"LED",
		configMINIMAL_STACK_SIZE,
		NULL,
		2,
		&led_task_handle
	);

	/* Create the timer task that sends the blink periods */
	xTaskCreate(
		timer_task,
		"Timer",
		configMINIMAL_STACK_SIZE,
		NULL,
		1,
		&timer_task_handle
	);

	vTaskStartScheduler();

	/* Should not get here */
	while (1) {
		;
	}
}

