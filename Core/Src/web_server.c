/*
 * web_server.c
 *
 *  Created on: Feb 12, 2021
 *      Author: Rafal
 */

#include "web_server.h"
#include "api.h"
#include "lwip/apps/fs.h"
#include "diagnostic_tools.h"
#include "string.h"
#include "lwip.h"

extern UART_HandleTypeDef huart3;
static void WebServerThread(void *arg);
static void http_server_serve(struct netconn *conn);
static void send_monitor_data(struct netconn *conn);

// !!! fsdata_custom.c must contain 404.html page otherwise application will crash regardless whether 404 is called or not !!!

void WebServerInit(void)
{
	sys_thread_new("myHTTP", WebServerThread, NULL, DEFAULT_THREAD_STACKSIZE, osPriorityNormal);
}

static void WebServerThread(void *arg)
{
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
	// debug start
	// snprintf(GUI_buffer, sizeof(GUI_buffer), " \nServing HTTP request \n ");
	// HAL_UART_Transmit(&huart3, (unsigned char*)&GUI_buffer , strlen(GUI_buffer) + 1, 200);
	// debug end

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

			HAL_UART_Transmit(&huart3, (unsigned char*)buf , buflen, 200); // buf is not \0 terminated hence need to use buflen for UART transmit !
//			snprintf(GUI_buffer, buflen + 7, "111%s999", buf);		// will not work properly because buf is not NULL terminated - can't copy properly to %s
			// debug end

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

				//	HAL_UART_Transmit(&huart3, "\n STM32 response \n", 18, 200);
				//	HAL_UART_Transmit(&huart3, (unsigned char*)response, strlen(response) + 1, 200);
				//	HAL_UART_Transmit(&huart3, "\n\n", 2, 200);

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
					char *receivedPOST = strstr(buf, "\r\n\r\n");

					if(receivedPOST != NULL)
					{
	//					snprintf(GUI_buffer, sizeof(GUI_buffer), "\n\n POST request received \n\n ");		// buf is not \0 terminated hence need to use buflen for UART transmit !
	//					HAL_UART_Transmit(&huart3, (unsigned char*)&GUI_buffer , strlen(GUI_buffer) , 200); //

						receivedPOST += 4; // skip 2 new line characters and point to POST body

						u16_t POST_length = buflen - (receivedPOST - buf); // subtract header length from the length of whole message

	//					HAL_UART_Transmit(&huart3, (unsigned char*)receivedPOST, POST_length, 200);

					// below
						char *responseToPOST = 	"HTTP/1.1 200 OK\r\n"
											//	"Content-Type: text/html\r\n"			// do I need this in this response?
												"Access-Control-Allow-Origin:* \r\n" 	// allow access for other clients (when request address and webserver address don't match)
												"\r\n";									// second\r\n mandatory to mark end of header!

						netconn_write(conn, (signed char*)responseToPOST, strlen(responseToPOST), NETCONN_NOCOPY);
					}
				}
		}
	}

	netconn_close(conn); // Close the connection (server closes in HTTP)
	netbuf_delete(inbuf); // Delete the buffer (netconn_recv gives us ownership, so we have to make sure to deallocate the buffer)
}


int voltage1 = 233;
int voltage2 = 223;
int voltage3 = 255;
int temperature1 = 40;
int temperature2 = 30;
int relay1 = 0;
int relay2 = 1;

void send_monitor_data(struct netconn *conn)
{
	char JSON_data[250] = {0};

	snprintf(JSON_data, sizeof(JSON_data),  "{\"voltage1\" : \"%d\","
											"\"voltage2\" : \"%d\","
											"\"voltage3\" : \"%d\","
											"\"temperature1\" : \"%d\","
											"\"temperature2\" : \"%d\","
											"\"relay1\" : \"%d\","
											"\"relay2\" : \"%d\""
											"}", ++voltage1, --voltage2, ++voltage3, ++temperature1, ++temperature2, ++relay1, ++relay2);

	char response[300] = 	"HTTP/1.1 200 OK\r\n"
										"Content-Type: text/html\r\n"
										"Access-Control-Allow-Origin:* \r\n" 	// allow access for other clients than from within this webserver
									//	"Content-Length : 20\r\n"				// not necessary, length will be established in other way
									//	"Connection: keep-alive \r\n";			// purpose?
										"\r\n";  								// second\r\n mandatory to mark end of header!

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

