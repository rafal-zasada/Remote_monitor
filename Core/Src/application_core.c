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
#include "string.h"
#include "stdlib.h"
#include "application_core.h"

osThreadId ApplicationCoreTaskHandle;
osMailQId mailSettingsHandle;

static void receive_settings_mail_and_parse(void);
void application_core_task(void const *argument);
void monitor_data_to_string(void);

// Data format for reading setting sent by client (settings only)
// First 3 characters - parameter
// Forth character - space
// Fifth, sixth, seventh character - parameter value
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

monitorValuesType monitorValues;

void app_core_init(void)
{
	osThreadDef(application_core, application_core_task, osPriorityNormal, 0, 400);  // macro, not a function
	ApplicationCoreTaskHandle = osThreadCreate(osThread(application_core), NULL);
}

void application_core_task(void const *argument)
{
	osMailQDef(mailSettings, 1, settingsMailDataType);			// Normally created in sender thread. What if other task tries to send to this queue when it has been not initialised yet?
	mailSettingsHandle = osMailCreate(osMailQ(mailSettings), NULL);

	while(1)
	{
		monitor_data_to_string();

		voltage1 = voltage1 + 0.01;
		voltage2 = voltage2 + 0.05;
		voltage3 = voltage3 + 0.05;
		temperature1 = temperature1 + 1.4;
		temperature2 = temperature2 - 1.4;
		osDelay(500);


		receive_settings_mail_and_parse();


	}
}

void monitor_data_to_string(void)
{
	// voltage1 to string
	if(voltage1 < 0 && voltage1 > -1)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f mV", voltage1);
	else if(voltage1 <= -1 && voltage1 > -10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 <= -10)
		snprintf(monitorValues.voltage1_str, 9, "%0.1f V", voltage1);
	else if(voltage1 < 1 && voltage1 >= 0)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f mV", voltage1);
	else if(voltage1 >= 1 && voltage1 < 10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 >= 10)
		snprintf(monitorValues.voltage1_str, 9, "%0.1f V", voltage1);
	else
		snprintf(monitorValues.voltage1_str, 9, "0.00 V");	// 0

	// voltage2 float to string
	if(voltage2 < 0 && voltage2 > -1)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f mV", voltage2);
	else if(voltage2 <= -1 && voltage2 > -10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 <= -10)
		snprintf(monitorValues.voltage2_str, 9, "%0.1f V", voltage2);
	else if(voltage2 < 1 && voltage2 >= 0)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f mV", voltage2);
	else if(voltage2 >= 1 && voltage2 < 10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 >= 10)
		snprintf(monitorValues.voltage2_str, 9, "%0.1f V", voltage2);
	else
		snprintf(monitorValues.voltage2_str, 9, "0.00 V");	// 0

	// voltag31 to string
	if(voltage3 < 0 && voltage3 > -1)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f mV", voltage3);
	else if(voltage3 <= -1 && voltage3 > -10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 <= -10)
		snprintf(monitorValues.voltage3_str, 9, "%0.1f V", voltage3);
	else if(voltage3 < 1 && voltage3 >= 0)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f mV", voltage3);
	else if(voltage3 >= 1 && voltage3 < 10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 >= 10)
		snprintf(monitorValues.voltage3_str, 9, "%0.1f V", voltage3);
	else
		snprintf(monitorValues.voltage3_str, 9, "0.00 V");	// 0
	;	// avoid copiler warning

	// temperature floats to strings
	snprintf(monitorValues.temperature1_str, 6, "%0.1f", temperature1);
	snprintf(monitorValues.temperature2_str, 6, "%0.1f", temperature2);
}

static void receive_settings_mail_and_parse(void)
{
	osEvent mailData;
	settingsMailDataType *newSettingsReceivedPtr;

	mailData = osMailGet(mailSettingsHandle, 0);

	if(mailData.status == osEventMail)
	{
		newSettingsReceivedPtr = mailData.value.p;
		printf("\nQueue receiver = %s\n", newSettingsReceivedPtr->mailString);

		char *p = newSettingsReceivedPtr->mailString;
		p = p + 4; // this is where setting number is supposed to start for CH1, CH2, CH3

		if(strncmp(newSettingsReceivedPtr->mailString, "CH1", 3) == 0)
		{
			CH1_setting = strtol(p, NULL, 10);
		}

		else if(strncmp(newSettingsReceivedPtr->mailString, "CH2", 3) == 0)
		{
			CH2_setting = strtol(p, NULL, 10);
		}

		if(strncmp(newSettingsReceivedPtr->mailString, "CH3", 3) == 0)
		{

			CH3_setting = strtol(p, NULL, 10);
		}




		if(strncmp(newSettingsReceivedPtr->mailString, "Re1", 3) == 0)
		{

			Relay1_setting = strtol(p, NULL, 10);
		}

		if(strncmp(newSettingsReceivedPtr->mailString, "Re2", 3) == 0)
		{

			Relay2_setting = strtol(p, NULL, 10);
		}






		osMailFree(mailSettingsHandle, newSettingsReceivedPtr);
	}

}




//			if(strncmp(parameter, "CH1", 3) == 0)
//				CH1_setting = strtol(parameter_value, &dummy_ptr, 10);
//			if(strncmp(parameter, "CH2", 3) == 0)
//				CH2_setting = strtol(parameter_value, &dummy_ptr, 10);
//			if(strncmp(parameter, "CH3", 3) == 0)
//				CH3_setting = strtol(parameter_value, &dummy_ptr, 10);
//			if(strncmp(parameter, "Re1", 3) == 0)
//				Relay1_setting = strtol(parameter_value, &dummy_ptr, 10);
//			if(strncmp(parameter, "Re2", 3) == 0)
//				Relay2_setting = strtol(parameter_value, &dummy_ptr, 10);

