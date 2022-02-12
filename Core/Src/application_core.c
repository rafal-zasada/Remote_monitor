/*
 * application_core.c
 *
 *  Created on: Jan 19, 2022
 *      Author: Rafal
 */

// This file is handling client GUI settings and displayed values

#include "cmsis_os.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "application_core.h"
#include "adc.h"
#include "SSL_email.h"

void application_core_task(void const *argument);
static void read_monitor_values(void);
static void receive_settings_mail_and_parse(void);
static void monitor_data_to_string(void);
static void ADC_raw_to_voltage(void);

extern TIM_HandleTypeDef htim9;
extern struct emailDAtaReceipient newEmail;

osThreadId ApplicationCoreTaskHandle;
osMailQId mailSettingsHandle;

// Data format for reading setting sent by client (settings only)
// First 3 characters - parameter
// Forth character - space
// Fifth, sixth, seventh character - parameter value
#define ADC_FREE_RUN 0
#define ADC_TRIGGERED 1

#define WATCHDOG_DISABLED 0
#define WATCHDOG_ENABLED 1
#define WATCHDOG_TRIGGERED 2

#define RAISING_EDGE 1
#define FALLING_EDGE 2

int CH1_setting = ADC_FREE_RUN;
int CH2_setting = ADC_TRIGGERED;
int CH3_setting = ADC_FREE_RUN;
int Relay1_setting = 1;  // relay setting = its value in other parts of the source code
int Relay2_setting = 0;  // relay setting = its value in other parts of the source code
int pulseMeasurementDelay = 3;
int watchdogState = 1;
int watchdogChannel = 2;
int watchdogTriggerEdge = RAISING_EDGE;
float watchdogThreshold = 23.4;
//int watchdogUnits = 2;	//	not used - to be removed
int watchdogAction1 = 3;
int watchdogAction2 = 4;

int voltage1_raw;
int voltage2_raw;
int voltage3_raw;
float voltage1 = -0.922;	// ADC1
float voltage2 = -3.44;		// ADC2
float voltage3 = -4.5;		// ADC3
float temperature1 = -15.3;
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

	HAL_ADC_Start(&hadc1); // start ADC (in trigger mode (software ACD trigger in ISR for GPIO_EXTI) it will be started again but this one includes all checks for proper operation of ADC)
	HAL_ADC_Start(&hadc2);
	HAL_ADC_Start(&hadc3);

	HAL_TIM_Base_Start(&htim9);	// for delay of pulse measurement

	strncpy(newEmail.emailRecipient, "zasi@poczta.onet.pl", 40); // temporary for test
	strncpy(newEmail.emailSubject, "Naprawde nowy email", 40); // temporary for test
	strncpy(newEmail.emailBody, "Tresc majla, jol :)\n", 40); // temporary for test

	while(1)
	{
		osDelay(50);

		receive_settings_mail_and_parse();
		read_monitor_values();

		if(CH1_setting == ADC_TRIGGERED || CH2_setting == ADC_TRIGGERED || CH3_setting == ADC_TRIGGERED)
		{
			// if trigger is connected and running then interrupt flag will be already waiting - just cycle through it once to avoid unnecessary interrupts
			HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
			HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
		}

		ADC_raw_to_voltage();
		monitor_data_to_string();
	}
}

static void monitor_data_to_string(void)
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

	mailData = osMailGet(mailSettingsHandle, 0);	// no waiting

	if(mailData.status == osEventMail)
	{
		newSettingsReceivedPtr = mailData.value.p;	// memory alloc'd in sender will have to be free'd here in receiver

		printf("\nQueue receiver = %s\n", newSettingsReceivedPtr->mailString);

		char *receivedMessagePtr = newSettingsReceivedPtr->mailString;

		// message format for CH1, CH2, CH3, Re1, Re2, DEL settings:
		// - first three letters are paramater
		// - space
		// - numerical value
		// e.g.   CH1 1

		if(strncmp(receivedMessagePtr, "CH1", 3) == 0)
		{
			CH1_setting = strtol(receivedMessagePtr + 4 , NULL, 10);
		}

		else if(strncmp(receivedMessagePtr, "CH2", 3) == 0)
		{
			CH2_setting = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "CH3", 3) == 0)
		{
			CH3_setting = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "Re1", 3) == 0)
		{
			Relay1_setting = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "Re2", 3) == 0)
		{
			Relay2_setting = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "DEL", 3) == 0)
		{
			pulseMeasurementDelay = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		// message format for watchdog settings:
		// e.g.  WAT SET 1 means enable watchdog

		if(strncmp(receivedMessagePtr, "WAT", 3) == 0)				// watchdog settings
		{
			if(strncmp(receivedMessagePtr + 4, "SET", 3) == 0)
			{
				watchdogState = strtol(receivedMessagePtr + 8, NULL, 10);
			}

			if(strncmp(receivedMessagePtr + 4, "OPT", 3) == 0)		// watchdog options - SAVE button on website pressed
			{
				// (0)WAT   (4)OPT   (8)0      (10)0    (12)123.44  (19)0   (21)0   (23)0     (25)zasi@poczta.onet.pl  (position in the string in brackets)
				// watchdog options channel above/below   value     units  action1  action2         email

				watchdogChannel = strtol(receivedMessagePtr + 8, NULL, 10);
				watchdogTriggerEdge = strtol(receivedMessagePtr + 10, NULL, 10);
				watchdogThreshold = strtof(receivedMessagePtr + 12, NULL);	// note: string to float
//				watchdogUnits = strtol(receivedMessagePtr + 19, NULL, 10);
				watchdogAction1 = strtol(receivedMessagePtr + 21, NULL, 10);
				watchdogAction2 = strtol(receivedMessagePtr + 23, NULL, 10);
				strncpy(newEmail.emailRecipient, receivedMessagePtr + 25, 45); // at the moment max email length set to 45

				printf("watchdogChannel = %d\n", watchdogChannel);
				printf("watchdogAboveBelow = %d\n", watchdogTriggerEdge);
				printf("watchdogThreshold = %f\n", watchdogThreshold);
//				printf("watchdogUnits = %d\n", watchdogUnits);
				printf("watchdogAction1 = %d\n", watchdogAction1);
				printf("watchdogAction2 = %d\n", watchdogAction2);
				printf("newEmail.emailReceipient = %s\n", newEmail.emailRecipient);

			}
			printf("watchdogStatus = %d\n", watchdogState);
		}

		if(strncmp(receivedMessagePtr, "TES", 3) == 0)
		{
			extern  osThreadId send_SSL_emailTaskHandle;
			char testEmailBody[EMAIL_BODY_MAX_SIZE] = {0};
			char watchdogUnitsString[2]; 	// this is just for email (null termination required for snprintf)
			char watchdogStateString[10];
			char watchdogTriggerEdgeString[6];
			char watchdogAction1String[30];
			char watchdogAction2String[30];

			if(watchdogChannel == 1 || watchdogChannel == 2 || watchdogChannel == 3)
				strncpy(watchdogUnitsString, "V", 2);
			else
				strncpy(watchdogUnitsString, "C", 2);

			if(watchdogState == WATCHDOG_DISABLED)
				strncpy(watchdogStateString, "DISABLED", 10);
			else if(watchdogState == WATCHDOG_ENABLED)
				strncpy(watchdogStateString, "ENABLED", 10);
			else if(watchdogState == WATCHDOG_TRIGGERED)
				strncpy(watchdogStateString, "TRIGGERED",10);

			if(watchdogTriggerEdge == RAISING_EDGE)
				strncpy(watchdogTriggerEdgeString, "above", 6);
			else if(watchdogTriggerEdge == FALLING_EDGE)
				strncpy(watchdogTriggerEdgeString, "below", 6);


			// from website
			// <option value="0">no action</option>
			// <option value="1">send email</option>
			// <option value="2">open Relay 1</option>
			// <option value="3">open Relay 2</option>
			// <option value="4">close Relay 1</option>
			// <option value="5">close Relay 2</option>
			// <option value="6">send email + open Relay 1</option>
			// <option value="7">wait 60s + open Relay 2</option>
			if(watchdogAction1 == 0)
				strncpy(watchdogAction1String, "no action", 30);
			else if(watchdogAction1 == 1)
				strncpy(watchdogAction1String, "send email", 30);
			else if(watchdogAction1 == 2)
				strncpy(watchdogAction1String, "open Relay 1", 30);
			else if(watchdogAction1 == 3)
				strncpy(watchdogAction1String, "open Relay 2", 30);
			else if(watchdogAction1 == 4)
				strncpy(watchdogAction1String, "close Relay 1", 30);
			else if(watchdogAction1 == 5)
				strncpy(watchdogAction1String, "close Relay 2", 30);
			else if(watchdogAction1 == 6)
				strncpy(watchdogAction1String, "send email + open Relay 1", 30);
			else if(watchdogAction1 == 7)
				strncpy(watchdogAction1String, "wait 60s + open Relay 2", 30);

			if(watchdogAction2 == 0)
				strncpy(watchdogAction2String, "no action", 30);
			else if(watchdogAction2 == 1)
				strncpy(watchdogAction2String, "send email", 30);
			else if(watchdogAction2 == 2)
				strncpy(watchdogAction2String, "open Relay 1", 30);
			else if(watchdogAction2 == 3)
				strncpy(watchdogAction2String, "open Relay 2", 30);
			else if(watchdogAction2 == 4)
				strncpy(watchdogAction2String, "close Relay 1", 30);
			else if(watchdogAction2 == 5)
				strncpy(watchdogAction2String, "close Relay 2", 30);
			else if(watchdogAction2 == 6)
				strncpy(watchdogAction2String, "send email + open Relay 1", 30);
			else if(watchdogAction2 == 7)
				strncpy(watchdogAction2String, "wait 60s + open Relay 2", 30);

			strncpy(newEmail.emailRecipient, receivedMessagePtr + 4, EMAIL_RECIPIENT_MAX_LENGH);
			strncpy(newEmail.emailSubject, "This is test email from Monitor1", EMAIL_SUBJECT_MAX_LENGH);

			snprintf(testEmailBody, 400, "CH 1 voltage = %s\n"
									     "CH 2 voltage = %s\n"
										 "CH 3 voltage = %s\n"
										 "TC 1 temperature = %s C\n"
										 "TC 1 temperature = %s C\n\n"
										 "Watchdog state = %s\n"
										 "Watchdog settings:\n"
										 "If CH%d is %s %f %s then %s and %s\n"
										 "Email for notifications: %s\n",
										 monitorValues.voltage1_str, monitorValues.voltage2_str, monitorValues.voltage3_str, monitorValues.temperature1_str,
										 monitorValues.temperature2_str, watchdogStateString, watchdogChannel, watchdogTriggerEdgeString, watchdogThreshold, watchdogUnitsString,
										 watchdogAction1String, watchdogAction2String, newEmail.emailRecipient);


			strncpy(newEmail.emailBody, testEmailBody, EMAIL_BODY_MAX_SIZE);
			osSignalSet(send_SSL_emailTaskHandle, 1); // send email
		}

		osMailFree(mailSettingsHandle, newSettingsReceivedPtr);
	}
}

static void read_monitor_values(void)
{
	if(CH1_setting == ADC_FREE_RUN)	// ADC in free run mode
	{
		HAL_ADC_Start(&hadc1);

		if(HAL_ADC_PollForConversion(&hadc1, 20) == 0)
		{
			voltage1_raw = HAL_ADC_GetValue(&hadc1);
		}
	}
	else
	{
		// voltage1_raw will be updated by interrupt
	}

	if(CH2_setting == ADC_FREE_RUN)	// ADC in free run mode
	{
		HAL_ADC_Start(&hadc2);

		if(HAL_ADC_PollForConversion(&hadc2, 20) == 0)
		{
			voltage2_raw = HAL_ADC_GetValue(&hadc2);
		}
	}
	else
	{
		// voltage1_raw will be updated by interrupt
	}

	if(CH3_setting == ADC_FREE_RUN)	// ADC in free run mode
	{
		HAL_ADC_Start(&hadc3);

		if(HAL_ADC_PollForConversion(&hadc3, 20) == 0)
		{
			voltage3_raw = HAL_ADC_GetValue(&hadc3);
		}
	}
	else
	{
		// voltage1_raw will be updated by interrupt
	}




	// read temperatures here - to be implemented

}

static void ADC_raw_to_voltage(void)
{
	// to be implemented

	// for test
	voltage1 = voltage1_raw;
	voltage2 = voltage2_raw;
	voltage3 = voltage3_raw;

}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)	// interrupt used only for pulse measurements (ADC triggered by external signal)
{
//	bug: if interrupt is triggered before TIM9 is running then application will freeze

	printf("\nInterrupt triggered by GPIO\n");

	uint16_t timerValue = pulseMeasurementDelay * 217 - 110;

	TIM9->ARR = timerValue;		// set timer auto reload value
	TIM9->CNT &= 0xFFFF0000;	// reset counter to 0
	TIM9->SR &= !TIM_SR_UIF;	// clear update interrupt flag
	while(!(TIM9->SR && TIM_SR_UIF));	// wait for update interrupt flag

//	GPIOG->BSRR = GPIO_PIN_0 << 16;		// reset pin, 3 clock cycles

	//	  // minimal version for ADC operation extracted from HAL_ADC_Start(&hadc1) and HAL_ADC_PollForConversion(&hadc1, 2000) (HAL_ADC_Start() must run once before)
	//	  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_EOC | ADC_FLAG_OVR); 	  // Clear regular group conversion flag and overrun flag (To ensure of no unknown state from potential previous ADC operations)
	//	  (&hadc1)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART; 	  // Enable the selected ADC software conversion for regular group
	//	  while(!(__HAL_ADC_GET_FLAG((&hadc1), ADC_FLAG_EOC)));		 // Check End of conversion flag  (wait indefinitely)
	//	  voltage1_raw = HAL_ADC_GetValue(&hadc1);
	//	  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_STRT | ADC_FLAG_EOC); // Clear regular group conversion flag

	// start conversion
	if(CH1_setting != ADC_FREE_RUN)
	{
	  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_EOC | ADC_FLAG_OVR);
	  (&hadc1)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}
	if(CH2_setting != ADC_FREE_RUN)
	{
		  __HAL_ADC_CLEAR_FLAG((&hadc2), ADC_FLAG_EOC | ADC_FLAG_OVR);
		  (&hadc2)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}
	if(CH3_setting != ADC_FREE_RUN)
	{
		  __HAL_ADC_CLEAR_FLAG((&hadc3), ADC_FLAG_EOC | ADC_FLAG_OVR);
		  (&hadc3)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}

	// wait for EOC flag and read value
	if(CH1_setting != ADC_FREE_RUN)
	{
		  while(!(__HAL_ADC_GET_FLAG((&hadc1), ADC_FLAG_EOC)));
		  voltage1_raw = HAL_ADC_GetValue(&hadc1);
		  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}
	if(CH2_setting != ADC_FREE_RUN)
	{
		  while(!(__HAL_ADC_GET_FLAG((&hadc2), ADC_FLAG_EOC)));
		  voltage2_raw = HAL_ADC_GetValue(&hadc2);
		  __HAL_ADC_CLEAR_FLAG((&hadc2), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}
	if(CH3_setting != ADC_FREE_RUN)
	{
		  while(!(__HAL_ADC_GET_FLAG((&hadc3), ADC_FLAG_EOC)));
		  voltage3_raw = HAL_ADC_GetValue(&hadc3);
		  __HAL_ADC_CLEAR_FLAG((&hadc3), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}
}

