/*
 * diagnostic_tools.h
 *
 *  Created on: Nov 12, 2021
 *      Author: Rafal
 */

#ifndef INC_DIAGNOSTIC_TOOLS_H_
#define INC_DIAGNOSTIC_TOOLS_H_

#include <string.h>
#include "stm32f7xx_hal.h" // HAL libraries, e.g. UART_HandleTypeDef data type

#define GUI_bufferSize 5000
extern char GUI_buffer[GUI_bufferSize];
extern UART_HandleTypeDef huart3;


void DiagnosticToolsInit(void);

#endif /* INC_DIAGNOSTIC_TOOLS_H_ */
