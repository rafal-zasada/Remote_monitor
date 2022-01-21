/*
 * web_server.c
 *
 *  Created on: Feb 12, 2021
 *      Author: Rafal
 */

#include "web_server.h"
#include "api.h"
#include "lwip/apps/fs.h"
#include "development_aid.h"
#include "string.h"
#include "lwip.h"
#include "application_core.h"

extern UART_HandleTypeDef huart3;
static void web_server_task(void *arg);
static void http_server_serve(struct netconn *conn);
static void send_monitor_data(struct netconn *conn);
static void read_POST(struct netconn *conn, char *buf, uint16_t buflen);
static void send_all_settings(struct netconn *conn);



// !!! fsdata_custom.c must contain 404.html page otherwise application will crash regardless whether 404 is called or not !!!



void WebServerInit(void)
{
	sys_thread_new("myHTTP", web_server_task, NULL, DEFAULT_THREAD_STACKSIZE, osPriorityNormal); // function related to LwIP
}

static void web_server_task(void *arg)
{

	printf("\nWebserver started\n");

	struct netconn *conn, *newconn;
	err_t err, accept_err;

	conn = netconn_new(NETCONN_TCP); // Create a new TCP connection handle
	if(conn!= NULL)
	{
		err = netconn_bind(conn, NULL, 80);	// Bind to port 80 (HTTP) with default IP address

		if(err == ERR_OK)
    	{
			netconn_listen(conn);			// Put the connection into LISTEN state

			while(1)
			{
				accept_err = netconn_accept(conn, &newconn);	// accept any incoming connection

				if(accept_err == ERR_OK)
				{
					http_server_serve(newconn);		// serve connection
					netconn_delete(newconn);		// delete connection
				}
			}
		}
	}
}

static void http_server_serve(struct netconn *conn) // new connection service
{
	struct netbuf *inbuf;
	err_t recv_err;
	char* buf;			// just a pointer without copying inbuf ???
	u16_t buflen;
	struct fs_file file;

	recv_err = netconn_recv(conn, &inbuf); // Read the data from the port, blocking if nothing yet there. We assume the request (the part we care about) is in one netbuf

	if(recv_err == ERR_OK)
	{
		if(netconn_err(conn) == ERR_OK)
		{
			netbuf_data(inbuf, (void**)&buf, &buflen);  // Get the data pointer and length of the data inside a netbuf.


			// debug start
			// print whole input buffer
			// HAL_UART_Transmit(&huart3, (unsigned char*)buf , buflen, 200); // buf is not \0 terminated hence need to use buflen for UART transmit !

			if((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0)) // Is this an HTTP GET command? (only check the first 5 chars, since there are other formats for GET, and we're keeping it very simple. Rest of the header is ignored )
			{
				if(strncmp((char const *)buf,"GET /js/main.js",15) == 0) // Check if request to get ST.gif
				{
					fs_open(&file, "/js/main.js");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if(strncmp((char const *)buf,"GET /cs/styles.css", 18) == 0)
				{
					fs_open(&file, "/cs/styles.css");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if((strncmp(buf, "GET /pictures/TMD_logo.png", 26) == 0))
				{
					fs_open(&file, "/pictures/TMD_logo.png");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if((strncmp(buf, "GET /pictures/CPI_logo.png", 26) == 0))
				{
					fs_open(&file, "/pictures/CPI_logo.png");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if((strncmp(buf, "GET /index.html", 15) == 0)||(strncmp(buf, "GET / ", 6) == 0))
				{
					fs_open(&file, "/index.html");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}

				else if(strncmp((char const *)buf,"GET /get_host_IP", 16) == 0)
				{
					osDelay(300); // to test javascript asynchronicity and other things ***********************************************

					char host_IP_string[17] = {0};
					extern struct netif gnetif;
					char response[100] = "HTTP/1.1 200 OK\r\n"
										 "Content-Type: text/html\r\n"
										 "Access-Control-Allow-Origin:* \r\n"
										 //	"Content-Length : 20\r\n"
										 //	"Connection: keep-alive \r\n";
										 "\r\n"; // second\r\n mandatory to mark end of header!

					Integer_to_IP(gnetif.ip_addr.addr, host_IP_string);
					strcat(response, host_IP_string); // both strings have to be NULL terminated
					netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
				}

				else if(strncmp((char const *)buf,"GET /get_all_settings", 21) == 0)
				{
					send_all_settings(conn);
				}

				else if(strncmp((char const *)buf,"GET /data1", 10) == 0)
				{
					send_monitor_data(conn);
				}

				else
				{
					fs_open(&file, "/404.html");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
					fs_close(&file);
				}
			}

			if((buflen >= 6) && (strncmp(buf, "POST /", 6) == 0)) // check if it is HTTP POST request
				{
					read_POST(conn, buf, buflen);
					 HAL_UART_Transmit(&huart3, (unsigned char*)buf , buflen, 200); // buf is not \0 terminated hence need to use buflen for UART transmit !
				}
		}
	}

	netconn_close(conn); // Close the connection (server closes in HTTP)
	netbuf_delete(inbuf); // Delete the buffer (netconn_recv gives us ownership, so we have to make sure to deallocate the buffer)
}


extern int CH1_setting;
extern int CH2_setting;
extern int CH3_setting;
extern int Relay1_setting;
extern int Relay2_setting;

static void send_all_settings(struct netconn *conn)
{
	char Message[300];

	HAL_UART_Transmit(&huart3, (char unsigned*)"\n\nSend all settings triggered on server\n\n", 19, 200);

	snprintf(Message, sizeof(Message), 	"HTTP/1.1 200 OK\r\n"
										"Content-Type: text/html\r\n"
										"Access-Control-Allow-Origin:* \r\n"
										"\r\n"
										"{\"Ch1_setting\" : \"%d\","
										"\"Ch2_setting\" : \"%d\","
										"\"Ch3_setting\" : \"%d\","
										"\"Relay1_setting\" : \"%d\","
										"\"Relay2_setting\" : \"%d\""
										"}", CH1_setting, CH2_setting, CH3_setting, Relay1_setting, Relay2_setting);

	netconn_write(conn, (signed char*)Message, strlen(Message), NETCONN_NOCOPY);

}

static void read_POST(struct netconn *conn, char *buf, uint16_t buflen)
{
	char *messagePointer = strstr(buf, "\r\n\r\n"); // find end of the header

	if(messagePointer != NULL)
	{
		messagePointer += 4; // skip 2 new line characters and point to POST body

		u32_t receivedMessageLength = buflen - (messagePointer - buf); 		// subtract header length from the length of whole message
		#define bufferSize 15
		char receivedMessage[bufferSize];



		if(!(receivedMessageLength >= bufferSize - 1)) 						// check for buffer overflow
		{
			strncpy(receivedMessage, messagePointer, receivedMessageLength);
			receivedMessage[receivedMessageLength] = 0;							// terminate string

			printf("\n webserver POST: %s\n", receivedMessage);

			settingsMailDataType *newSettingsPtr; // normally created in a thread before infinite loop but this should be OK.

			newSettingsPtr = osMailCAlloc(mailSettingsHandle, osWaitForever); // alloc in sender, free in receiver
			strncpy(newSettingsPtr->mailString, receivedMessage, bufferSize);

			printf("\nString from mail struct = %s\n", newSettingsPtr->mailString);

			if(osMailPut(mailSettingsHandle, newSettingsPtr) != osOK)
				printf("Error while sending mail\n");



		//	osMailPut(mailSettingsHandle, 0mmm);


			// Data format for reading setting sent by client (settings only)
			// First 3 characters - parameter
			// Forth character - space
			// Fifth, sixth, seventh character - parameter value

			char parameter[4] = {0};
			char parameter_value[4] ={0};
			char *dummy_ptr;

			strncpy(parameter, receivedMessage, 3);
			strncpy(parameter_value, receivedMessage + 4, 3);


			if(strncmp(parameter, "CH1", 3) == 0)
				CH1_setting = strtol(parameter_value, &dummy_ptr, 10);
			if(strncmp(parameter, "CH2", 3) == 0)
				CH2_setting = strtol(parameter_value, &dummy_ptr, 10);
			if(strncmp(parameter, "CH3", 3) == 0)
				CH3_setting = strtol(parameter_value, &dummy_ptr, 10);
			if(strncmp(parameter, "Re1", 3) == 0)
				Relay1_setting = strtol(parameter_value, &dummy_ptr, 10);
			if(strncmp(parameter, "Re2", 3) == 0)
				Relay2_setting = strtol(parameter_value, &dummy_ptr, 10);


			char serverResponse[100] = 	"HTTP/1.1 200 OK\r\n"
								//	"Content-Type: text/html\r\n"			// do I need this in this response?
									"Access-Control-Allow-Origin:* \r\n" 	// allow access for other clients (when request address and webserver address don't match)
									"\r\n";									// second\r\n mandatory to mark end of the header!

			strcat(serverResponse, receivedMessage);
			netconn_write(conn, (signed char*)serverResponse, strlen(serverResponse), NETCONN_NOCOPY); // send header and received message
		}
	}
}

extern char voltage1_str[];
extern char voltage2_str[];
extern char voltage3_str[];
extern char temperature1_str[];
extern char temperature2_str[];

void send_monitor_data(struct netconn *conn)
{
	char response[300] = 	"HTTP/1.1 200 OK\r\n"
										"Content-Type: text/html\r\n"
										"Access-Control-Allow-Origin:* \r\n" 	// allow access for other clients than from within this webserver
									//	"Content-Length : 20\r\n"				// not necessary, length will be established in other way
									//	"Connection: keep-alive \r\n";			// purpose?
										"\r\n";  								// second\r\n mandatory to mark end of header!

	char JSON_data[350] = {0};


	snprintf(JSON_data, sizeof(JSON_data),  "{\"voltage1\" : \"%s\","
											"\"voltage2\" : \"%s\","
											"\"voltage3\" : \"%s\","
											"\"temperature1\" : \"%s\","
											"\"temperature2\" : \"%s\","
											"\"relay1\" : \"%d\","
											"\"relay2\" : \"%d\""
											"}", voltage1_str, voltage2_str, voltage3_str, temperature1_str, temperature2_str, Relay1_setting, Relay2_setting);

	strcat(response, JSON_data);
	netconn_write(conn, (const unsigned char*)(response), strlen(response), NETCONN_NOCOPY);
}

/*
 * @param integerIP - 4 * 8bit value in reversed order read from LwIP stack
 * @param IP_string - pointer to created string (minimum size 16 bytes!)
 */
void Integer_to_IP(uint32_t integerIP, char *IP_string)
{
	uint8_t IP_part1 = (integerIP & 0xFF000000) >> 24;
	uint8_t IP_part2 = (integerIP & 0x00FF0000) >> 16;
	uint8_t IP_part3 = (integerIP & 0x0000FF00) >> 8;
	uint8_t IP_part4 = (integerIP & 0x000000FF);

	snprintf(IP_string, 16, "%u.%u.%u.%u", IP_part4, IP_part3, IP_part2, IP_part1);
}

