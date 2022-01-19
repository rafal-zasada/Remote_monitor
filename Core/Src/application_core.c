/*
 * application_core.c
 *
 *  Created on: Jan 19, 2022
 *      Author: Rafal
 */

//  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 200);
//  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

#include "cmsis_os.h"

osThreadId ApplicationCoreTaskHandle;

void application_core_task(void const *argument);

int CH1_setting = 3;
int CH2_setting = 4;
int CH3_setting = 5;
int Relay1_setting = 1;  // relay setting = its value
int Relay2_setting = 0;  // relay setting = its value

float voltage1 = 233;		// ADC1
float voltage2 = 223;		// ADC2
float voltage3 = 255;		// ADC3
float temperature1 = 40;	//
float temperature2 = 30;


void app_core_init(void)
{
	osThreadDef(application_core, application_core_task, osPriorityNormal, 0, 200);  // macro, not a function
	ApplicationCoreTaskHandle = osThreadCreate(osThread(application_core), NULL);
}

void application_core_task(void const *argument)
{





}
