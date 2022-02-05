/*
 * SSL_email.h
 *
 *  Created on: Dec 15, 2021
 *      Author: Rafal
 */

#ifndef INC_SSL_EMAIL_H_
#define INC_SSL_EMAIL_H_

void SSL_email_init(void);

typedef struct emailDAtaReceipient
{
	char emailRecipient[75];
	char emailSubject[75];
	char emailBody[500];
}newEmailType;

#endif /* INC_SSL_EMAIL_H_ */
