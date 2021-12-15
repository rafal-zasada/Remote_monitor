 /** 
  *
  *  Portions COPYRIGHT 2016 STMicroelectronics
  *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
  *
  ******************************************************************************
  * @file    ssl_client.c 
  * @author  MCD Application Team
  * @brief   SSL client application 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
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

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_RSA_C) ||         \
    !defined(MBEDTLS_CERTS_C) || !defined(MBEDTLS_PEM_PARSE_C) || \
    !defined(MBEDTLS_CTR_DRBG_C) || !defined(MBEDTLS_X509_CRT_PARSE_C)
int main( void )
{
  mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
                 "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or "
                 "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
                 "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C "
                 "not defined.\n");
  return( 0 );
}
#else

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

static mbedtls_net_context server_fd;
static uint32_t flags;
static uint8_t buf[1024];
static const uint8_t *pers = "ssl_client";
static uint8_t vrfy_buf[512];
static int ret;

#define SERVER_PORT "465"

#ifdef USE_DHCP
#define SERVER_NAME "smtp.gmail.com"
#else
#define SERVER_NAME "smtp.gmail.com"
#endif




// When the STARTTLS command is used, the EHLO command must also be used.  // not used here?

//USE_DHCP

#define IP_ADDR0  0
#define IP_ADDR1  0
#define IP_ADDR2  0
#define IP_ADDR3  0

#define GW_ADDR0  0
#define GW_ADDR1  0
#define GW_ADDR2  0
#define GW_ADDR3  0

#define MASK_ADDR0  0
#define MASK_ADDR1  0
#define MASK_ADDR2  0
#define MASK_ADDR3  0



static int write_ssl_and_get_response( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );
static int write_ssl_data( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len );



mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;

/* use static allocation to keep the heap size as low as possible */
#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
uint8_t memory_buf[MAX_MEM_SIZE];
#endif

void SSL_Client(void const *argument)
{
  int len;

  /*
   * 0. Initialize the RNG and the session data
   */
#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
  mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));
#endif

  mbedtls_net_init(NULL);
//  mbedtls_ssl_init(&ssl);
//  mbedtls_ssl_config_init(&conf);
//  mbedtls_x509_crt_init(&cacert);  // how come it works without it when cacert is called here but only mbedtls_x509_crt_init(&cert); has been called by MX_MBEDTLS_Init();?
//  mbedtls_ctr_drbg_init(&ctr_drbg);


// called by MX_MBEDTLS_Init();
//  mbedtls_ssl_init(&ssl);     // only allocates memory
//  mbedtls_ssl_config_init(&conf);
//  mbedtls_x509_crt_init(&cert);
//  mbedtls_ctr_drbg_init(&ctr_drbg);
//  mbedtls_entropy_init( &entropy );



  // from smtp.c for reference

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





  mbedtls_printf( "\n  . Seeding the random number generator..." );
 
  mbedtls_entropy_init( &entropy );
  len = strlen((char *)pers);
  if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, len)) != 0)
  {
    mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
    goto exit;
  }

  mbedtls_printf( " ok\n" );

  /*
   * 1. Initialize certificates
   */
  mbedtls_printf( "  . Loading the CA root certificate ..." );

  ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );
  
  if( ret < 0 )
  {
    mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
    goto exit;
  }

  mbedtls_printf( " ok (%d skipped)\n", ret );


  /*
   * 2. Start the connection
   */
  // from forum
//  Setting up local IP address and netmask could get it working. But the root cause of the ERR_RTE (Routing problem) is that the TCP/IP stack has not finished
//  setting up the ip/netmask/gw before netconn_connect is called. That should be a err because the ip/netmask/gw are probably empty at that time.
//  The solution would be delay netconn_connect() until proper IP configuration is done.


  osDelay(5000);

  extern struct netif gnetif;
	snprintf(GUI_buffer, sizeof(GUI_buffer) - 1, "IP = %lu\n\n", gnetif.ip_addr.addr);
	HAL_UART_Transmit(&huart3, (uint8_t*)GUI_buffer, strlen(GUI_buffer) + 1, 200);



	int count = 0;

do
{
	count++;
	printf("\nAttempt no %d\n", count);
  mbedtls_printf( "  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT );
  
  if((ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
  {
    mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
  //  goto exit;
  }

  if(count == 10)
	  break;

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
  mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
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

//  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

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
  

  osDelay(2000);

  len = 73;

  printf("\nWhat is buffer len?, len = %d\n", len);

  ret = mbedtls_ssl_read( &ssl, buf, len );
  len = ret;
  mbedtls_printf( " %d Bytes read after connecting and before sending:\n %s\n", len, (char *) buf );


osDelay(1000);

//char buffer[1000];

char send_buffer[200];
int send_len = 0;

snprintf(send_buffer, sizeof(send_buffer), "EHLO gmail.com\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "auth login\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "Ym9iMjAwNTA2QGdtYWlsLmNvbQ==\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "Qm9iMTIzNDU=\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "MAIL FROM: <bob200506@gmail.com>\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "RCPT To: <zasi@poczta.onet.pl>\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "DATA\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "From: zdzichu@gmail.com\r\n"); // can be any name, probably not all mail providers will display it
send_len = strlen(send_buffer);
write_ssl_data(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "To: zasi@poczta.onet.pl\r\n"); // can be any name
send_len = strlen(send_buffer);
write_ssl_data(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "Subject: Important email :)\r\n\r\n");
send_len = strlen(send_buffer);
write_ssl_data(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), "Body Hello and Regards\r\n");
send_len = strlen(send_buffer);
write_ssl_data(&ssl, (unsigned char*)send_buffer, send_len);

snprintf(send_buffer, sizeof(send_buffer), ".\r\n");
send_len = strlen(send_buffer);
write_ssl_and_get_response(&ssl, (unsigned char*)send_buffer, send_len); // do I have to get response?

printf("\nAfter function\n");

while(1);

  /*
   * 6. Write the GET request
   */

  mbedtls_printf( "\n\nWriting to server first time\n" );

  sprintf((char *) buf, "EHLO gmail.com\r\n");
  len = strlen((char *) buf);

  printf("Message being send: %s", buf);

  while((ret = mbedtls_ssl_write(&ssl, buf, len)) <= 0)
  {
    if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
    {
      mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
      goto exit;
    }
  }

  len = ret;
  mbedtls_printf(" %d bytes written:  %s\n", len, (char *) buf);


  osDelay(100);

  /*
   * 7. Read the HTTP response
   */
   mbedtls_printf("Reading from server first time:\n");

     len = sizeof( buf ) - 1;
     memset( buf, 0, sizeof( buf ) );
     ret = mbedtls_ssl_read( &ssl, buf, len );
     mbedtls_printf( " %d bytes read\n\n%s", len, (char *) buf );

















//     printf("\n ret = %d \n", ret);
//
//     if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
//     {
//       continue;
//     }
//
//     if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
//     {
//    	 printf("Error: MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY");
//       break;
//     }
//
//     if(ret < 0)
//     {
//       mbedtls_printf( "failed!   mbedtls_ssl_read returned %d\n\n", ret );
//       break;
//     }
//
//     len = ret;
//     mbedtls_printf( " %d bytes read\n\n%s", len, (char *) buf );
//   }
//   while(0);
//   //   was while(1);



   printf("\n Just before delay \n");
   osDelay(1000);














  mbedtls_ssl_close_notify( &ssl );

exit:
  mbedtls_net_free( &server_fd );

  mbedtls_x509_crt_free( &cacert );
  mbedtls_ssl_free( &ssl );
  mbedtls_ssl_config_free( &conf );
  mbedtls_ctr_drbg_free( &ctr_drbg );
  mbedtls_entropy_free( &entropy );
  
  if ((ret < 0) && (ret != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY))
  {
    Error_Handler();
  }
  else
  {
   // Success_Handler();
  }
}

#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_SSL_TLS_C &&
          MBEDTLS_SSL_CLI_C && MBEDTLS_NET_C && MBEDTLS_RSA_C &&
          MBEDTLS_CERTS_C && MBEDTLS_PEM_PARSE_C && MBEDTLS_CTR_DRBG_C &&
          MBEDTLS_X509_CRT_PARSE_C */



  static int write_ssl_and_get_response( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
 {
	  printf("Test1\n");

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

      printf("Message written:  %s\n", buf);


      do
      {
    	//  printf("\nTest2\n");


          len = sizeof( data ) - 1;
          memset( data, 0, sizeof( data ) );
          ret = mbedtls_ssl_read( ssl, data, len );


    //	  printf("\nTest3\n");

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



  static int write_ssl_data( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
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


