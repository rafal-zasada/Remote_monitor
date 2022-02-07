/*
 * SSL_email.h
 *
 *  Created on: Dec 15, 2021
 *      Author: Rafal
 */

#ifndef INC_SSL_EMAIL_H_
#define INC_SSL_EMAIL_H_

void SSL_email_init(void);

#define EMAIL_RECIPIENT_MAX_LENGH 75
#define EMAIL_SUBJECT_MAX_LENGH 75
#define EMAIL_BODY_MAX_SIZE 500

typedef struct emailDAtaReceipient
{
	char emailRecipient[EMAIL_RECIPIENT_MAX_LENGH];
	char emailSubject[EMAIL_SUBJECT_MAX_LENGH];
	char emailBody[EMAIL_BODY_MAX_SIZE];
}newEmailType;

#endif /* INC_SSL_EMAIL_H_ */
