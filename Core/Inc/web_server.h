/*
 * web_server.h
 *
 *  Created on: Feb 12, 2021
 *      Author: Rafal
 */

#ifndef INC_WEB_SERVER_H_
#define INC_WEB_SERVER_H_

//#include "stm32f7xx_hal.h" // HAL libraries, e.g. UART_HandleTypeDef data type
#include "stdint.h" // for uint32_t

void WebServerInit(void);
void Integer_to_IP(uint32_t integerIP, char *IP_string);


#endif /* INC_WEB_SERVER_H_ */
