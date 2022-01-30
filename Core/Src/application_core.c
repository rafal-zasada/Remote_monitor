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

static void read_monitor_values(void);
static void receive_settings_mail_and_parse(void);
void application_core_task(void const *argument);
void monitor_data_to_string(void);

extern TIM_HandleTypeDef htim9;

osThreadId ApplicationCoreTaskHandle;
osMailQId mailSettingsHandle;

// Data format for reading setting sent by client (settings only)
// First 3 characters - parameter
// Forth character - space
// Fifth, sixth, seventh character - parameter value
#define ADC_FREE_RUN 0
int CH1_setting = 0;
int CH2_setting = 1;
int CH3_setting = 0;
int Relay1_setting = 1;  // relay setting = its value
int Relay2_setting = 0;  // relay setting = its value
int pulseMeasurementDelay = 1;

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

	while(1)
	{
		osDelay(50);
		monitor_data_to_string();
		receive_settings_mail_and_parse();
		read_monitor_values();
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
		newSettingsReceivedPtr = mailData.value.p;	// memory alloc'd in sender will have to be free'd here in receiver

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

		if(strncmp(newSettingsReceivedPtr->mailString, "DEL", 3) == 0)
		{
			pulseMeasurementDelay = strtol(p, NULL, 10);
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


	// for test
	voltage1 = voltage1_raw;
	voltage2 = voltage2_raw;
	voltage3 = voltage3_raw;


	// read temperatures here

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
//	  // minimal version for ADC operation extracted from HAL_ADC_Start(&hadc1) and HAL_ADC_PollForConversion(&hadc1, 2000) (HAL_ADC_Start() must run once before)
//	  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_EOC | ADC_FLAG_OVR); 	  // Clear regular group conversion flag and overrun flag (To ensure of no unknown state from potential previous ADC operations)
//	  (&hadc1)->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART; 	  // Enable the selected ADC software conversion for regular group
//	  while(!(__HAL_ADC_GET_FLAG((&hadc1), ADC_FLAG_EOC)));		 // Check End of conversion flag  (wait indefinitely)
//	  voltage1_raw = HAL_ADC_GetValue(&hadc1);
//	  __HAL_ADC_CLEAR_FLAG((&hadc1), ADC_FLAG_STRT | ADC_FLAG_EOC); // Clear regular group conversion flag


	// interrupt used only for pulse measurements (ADC triggered by external signal)

	GPIOG->BSRR = GPIO_PIN_0;	// set pin 0, 3 clock cycles

	uint16_t timerValue = pulseMeasurementDelay * 217 - 114;


	TIM9->ARR = timerValue;		// set timer auto reload value
	TIM9->CNT &= 0xFFFF0000;	// reset counter to 0
	TIM9->SR &= !TIM_SR_UIF;	// clear update interrupt flag
	while(!(TIM9->SR && TIM_SR_UIF));	// wait for update interrupt flag


	GPIOG->BSRR = GPIO_PIN_0 << 16;		// reset pin, 3 clock cycles



	 // HAL_TIM

//	  int pulseMeasurementDelayAdjusted;          // set value is integer but float is needed for delay adjustment
//
//	  if(pulseMeasurementDelay == 1)
//		  pulseMeasurementDelayAdjusted = pulseMeasurementDelay * 6;
//	  else if(pulseMeasurementDelay == 2)
//		  pulseMeasurementDelayAdjusted = pulseMeasurementDelay * 15;
//	  else
//		  pulseMeasurementDelayAdjusted = pulseMeasurementDelay * 25;
//
//
//	  for(int i = 0; i < (pulseMeasurementDelayAdjusted); i++);		// pulse measurement delay implementation (216MHz clock)


//	  printf("pulseMeasurementDelayAdjusted = %d\n", pulseMeasurementDelayAdjusted);

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

