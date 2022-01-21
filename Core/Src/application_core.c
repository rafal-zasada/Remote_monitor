/*
 * application_core.c
 *
 *  Created on: Jan 19, 2022
 *      Author: Rafal
 */

//  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 200);
//  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

#include "cmsis_os.h"
#include "stdio.h"
#include "application_core.h"

osThreadId ApplicationCoreTaskHandle;
osMailQId mailSettingsHandle;

void application_core_task(void const *argument);
void monitor_data_to_string(void);

int CH1_setting = 3;
int CH2_setting = 4;
int CH3_setting = 5;
int Relay1_setting = 1;  // relay setting = its value
int Relay2_setting = 0;  // relay setting = its value

float voltage1 = -0.922;		// ADC1
float voltage2 = -3.44;		// ADC2
float voltage3 = -4.5;		// ADC3
float temperature1 = -15.3;	//
float temperature2 = -17.7;

char voltage1_str[9] = "init";		// e.g. -99.8 V or 123 mV or -1.23 mV
char voltage2_str[9] = "init";
char voltage3_str[9] = "init";
char temperature1_str[6] = "init";	// e.g. 23.4 or -83.4 or 123.4
char temperature2_str[6] = "init";


void app_core_init(void)
{
	osThreadDef(application_core, application_core_task, osPriorityNormal, 0, 400);  // macro, not a function
	ApplicationCoreTaskHandle = osThreadCreate(osThread(application_core), NULL);
}

void application_core_task(void const *argument)
{
	osMailQDef(mailSettings, 1, settingsMailDataType);			// Normally created in sender thread. What if other task tries to send to this queue when it has been not initialised yet?
	mailSettingsHandle = osMailCreate(osMailQ(mailSettings), NULL);


	osEvent mailData;
	settingsMailDataType *newSettingsReceivedPtr;

	while(1)
	{
		monitor_data_to_string();

		voltage1 = voltage1 + 0.01;
		voltage2 = voltage2 + 0.05;
		voltage3 = voltage3 + 0.05;
		temperature1 = temperature1 + 1.4;
		temperature2 = temperature2 - 1.4;
		osDelay(500);


		mailData = osMailGet(mailSettingsHandle, 0);

		if(mailData.status == osEventMail)
		{
			newSettingsReceivedPtr = mailData.value.p;
			printf("\nQueue receiver = %s\n", newSettingsReceivedPtr->mailString);
			osMailFree(mailSettingsHandle, newSettingsReceivedPtr);
		}
	}
}

void monitor_data_to_string(void)
{
	// voltage1 to string
	if(voltage1 < 0 && voltage1 > -1)
		snprintf(voltage1_str, 9, "%0.2f mV", voltage1);
	else if(voltage1 <= -1 && voltage1 > -10)
		snprintf(voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 <= -10)
		snprintf(voltage1_str, 9, "%0.1f V", voltage1);
	else if(voltage1 < 1 && voltage1 >= 0)
		snprintf(voltage1_str, 9, "%0.2f mV", voltage1);
	else if(voltage1 >= 1 && voltage1 < 10)
		snprintf(voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 >= 10)
		snprintf(voltage1_str, 9, "%0.1f V", voltage1);
	else
		snprintf(voltage1_str, 9, "0.00 V");	// 0

	// voltage2 float to string
	if(voltage2 < 0 && voltage2 > -1)
		snprintf(voltage2_str, 9, "%0.2f mV", voltage2);
	else if(voltage2 <= -1 && voltage2 > -10)
		snprintf(voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 <= -10)
		snprintf(voltage2_str, 9, "%0.1f V", voltage2);
	else if(voltage2 < 1 && voltage2 >= 0)
		snprintf(voltage2_str, 9, "%0.2f mV", voltage2);
	else if(voltage2 >= 1 && voltage2 < 10)
		snprintf(voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 >= 10)
		snprintf(voltage2_str, 9, "%0.1f V", voltage2);
	else
		snprintf(voltage2_str, 9, "0.00 V");	// 0

	// voltag31 to string
	if(voltage3 < 0 && voltage3 > -1)
		snprintf(voltage3_str, 9, "%0.2f mV", voltage3);
	else if(voltage3 <= -1 && voltage3 > -10)
		snprintf(voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 <= -10)
		snprintf(voltage3_str, 9, "%0.1f V", voltage3);
	else if(voltage3 < 1 && voltage3 >= 0)
		snprintf(voltage3_str, 9, "%0.2f mV", voltage3);
	else if(voltage3 >= 1 && voltage3 < 10)
		snprintf(voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 >= 10)
		snprintf(voltage3_str, 9, "%0.1f V", voltage3);
	else
		snprintf(voltage3_str, 9, "0.00 V");	// 0
	;	// avoid copiler warning

	// temperature floats to strings
	snprintf(temperature1_str, 6, "%0.1f", temperature1);
	snprintf(temperature2_str, 6, "%0.1f", temperature2);
}
