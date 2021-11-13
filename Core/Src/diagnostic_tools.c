/*
 * diagnostic_tools.c
 *
 *  Created on: Nov 12, 2021
 *      Author: Rafal
 */

#include "diagnostic_tools.h"
#include "cmsis_os.h"
#include "lwip.h"  // for struct netif data type

extern struct netif gnetif;
char GUI_buffer[GUI_bufferSize] = {0};
osThreadId GUI_UART_TaskHandle;


// osThreadList() (or vTaskList())  calls uxTaskGetSystemState() -	// shows the state of each task, including the task's stack high water mark (the smaller the high water mark
																	// number the closer the task has come to overflowing its stack). Results are in words!
																	// configUSE_TRACE_FACILITY 1 and configUSE_STATS_FORMATTING_FUNCTIONS 1.

void GUI_UART_Task(void const *argument);

void DiagnosticToolsInit(void)
{
	  osThreadDef(GUI_UART_Task111, GUI_UART_Task, osPriorityNormal, 0, 200);  // in words !
	  GUI_UART_TaskHandle = osThreadCreate(osThread(GUI_UART_Task111), NULL);

//		snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "uxTaskGetStackHighWaterMark() = %lu\n\n", uxTaskGetStackHighWaterMark(NULL));
//		HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);
}

void GUI_UART_Task(void const *argument)
{

	//test
	char aaa[256] = {0};

//aaa[19] = 7;


	while(1)
	{
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 1)
		{
			snprintf(GUI_buffer, sizeof(GUI_buffer), "Test value \n");
			HAL_UART_Transmit(&huart3, (uint8_t*)GUI_buffer, strlen(GUI_buffer) + 1, 50);

			snprintf(GUI_buffer, sizeof(GUI_buffer), "IP = %lu\n\n", gnetif.ip_addr.addr);
			HAL_UART_Transmit(&huart3, (uint8_t*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			// print freeRTOS statistics
			osThreadList((unsigned char*)GUI_buffer);
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "\nxPortGetFreeHeapSize() = %u\n\n", xPortGetFreeHeapSize());
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "xPortGetMinimumEverFreeHeapSize() = %u\n\n", xPortGetMinimumEverFreeHeapSize());
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "uxTaskGetStackHighWaterMark() = %lu\n\n", uxTaskGetStackHighWaterMark(NULL));
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);
		}
		osDelay(100);
	}
}




//************************************

// GUI_messages.c
/*

#include "GUI_messages.h"
#include <string.h>
#include "lwip.h"
#include "main.h"
#include "cmsis_os.h"

char GUI_buffer[GUI_BUFFER_SIZE] = {0};

extern UART_HandleTypeDef huart3;
extern struct netif gnetif;

void GUI_taskThread(void const * argument);

void GUI_start(void)
{
	  osThreadDef(GUI_task, GUI_taskThread, osPriorityNormal, 0, 500);
	  osThreadCreate(osThread(GUI_task), NULL);
}

void GUI_taskThread(void const * argument)
{
	while(1)
	{
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 1)
		{
			// print IP address
			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "IP = %lu\n\n", gnetif.ip_addr.addr);
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			// print freeRTOS statistics
			osThreadList((unsigned char*)GUI_buffer);
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "\nxPortGetFreeHeapSize() = %u\n\n", xPortGetFreeHeapSize());
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "xPortGetMinimumEverFreeHeapSize() = %u\n\n", xPortGetMinimumEverFreeHeapSize());
			HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);

			osDelay(500);
		}
		osDelay(100);
	}
}

void vApplicationStackOverflowHook( xTaskHandle xTask, signed char *pcTaskName )
{
	snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "Overflow detected in: %s\n", pcTaskName);
	HAL_UART_Transmit(&huart3, (unsigned char*)GUI_buffer, strlen(GUI_buffer) + 1, 200);
	osDelay(1000);
}

//  If sum of all defined (used?) stack sizes is more than configTOTAL_HEAP_SIZE it will cause problems!
// defined stack size is in words, configTOTAL_HEAP_SIZE is in bytes !

// https://www.freertos.org/uxTaskGetStackHighWaterMark.html  - check stack usage


// Call xPortGetFreeHeapSize(), create your tasks queues semaphores etc. then call xPortGetFreeHeapSize() again to find the difference. http://www.freertos.org/a00111.html
// Actually, there’s also  xPortGetMinimumEverFreeHeapSize() but only when using heap4
*/


