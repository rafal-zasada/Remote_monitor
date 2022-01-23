/*
 * application_core.h
 *
 *  Created on: Jan 19, 2022
 *      Author: Rafal
 */

#ifndef INC_APPLICATION_CORE_H_
#define INC_APPLICATION_CORE_H_


typedef struct
{
	char mailString[20];
} settingsMailDataType;

typedef struct
{
	char voltage1_str[9];
	char voltage2_str[9];
	char voltage3_str[9];
	char temperature1_str[7];
	char temperature2_str[7];
} monitorValuesType;

extern osMailQId mailSettingsHandle;

void app_core_init(void);

#endif /* INC_APPLICATION_CORE_H_ */

