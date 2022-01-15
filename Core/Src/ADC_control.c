/*
 * App_control_and_state.c
 *
 *  Created on: Jan 12, 2022
 *      Author: Rafal
 */
//#include "FreeRTOS.h"
#include "cmsis_os.h"

osThreadId appControlHandle;

void app_control_thread(void const *argument);

void init_ADC_scheduling(void)
{
	osThreadDef(app_control, app_control_thread, osPriorityNormal,0, 1024);
	appControlHandle = osThreadCreate(osThread(app_control), NULL);
}


