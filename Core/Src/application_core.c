/*
 * application_core.c
 *
 *  Created on: Jan 19, 2022
 *      Author: Rafal
 */

// This file is handling client GUI settings, acquires input data, formats values for GUI and handles watchdog (user watchdog)

#include "cmsis_os.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "application_core.h"
#include "adc.h"
#include "SSL_email.h"

void application_core_task(void const *argument);
static void read_monitor_values(void);
static void processClientInstruction(void);
static void monitor_data_to_string(void);
static void ADC_raw_to_voltage_and_averaging(void);
static void ADC_range_selection(void);
static void checkWatchdogTriggers(void);
static void ExecuteWatchdogActions(int triggeringChannel);
static void osTimerCallback(void const *argument);
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
#define RANGE_2V 0
#define RANGE_40V 1

int ADC1_range = RANGE_40V;
int ADC2_range = RANGE_40V;
int ADC3_range = RANGE_40V;
static int voltage1_raw; // ADC1 reading
static int voltage2_raw; // ADC2 reading
static int voltage3_raw; // ADC3 reading
float voltage1;
float voltage2;
float voltage3;
float temperature_TC1;
float temperature_TC2;
int CH1_mode = ADC_FREE_RUN;
int CH2_mode = ADC_FREE_RUN;
int CH3_mode = ADC_FREE_RUN;
int switch1_setting = 1;
int switch2_setting = 1;
int pulseMeasurementDelay = 3;

// variables for averaging
static float CH1_buffer[5];
static float CH2_buffer[5];
static float CH3_buffer[5];
static int bufferPosition;

#define WATCHDOG_DISABLED 0
#define WATCHDOG_ENABLED 1
#define WATCHDOG_TRIGGERED 2
#define WATCHDOG_CHANNEL_CH1 1
#define WATCHDOG_CHANNEL_CH2 2
#define WATCHDOG_CHANNEL_CH3 3
#define WATCHDOG_CHANNEL_TC1 4
#define WATCHDOG_CHANNEL_TC2 5
#define UPWARD 1
#define DOWNWORD 2
#define OPEN_SWITCH1 11
#define OPEN_SWITCH2 21
#define CLOSE_SWITCH1 10
#define CLOSE_SWITCH2 20

int watchdogState = WATCHDOG_DISABLED;
int watchdogChannel = WATCHDOG_CHANNEL_CH1;
int watchdogTriggerDirection = UPWARD;
float watchdogThreshold = 100;
//int watchdogUnits = 2;	//	not used - to be removed
int watchdogAction1 = 3;
int watchdogAction2 = 4;
int timerCallbackArgument;
monitorValuesType monitorValues;

void app_core_init(void)
{
	osThreadDef(application_core, application_core_task, osPriorityNormal, 0, 800);  // macro, not a function
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

	// ADC initial input pin selection (range)
	  ADC_ChannelConfTypeDef sConfig = {0};

	  sConfig.Channel = ADC_CHANNEL_5;
	  sConfig.Rank = ADC_REGULAR_RANK_1;
	  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
	  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  ADC_ChannelConfTypeDef sConfig_2 = {0};

	  sConfig_2.Channel = ADC_CHANNEL_13;
	  sConfig_2.Rank = ADC_REGULAR_RANK_1;
	  sConfig_2.SamplingTime = ADC_SAMPLETIME_15CYCLES;
	  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig_2) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  ADC_ChannelConfTypeDef sConfig_3 = {0};

	  sConfig_3.Channel = ADC_CHANNEL_14;
	  sConfig_3.Rank = ADC_REGULAR_RANK_1;
	  sConfig_3.SamplingTime = ADC_SAMPLETIME_15CYCLES;
	  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig_3) != HAL_OK)
	  {
	    Error_Handler();
	  }

	while(1)
	{
		osDelay(100);
		processClientInstruction();
		ADC_range_selection();
		read_monitor_values();
		ADC_raw_to_voltage_and_averaging();
		monitor_data_to_string();
		checkWatchdogTriggers();

//		printf("raw= %d   v= %0.2f  r= %d\n", voltage3_raw, voltage3, ADC3_range);


	//	move to to function process client instruction ???
		if(CH1_mode == ADC_TRIGGERED || CH2_mode == ADC_TRIGGERED || CH3_mode == ADC_TRIGGERED)
		{
			HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
			HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
		}
		else
		{
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
		}

	}
}

static void ADC_range_selection(void)
{
	ADC_ChannelConfTypeDef sConfig = {0};

//	printf("ADC1 raw avg. = %d\n", voltage1_raw);
//	printf("ADC1 range before = %d\n", ADC1_range);

	// ADC1
	if((voltage1 > 0.950 || voltage1 < -0.950) && (ADC1_range == RANGE_2V)) // switch to high range required?
	{
		sConfig.Channel = ADC_CHANNEL_5;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			Error_Handler();

		ADC1_range = RANGE_40V;
	}
	else if((voltage1 < 0.950 && voltage1 > -0.950) && (ADC1_range == RANGE_40V))
	{
		sConfig.Channel = ADC_CHANNEL_4;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			Error_Handler();

		ADC1_range = RANGE_2V;
	}

	// ADC2
	if((voltage2 > 0.950 || voltage2 < -0.950) && (ADC2_range == RANGE_2V)) // switch to high range required?
	{
		sConfig.Channel = ADC_CHANNEL_13;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
			Error_Handler();

		ADC2_range = RANGE_40V;
	}
	else if((voltage2 < 0.950 && voltage2 > -0.950) && (ADC2_range == RANGE_40V)) // switch to low range required?
	{
		sConfig.Channel = ADC_CHANNEL_12;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
			Error_Handler();

		ADC2_range = RANGE_2V;
	}

	// ADC3
	if((voltage3 > 0.950 || voltage3 < -0.950) && (ADC3_range == RANGE_2V)) // switch to high range required?
	{
		sConfig.Channel = ADC_CHANNEL_14;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
			Error_Handler();

		ADC3_range = RANGE_40V;

		osDelay(1); // stabilise ?
	}
	else if((voltage3 < 0.950 && voltage3 > -0.950) && (ADC3_range == RANGE_40V)) // switch to low range required?
	{
		sConfig.Channel = ADC_CHANNEL_9;
		sConfig.Rank = ADC_REGULAR_RANK_1;
		sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

		if(HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
			Error_Handler();

		ADC3_range = RANGE_2V;
	}

//	printf("ADC1 range after = %d\n", ADC1_range);
}

static void read_monitor_values(void)
{
	if(CH1_mode == ADC_FREE_RUN)	// ADC in free run mode
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

	if(CH2_mode == ADC_FREE_RUN)	// ADC in free run mode
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

	if(CH3_mode == ADC_FREE_RUN)	// ADC in free run mode
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


	//debug
//	printf("voltage3 raw = %d\n", voltage3_raw);
//	printf("ADC3 range = %d\n", ADC3_range);


	// read temperatures

	extern SPI_HandleTypeDef hspi3;
	extern SPI_HandleTypeDef hspi4;
	unsigned char test_SPI[4];
	int data_in;
	int TC_temperature_raw = 0;
	int TC1_internal_temperature_raw = 0; // to be used for temperature adjustment
	int TC2_internal_temperature_raw = 0;
	unsigned int TC1_error = 0;
	unsigned int TC2_error = 0;
	unsigned int TC1_open = 0;
	unsigned int TC2_open = 0;


	// reading TC1 temperature
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);	// chip select ON for TC1
	HAL_SPI_Receive(&hspi3, test_SPI, 4, 500);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);		// chip select OFF for TC1

	data_in = 0;
	data_in |= test_SPI[3] << 0;
	data_in |= test_SPI[2] << 8;
	data_in |= test_SPI[1] << 16;
	data_in |= test_SPI[0] << 24;
	TC1_error |= (data_in & 65536u);	// bit 16 - MAX31855 reading error
	TC1_open |= (data_in & 1u);			// bit 0 - MAX31855 thermocouple open circuit


	if(!TC1_error)
	{
		TC1_internal_temperature_raw = (data_in >> 4) & 4095u;
		TC_temperature_raw = data_in >> 18;
		temperature_TC1 = (float)TC_temperature_raw / 4; // - TC1_internal_temperature_raw / 16 + 25;

//		printf("TC1 temparature = %0.2f\n", temperature_TC1);
//		printf("TC1 Internal temperature = %d\n\n", TC1_internal_temperature_raw / 16);
	}
	else if(TC1_open)
		temperature_TC1 = -888; // code for TC open circuit
	else
		temperature_TC1 = -999; // code for TC error




	// reading TC2 temperature
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_RESET);	// chip select ON for TC2
	HAL_Delay(1);
	HAL_SPI_Receive(&hspi4, test_SPI, 4, 500);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_SET);		// chip select OFF for TC2

	// re-use temporary variables
	data_in = 0;
	data_in |= test_SPI[3] << 0;
	data_in |= test_SPI[2] << 8;
	data_in |= test_SPI[1] << 16;
	data_in |= test_SPI[0] << 24;
	TC2_error |= (data_in & 65536u);	// bit 16 - MAX31855 reading error
	TC2_open |= (data_in & 1u);			// bit 0 - MAX31855 thermocouple open circuit

//	printf("TC2 data= %d\n", data_in);

	if(!TC2_error)
	{
		TC2_internal_temperature_raw = (data_in >> 4) & 4095u;
		TC_temperature_raw = data_in >> 18;
		temperature_TC2 = (float)TC_temperature_raw / 4; // - TC2_internal_temperature_raw / 16 + 25;

//		printf("TC2 temparature = %0.2f\n", temperature_TC2);
//		printf("TC2 Internal temperature = %d\n\n", TC2_internal_temperature_raw / 16);
	}
	else if(TC2_open)
		temperature_TC2 = -888; // code for TC open circuit
	else
		temperature_TC2 = -999; // code for TC error


}

static void ADC_raw_to_voltage_and_averaging(void)
{
	if(ADC1_range == RANGE_2V)
		CH1_buffer[bufferPosition] = (float)2 / 4095 * voltage1_raw - 1;
	else // RANGE_40V
		CH1_buffer[bufferPosition] = (float)40 / 4095 * voltage1_raw - 20;

	if(ADC2_range == RANGE_2V)
		CH2_buffer[bufferPosition] = (float)2 / 4095 * voltage2_raw - 1;
	else // RANGE_40V
		CH2_buffer[bufferPosition] = (float)40 / 4095 * voltage2_raw - 20;

	if(ADC3_range == RANGE_2V)
		CH3_buffer[bufferPosition] = (float)2 / 4095 * voltage3_raw - 1;
	else // RANGE_40V
		CH3_buffer[bufferPosition] = (float)40 / 4095 * voltage3_raw - 20;
	
	bufferPosition++;
	if(bufferPosition == 5)
		bufferPosition = 0;
	
	// calibration factor = 1.0227
	voltage1 = (CH1_buffer[0] + CH1_buffer[1] + CH1_buffer[2] + CH1_buffer[3] + CH1_buffer[4]) / 5 * 1.0227;
	voltage2 = (CH2_buffer[0] + CH2_buffer[1] + CH2_buffer[2] + CH2_buffer[3] + CH2_buffer[4]) / 5 * 1.0227;
	voltage3 = (CH3_buffer[0] + CH3_buffer[1] + CH3_buffer[2] + CH3_buffer[3] + CH3_buffer[4]) / 5 * 1.0227;
}

static void monitor_data_to_string(void)
{
	// voltage1 to string
	if(voltage1 < 0 && voltage1 > -1)
		snprintf(monitorValues.voltage1_str, 9, "%0.0f mV", voltage1 * 1000);
	else if(voltage1 <= -1 && voltage1 > -10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 <= -10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 < 1 && voltage1 >= 0)
		snprintf(monitorValues.voltage1_str, 9, "%0.0f mV", voltage1 * 1000);
	else if(voltage1 >= 1 && voltage1 < 10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else if(voltage1 >= 10)
		snprintf(monitorValues.voltage1_str, 9, "%0.2f V", voltage1);
	else
		snprintf(monitorValues.voltage1_str, 9, "0.00 V");	// 0

	// voltage2 float to string
	if(voltage2 < 0 && voltage2 > -1)
		snprintf(monitorValues.voltage2_str, 9, "%0.0f mV", voltage2 * 1000);
	else if(voltage2 <= -1 && voltage2 > -10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 <= -10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 < 1 && voltage2 >= 0)
		snprintf(monitorValues.voltage2_str, 9, "%0.0f mV", voltage2 * 1000);
	else if(voltage2 >= 1 && voltage2 < 10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else if(voltage2 >= 10)
		snprintf(monitorValues.voltage2_str, 9, "%0.2f V", voltage2);
	else
		snprintf(monitorValues.voltage2_str, 9, "0.00 V");	// 0

	// voltag3 to string
	if(voltage3 < 0 && voltage3 > -1)
		snprintf(monitorValues.voltage3_str, 9, "%0.0f mV", voltage3 * 1000);
	else if(voltage3 <= -1 && voltage3 > -10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 <= -10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 < 1 && voltage3 >= 0)
		snprintf(monitorValues.voltage3_str, 9, "%0.0f mV", voltage3 * 1000);
	else if(voltage3 >= 1 && voltage3 < 10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else if(voltage3 >= 10)
		snprintf(monitorValues.voltage3_str, 9, "%0.2f V", voltage3);
	else
		snprintf(monitorValues.voltage3_str, 9, "0.00 V");	// 0
	;	// avoid copiler warning

	// temperature floats to strings
	snprintf(monitorValues.temperature1_str, 7, "%0.2f", temperature_TC1);
	snprintf(monitorValues.temperature2_str, 7, "%0.2f", temperature_TC2);
}

static void processClientInstruction(void)
{
	osEvent mailData; // (RTOS mail)
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
			CH1_mode = strtol(receivedMessagePtr + 4 , NULL, 10);
		}

		else if(strncmp(receivedMessagePtr, "CH2", 3) == 0)
		{
			CH2_mode = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "CH3", 3) == 0)
		{
			CH3_mode = strtol(receivedMessagePtr + 4, NULL, 10);
		}

		if(strncmp(receivedMessagePtr, "Re1", 3) == 0)
		{
			switch1_setting = strtol(receivedMessagePtr + 4, NULL, 10);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, !switch1_setting);
		}

		if(strncmp(receivedMessagePtr, "Re2", 3) == 0)
		{
			switch2_setting = strtol(receivedMessagePtr + 4, NULL, 10);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, !switch2_setting);
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
				watchdogTriggerDirection = strtol(receivedMessagePtr + 10, NULL, 10);
				watchdogThreshold = strtof(receivedMessagePtr + 12, NULL);	// note: string to float
//				watchdogUnits = strtol(receivedMessagePtr + 19, NULL, 10);
				watchdogAction1 = strtol(receivedMessagePtr + 21, NULL, 10);
				watchdogAction2 = strtol(receivedMessagePtr + 23, NULL, 10);
				strncpy(newEmail.emailRecipient, receivedMessagePtr + 25, 45); // at the moment max email length set to 45

				printf("watchdogChannel = %d\n", watchdogChannel);
				printf("watchdogAboveBelow = %d\n", watchdogTriggerDirection);
				printf("watchdogThreshold = %f\n", watchdogThreshold);
//				printf("watchdogUnits = %d\n", watchdogUnits);
				printf("watchdogAction1 = %d\n", watchdogAction1);
				printf("watchdogAction2 = %d\n", watchdogAction2);
				printf("newEmail.emailReceipient = %s\n", newEmail.emailRecipient);

			}
			printf("watchdogStatus = %d\n", watchdogState);
		}

		// if message contains test e-mail request handle it here
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

			if(watchdogTriggerDirection == UPWARD)
				strncpy(watchdogTriggerEdgeString, "above", 6);
			else if(watchdogTriggerDirection == DOWNWORD)
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

			if((int)temperature_TC1 == -888)	// thermocouple disconnected code
			{
				snprintf(monitorValues.temperature1_str, 9, "N/A");
			}

			if((int)temperature_TC2 == -888)	// thermocouple disconnected code
			{
				snprintf(monitorValues.temperature2_str, 9, "N/A");
			}

			strncpy(newEmail.emailRecipient, receivedMessagePtr + 4, EMAIL_RECIPIENT_MAX_LENGH);
			strncpy(newEmail.emailSubject, "This is test email from Monitor1", EMAIL_SUBJECT_MAX_LENGH);

			snprintf(testEmailBody, 400, "CH 1 voltage = %s\n"
									     "CH 2 voltage = %s\n"
										 "CH 3 voltage = %s\n"
										 "TC 1 temperature = %s C\n"
										 "TC 2 temperature = %s C\n\n"
										 "Watchdog state = %s\n"
										 "Watchdog settings:\n"
										 "If CH%d is %s %0.2f %s then %s and %s\n"
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

static void checkWatchdogTriggers(void)
{
	if(watchdogState == WATCHDOG_ENABLED)
	{
		if(watchdogChannel == WATCHDOG_CHANNEL_CH1)
		{
			if(watchdogTriggerDirection == UPWARD)
			{
				if(voltage1 > watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH1);
				}
			}
			else if(watchdogTriggerDirection == DOWNWORD)
			{
				if(voltage1 < watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH1);
				}
			}
		}
		else if(watchdogChannel == WATCHDOG_CHANNEL_CH2)
		{
			if(watchdogTriggerDirection == UPWARD)
			{
				if(voltage2 > watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH2);
				}
			}
			else if(watchdogTriggerDirection == DOWNWORD)
			{
				if(voltage2 < watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH2);
				}
			}
		}
		else if(watchdogChannel == WATCHDOG_CHANNEL_CH3)
		{
			if(watchdogTriggerDirection == UPWARD)
			{
				if(voltage3 > watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH3);
				}
			}
			else if(watchdogTriggerDirection == DOWNWORD)
			{
				if(voltage3 < watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_CH3);
				}
			}
		}
		else if(watchdogChannel == WATCHDOG_CHANNEL_TC1)
		{
			if(watchdogTriggerDirection == UPWARD)
			{
				if(temperature_TC1 > watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_TC1);
				}
			}
			else if(watchdogTriggerDirection == DOWNWORD)
			{
				if(temperature_TC1 < watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_TC1);
				}
			}
		}
		else if(watchdogChannel == WATCHDOG_CHANNEL_TC2)
		{
			if(watchdogTriggerDirection == UPWARD)
			{
				if(temperature_TC2 > watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_TC2);
				}
			}
			else if(watchdogTriggerDirection == DOWNWORD)
			{
				if(temperature_TC2 < watchdogThreshold)
				{
					ExecuteWatchdogActions(WATCHDOG_CHANNEL_TC2);
				}
			}
		}
	}
}

static void ExecuteWatchdogActions(int triggeringChannel)
{
	// from website
	// <option value="0">no action</option>
	// <option value="1">send email</option>
	// <option value="2">open Relay 1</option>
	// <option value="3">open Relay 2</option>
	// <option value="4">close Relay 1</option>
	// <option value="5">close Relay 2</option>
	// <option value="6">send email + open Relay 1</option>
	// <option value="7">wait 60s + open Relay 2</option>

	#define SEND_EMAIL 		1
	#define OPEN_SWITCH_1 	2
	#define OPEN_SWITCH_2 	3
	#define CLOSE_SWITCH_1	4
	#define CLOSE_SWITCH_2	5
	#define SEND_EMAIL_OPEN_SWITCH_1   6
	#define WAIT_60S_OPEN_SWITCH_2	  7

	if(watchdogState == WATCHDOG_TRIGGERED)
	{
		return; // no action if already triggered once (and not cleared)
	}

	printf("Test inside ExecuteWatchdogActions \n ");
	printf("Action 1 = %d\n", watchdogAction1);
	printf("Action 2 = %d\n", watchdogAction2);

	extern  osThreadId send_SSL_emailTaskHandle;
	char WatchdogEmailBody[EMAIL_BODY_MAX_SIZE] = {0};
	char watchdogEmailSubject[EMAIL_SUBJECT_MAX_LENGH] = {0};

	// email always prepared but for some options it is not sent

	if(triggeringChannel == WATCHDOG_CHANNEL_CH1)
		snprintf(watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH, "CH 1 voltage outside limit!");

	if(triggeringChannel == WATCHDOG_CHANNEL_CH2)
		snprintf(watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH, "CH 2 voltage outside limit!");

	if(triggeringChannel == WATCHDOG_CHANNEL_CH3)
		snprintf(watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH, "CH 3 voltage outside limit!");

	if(triggeringChannel == WATCHDOG_CHANNEL_TC1)
		snprintf(watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH, "TC 1 temperature outside limit!");

	if(triggeringChannel == WATCHDOG_CHANNEL_TC2)
		snprintf(watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH, "TC 2 temperature outside limit!");

//	printf("TC1 = %0.2f\n", temperature_TC1);

	if((int)temperature_TC1 == -888)	// thermocouple disconnected (update for email)
	{
		snprintf(monitorValues.temperature1_str, 9, "N/A");
		printf("Executed N/A\n");
	}


	if((int)temperature_TC2 == -888)  // thermocouple disconnected (update for email)
		snprintf(monitorValues.temperature2_str, 9, "N/A");


	snprintf(WatchdogEmailBody, EMAIL_BODY_MAX_SIZE, "CH 1 voltage = %s\n"
										     "CH 2 voltage = %s\n"
											 "CH 3 voltage = %s\n"
											 "TC 1 temperature = %s C\n"
											 "TC 2 temperature = %s C\n\n"
											 "Watchdog was been disabled following this message. To re-enable watchdog visit website http://monitor1\n",
											 monitorValues.voltage1_str, monitorValues.voltage2_str, monitorValues.voltage3_str,
											 monitorValues.temperature1_str, monitorValues.temperature2_str);

	strncpy(newEmail.emailBody, WatchdogEmailBody, EMAIL_BODY_MAX_SIZE);
	strncpy(newEmail.emailSubject, watchdogEmailSubject, EMAIL_SUBJECT_MAX_LENGH);

	// action 1
	if(watchdogAction1 == SEND_EMAIL)
		osSignalSet(send_SSL_emailTaskHandle, 1); // send email
	else if(watchdogAction1 == OPEN_SWITCH_1)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		switch1_setting = 1;
	}
	else if(watchdogAction1 == OPEN_SWITCH_2)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
		switch2_setting = 1;
	}

	else if(watchdogAction1 == CLOSE_SWITCH_1)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		switch1_setting = 0;
	}

	else if(watchdogAction1 == CLOSE_SWITCH_2)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
		switch2_setting = 0;
	}

	else if(watchdogAction1 == SEND_EMAIL_OPEN_SWITCH_1)
	{
		osSignalSet(send_SSL_emailTaskHandle, 1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		switch1_setting = 1;
	}
	else if(watchdogAction1 == WAIT_60S_OPEN_SWITCH_2)
	{
		timerCallbackArgument = OPEN_SWITCH2;
		osTimerDef(SwitchTimer, osTimerCallback);

		osTimerId osTimer1 = osTimerCreate(osTimer(SwitchTimer), osTimerOnce, NULL);

		osTimerStart(osTimer1, 2000);
	}

	// action 2
	if(watchdogAction2 == SEND_EMAIL)
		osSignalSet(send_SSL_emailTaskHandle, 1); // send email
	else if(watchdogAction2 == OPEN_SWITCH_1)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		switch1_setting = 1;
	}
	else if(watchdogAction2 == OPEN_SWITCH_2)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
		switch2_setting = 1;
	}

	else if(watchdogAction2 == CLOSE_SWITCH_1)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
		switch1_setting = 0;
		printf("Closing switch 1\n");
	}

	else if(watchdogAction2 == CLOSE_SWITCH_2)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
		switch2_setting = 0;
	}

	else if(watchdogAction2 == SEND_EMAIL_OPEN_SWITCH_1)
	{
		osSignalSet(send_SSL_emailTaskHandle, 1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
		switch1_setting = 1;
	}
	else if(watchdogAction2 == WAIT_60S_OPEN_SWITCH_2)
	{
		timerCallbackArgument = OPEN_SWITCH2;
		osTimerDef(SwitchTimer, osTimerCallback);

		osTimerId osTimer1 = osTimerCreate(osTimer(SwitchTimer), osTimerOnce, NULL);

		osTimerStart(osTimer1, 60000);
	}

	watchdogState = WATCHDOG_TRIGGERED;
}

// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)	// interrupt used only for pulse measurements (ADC triggered by external signal)
void EXTI9_5_IRQHandler(void)
{
//	bug: if interrupt is triggered before TIM9 is running then application will freeze ?

	uint16_t timerValue = pulseMeasurementDelay * 217 - 110;

	TIM9->ARR = timerValue;		// set timer auto reload value
	TIM9->CNT &= 0xFFFF0000;	// reset counter to 0
	TIM9->SR &= !TIM_SR_UIF;	// clear update interrupt flag
	while(!(TIM9->SR && TIM_SR_UIF));	// wait for update interrupt flag

	// start conversion
	if(CH1_mode != ADC_FREE_RUN)
	{
		__HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_EOC | ADC_FLAG_OVR);
		(&hadc1)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}

	if(CH2_mode != ADC_FREE_RUN)
	{
		__HAL_ADC_CLEAR_FLAG((&hadc2), ADC_FLAG_EOC | ADC_FLAG_OVR);
		(&hadc2)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}

	if(CH3_mode != ADC_FREE_RUN)
	{
		__HAL_ADC_CLEAR_FLAG((&hadc3), ADC_FLAG_EOC | ADC_FLAG_OVR);
		(&hadc3)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
	}

	// wait for EOC flag and read value
	if(CH1_mode != ADC_FREE_RUN)
	{
		while(!(__HAL_ADC_GET_FLAG((&hadc1), ADC_FLAG_EOC)));
		voltage1_raw = HAL_ADC_GetValue(&hadc1);
		__HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}

	if(CH2_mode != ADC_FREE_RUN)
	{
		while(!(__HAL_ADC_GET_FLAG((&hadc2), ADC_FLAG_EOC)));
		voltage2_raw = HAL_ADC_GetValue(&hadc2);
		__HAL_ADC_CLEAR_FLAG((&hadc2), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}

	if(CH3_mode != ADC_FREE_RUN)
	{
		while(!(__HAL_ADC_GET_FLAG((&hadc3), ADC_FLAG_EOC)));
		voltage3_raw = HAL_ADC_GetValue(&hadc3);
		__HAL_ADC_CLEAR_FLAG((&hadc3), ADC_FLAG_STRT | ADC_FLAG_EOC);
	}

	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);

//	printf("!\n");
}

static void osTimerCallback(void const *argument)
{
	if(timerCallbackArgument == OPEN_SWITCH2) // at the moment only one option
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);	 // 0 output (FET gate) means open switch
		switch2_setting = 1;
		printf("Timer callback execuded OPEN_SWITCH2\n\n");
	}
}

