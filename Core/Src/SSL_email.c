/*
 * SSL_email.c
 *
 *  Created on: Dec 14, 2021
 *      Author: Rafal
 */
/*
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time       time
#define mbedtls_time_t     time_t
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#endif

// a lot of #if options has been removed from original example for clarity

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include "diagnostic_tools.h"
#include "netif.h"  // for netif type
#include "SSL_email.h"
#include "base64.h"
#include "lwip.h"  // for sys_thread_new()

static mbedtls_net_context server_fd;
static uint32_t flags;
static uint8_t buf[1024];
static const uint8_t *pers = (unsigned char*)"ssl_client";   // what for?
static uint8_t vrfy_buf[512];
static int ret;
extern struct netif gnetif;
osThreadId send_SSL_emailTaskHandle;


#define EMAIL_LOGIN_ASCII_BUF_SIZE 75
#define EMAIL_PASSWORD_ASCII_BUF_SIZE 75
#define EMAIL_LOGIN_BASE64_BUF_SIZE 100
#define EMAIL_PASSWORD_BASE64_BUF_SIZE 100

struct emailData
{
	char serverPort[5];
	char serverName[30];
	char emailLogin_ASCII[EMAIL_LOGIN_ASCII_BUF_SIZE];
	char emailPassword_ASCII[EMAIL_PASSWORD_ASCII_BUF_SIZE];
}email;


// already created in mbedtls.c
extern mbedtls_ssl_context ssl;
extern mbedtls_ssl_config conf;
extern mbedtls_x509_crt cert;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_entropy_context entropy;

void send_SSL_email(char *recipient, char *emailSubject, char *emailBody);

static void send_SSL_email_thread(void *argument);
static int write_SSL_and_get_response( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );
static int write_SLL_data( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );
static void send_SSL_email_data(char *recipient, char *emailSubject, char *emailBody);

void SSL_email_init(void)
{
	printf("\nStart SSL_email_init\n");

//			from forum:
//			From the lwIP doc: "Application threads that use lwIP must be created using the lwIP sys_thread_new API......
	sys_thread_new("send_SSL_emailTask", send_SSL_email_thread, NULL, DEFAULT_THREAD_STACKSIZE, osPriorityNormal);

//	temporary hard coded here for testing:
	strncpy(email.serverPort, "465", 4); // included null termination to avoid warnings. Seem to have no effect in further functions
//	email.serverPort[3] = '\0';
	strncpy(email.serverName, "smtp.gmail.com", 15); //
	strncpy(email.emailLogin_ASCII, "bob200506@gmail.com", 20);
	strncpy(email.emailPassword_ASCII, "Bob12345", 9);

}

static void send_SSL_email_thread(void *argument)
{
	while(1)
	{
		// add RTOS signal (semaphore) here to send email


		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 1)
		{
			send_SSL_email("zasi@poczta.onet.pl", "Nowy subject gg", "Email body \n tresc email");
			osDelay(1000);  // to avoid bouncing (not really necessary since send_SSL_email takes considerable time to execute)
		}
		osDelay(100);
	}
}

void send_SSL_email(char *recipient, char *emailSubject, char *emailBody) // those parameters are only passed to function inside this block
{
	int len;

	/*
	 * 0. Initialize the RNG and the session data
	 */
//#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
//	mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));
//#endif

	osDelay(2000);

	// mbedtls_net_init(NULL);  // not needed, MX_LWIP_Init() already called in default task. If you call it second time it will get stuck in timeouts.c

	mbedtls_printf( "\n  . Seeding the random number generator..." );

	len = strlen((char *)pers);
	if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, len)) != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
		goto exit;
	}

	mbedtls_printf( " ok\n" );


	// 1. Initialize certificates

	mbedtls_printf( "  . Loading the CA root certificate ..." );
	ret = mbedtls_x509_crt_parse( &cert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );

	if( ret < 0 )
	{
		mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
		goto exit;
	}

	mbedtls_printf( " ok (%d skipped)\n", ret );



	//  from forum:
	//  Setting up local IP address and netmask could get it working. But the root cause of the ERR_RTE (Routing problem) is that the TCP/IP stack has not finished
	//  setting up the ip/netmask/gw before netconn_connect is called. That should be a err because the ip/netmask/gw are probably empty at that time.
	//  The solution would be delay netconn_connect() until proper IP configuration is done.
	osDelay(500); // neccessary to give allow time for setting up the ip/netmask/gw

	// 2. Start the connection
	int count = 0;

	do
	{
		count++;
		printf("\nAttempt no %d\n", count);
		mbedtls_printf( "  . Connecting to tcp/%s/%s...", email.serverName, email.serverPort);

		if((ret = mbedtls_net_connect(&server_fd, email.serverName, email.serverPort, MBEDTLS_NET_PROTO_TCP)) != 0) // connection unsuccessful
		{
			mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
//			printf("Failed to connect on this occasion\n");
			osDelay(250);
		}

		if(count == 5)
		{
			printf("\nConnecting to server failed\n");
			goto exit;
		}
	}while(ret != 0);

	mbedtls_printf( " ok\n" );

	/*
	 * 3. Setup stuff
	 */
	mbedtls_printf( "  . Setting up the SSL/TLS structure..." );

	if((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
		goto exit;
	}

	mbedtls_printf( " ok\n" );

	/* OPTIONAL is not optimal for security,
	 * but makes interop easier in this simplified example */
	mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
	mbedtls_ssl_conf_ca_chain( &conf, &cert, NULL );

	mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

	if((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
		goto exit;
	}

	if((ret = mbedtls_ssl_set_hostname( &ssl, "localhost" )) != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
		goto exit;
	}

//	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL); 			// no receive timeout
	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);	// receive timeout

	//	Set the timeout period for mbedtls_ssl_read() (Default: no timeout.)
	mbedtls_ssl_conf_read_timeout (&conf, 5000);


	/*
	 * 4. Handshake
	 */
	mbedtls_printf( "  . Performing the SSL/TLS handshake..." );

	while((ret = mbedtls_ssl_handshake( &ssl )) != 0)
	{
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
			goto exit;
		}
	}

	mbedtls_printf( " ok\n" );

	/*
	 * 5. Verify the server certificate
	 */
	mbedtls_printf( "  . Verifying peer X.509 certificate..." );

	if(( flags = mbedtls_ssl_get_verify_result( &ssl )) != 0)
	{

		mbedtls_printf( " failed\n" );
		mbedtls_x509_crt_verify_info((char *)vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags);

		mbedtls_printf( "%s\n", vrfy_buf );
	}
	else
	{
		mbedtls_printf( " ok\n" );
	}

//	osDelay(100);

	mbedtls_ssl_conf_read_timeout (&conf, 2000); // reduce timeout as receiving is not as important as during handshake ?


	ret = mbedtls_ssl_read( &ssl, buf, len ); // ret value can be used for error checking (MBEDTLS_ERR_SSL_TIMEOUT -0x6800)
	len = ret;
	mbedtls_printf( " %d Bytes read after connecting and before sending:\n %s\n", len, (char *) buf );
	osDelay(100);

//	void send_SSL_email(char *recipient, char *emailSubject, char *emailBody)
	send_SSL_email_data(recipient, emailSubject, emailBody);
	mbedtls_ssl_close_notify( &ssl );

	exit:

	mbedtls_net_free( &server_fd );
	mbedtls_x509_crt_free( &cert );
	mbedtls_ssl_free( &ssl );
	mbedtls_ssl_config_free( &conf );
	mbedtls_ctr_drbg_free( &ctr_drbg );
//	mbedtls_entropy_free( &entropy );  // this has to be disabled otherwise seeding will fail during second round (something to do with re-seeding in another function or hardware RNG?)

	if ((ret < 0) && (ret != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY))
	{

		printf("\nERROR occured here :((((((\n");
		osDelay(1000);
		// Error_Handler();
	}
	else
	{
		// Success_Handler();
	}
}

static void send_SSL_email_data(char *recipient, char *emailSubject, char *emailBody)
{
	char send_buffer[200];
	int send_len = 0;

	snprintf(send_buffer, sizeof(send_buffer), "EHLO gmail.com\r\n");
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "auth login\r\n");
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	unsigned char emailLoginBase64[EMAIL_LOGIN_BASE64_BUF_SIZE];
	unsigned char emailPasswordBase64[EMAIL_PASSWORD_BASE64_BUF_SIZE];
	unsigned int bytes_written;

	mbedtls_base64_encode(emailLoginBase64, EMAIL_LOGIN_BASE64_BUF_SIZE, &bytes_written, (uint8_t*)email.emailLogin_ASCII, strlen((char*)email.emailLogin_ASCII)); // login ASCII to base64
	snprintf(send_buffer, sizeof(send_buffer), "%s\r\n", emailLoginBase64);
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	mbedtls_base64_encode(emailPasswordBase64, EMAIL_PASSWORD_BASE64_BUF_SIZE, &bytes_written, (uint8_t*)email.emailPassword_ASCII, strlen((char*)email.emailPassword_ASCII)); // password ASCII to base64
	snprintf(send_buffer, sizeof(send_buffer), "%s\r\n", emailPasswordBase64);
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "MAIL FROM: <%s>\r\n", email.emailLogin_ASCII); // in case when sender email = sender login
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "RCPT To: <%s>\r\n", recipient);
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "DATA\r\n");
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "From: Test Monitor 1\r\n"); // can be any name
	send_len = strlen(send_buffer);
	write_SLL_data(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "To: CPI_Engineer\r\n"); // can be any name, probably not all mail providers will display it
	send_len = strlen(send_buffer);
	write_SLL_data(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "Subject: %s\r\n\r\n", emailSubject);
	send_len = strlen(send_buffer);
	write_SLL_data(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "%s\r\n", emailBody);
	send_len = strlen(send_buffer);
	write_SLL_data(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), ".\r\n");
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

	snprintf(send_buffer, sizeof(send_buffer), "QUIT\r\n");
	send_len = strlen(send_buffer);
	write_SSL_and_get_response(&ssl, (unsigned char*)send_buffer, send_len); // do I have to get response?
}

static int write_SSL_and_get_response( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
{
	int ret;
	unsigned char data[128]; // bigger size = less looping ?
	char code[4];
	size_t i, idx = 0;

	mbedtls_printf("\n%s", buf);
	while( len && ( ret = mbedtls_ssl_write( ssl, buf, len ) ) <= 0 )
	{
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
		{
			mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
			return -1;
		}
	}

	do
	{
		len = sizeof( data ) - 1;
		memset( data, 0, sizeof( data ) );
		ret = mbedtls_ssl_read( ssl, data, len );

		if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
			continue;

		if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
			return -1;

		if( ret <= 0 )
		{
			mbedtls_printf( "failed\n  ! mbedtls_ssl_read returned %d\n\n", ret );
			return -1;
		}

		mbedtls_printf("\nData received:   %s", data);
		len = ret;
		for( i = 0; i < len; i++ )
		{
			if( data[i] != '\n' )
			{
				if( idx < 4 )
					code[ idx++ ] = data[i];
				continue;
			}

			if( idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ' )
			{
				code[3] = '\0';
				return atoi( code );
			}

			idx = 0;
		}
	}
	while( 1 );
}

static int write_SLL_data( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
{
	int ret;

	mbedtls_printf("\n%s", buf);
	while( len && ( ret = mbedtls_ssl_write( ssl, buf, len ) ) <= 0 )
	{
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
		{
			mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
			return -1;
		}
	}

	return( 0 );
}


// from smtp.c just for reference

//#define SMTP_RX_BUF_LEN         255
//#define SMTP_TX_BUF_LEN         255
//#define SMTP_CRLF               "\r\n"
//#define SMTP_CRLF_LEN           2
//
//#define SMTP_RESP_220           "220"
//#define SMTP_RESP_235           "235"
//#define SMTP_RESP_250           "250"
//#define SMTP_RESP_334           "334"
//#define SMTP_RESP_354           "354"
//#define SMTP_RESP_LOGIN_UNAME   "VXNlcm5hbWU6"
//#define SMTP_RESP_LOGIN_PASS    "UGFzc3dvcmQ6"
//
//#define SMTP_KEYWORD_AUTH_SP    "AUTH "
//#define SMTP_KEYWORD_AUTH_EQ    "AUTH="
//#define SMTP_KEYWORD_AUTH_LEN   5
//#define SMTP_AUTH_PARAM_PLAIN   "PLAIN"
//#define SMTP_AUTH_PARAM_LOGIN   "LOGIN"
//
//#define SMTP_CMD_EHLO_1           "EHLO ["
//#define SMTP_CMD_EHLO_1_LEN       6
//#define SMTP_CMD_EHLO_2           "]\r\n"
//#define SMTP_CMD_EHLO_2_LEN       3
//#define SMTP_CMD_AUTHPLAIN_1      "AUTH PLAIN "
//#define SMTP_CMD_AUTHPLAIN_1_LEN  11
//#define SMTP_CMD_AUTHPLAIN_2      "\r\n"
//#define SMTP_CMD_AUTHPLAIN_2_LEN  2
//#define SMTP_CMD_AUTHLOGIN        "AUTH LOGIN\r\n"
//#define SMTP_CMD_AUTHLOGIN_LEN    12
//#define SMTP_CMD_MAIL_1           "MAIL FROM: <"
//#define SMTP_CMD_MAIL_1_LEN       12
//#define SMTP_CMD_MAIL_2           ">\r\n"
//#define SMTP_CMD_MAIL_2_LEN       3
//#define SMTP_CMD_RCPT_1           "RCPT TO: <"
//#define SMTP_CMD_RCPT_1_LEN       10
//#define SMTP_CMD_RCPT_2           ">\r\n"
//#define SMTP_CMD_RCPT_2_LEN       3
//#define SMTP_CMD_DATA             "DATA\r\n"
//#define SMTP_CMD_DATA_LEN         6
//#define SMTP_CMD_HEADER_1         "From: <"
//#define SMTP_CMD_HEADER_1_LEN     7
//#define SMTP_CMD_HEADER_2         ">\r\nTo: <"
//#define SMTP_CMD_HEADER_2_LEN     8
//#define SMTP_CMD_HEADER_3         ">\r\nSubject: "
//#define SMTP_CMD_HEADER_3_LEN     12
//#define SMTP_CMD_HEADER_4         "\r\n\r\n"
//#define SMTP_CMD_HEADER_4_LEN     4
//#define SMTP_CMD_BODY_FINISHED    "\r\n.\r\n"
//#define SMTP_CMD_BODY_FINISHED_LEN 5
//#define SMTP_CMD_QUIT             "QUIT\r\n"
//#define SMTP_CMD_QUIT_LEN         6
//

