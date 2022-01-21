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

extern osMailQId mailSettingsHandle;

void app_core_init(void);

#endif /* INC_APPLICATION_CORE_H_ */
