/*	opendatacon
 *
 *	Copyright (c) 2014:
 *
 *		DCrip3fJguWgVCLrZFfA7sIGgvx1Ou3fHfCxnrz4svAi
 *		yxeOtDhDCXf1Z4ApgXvX5ahqQmzRfJ2DoX8S05SqHA==
 *	
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *	
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */ 
#ifdef _WIN32
	#include <io.h>
	#include <ws2tcpip.h>
	#include <stdarg.h>
	#include <stdint.h>
	typedef SSIZE_T ssize_t;
	#include <sys/types.h> //off_t, ssize_t
	#include <sys/stat.h>
	#define MHD_PLATFORM_H
#endif

#include <functional>

#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "WebUI.h"

#define BUF_SIZE 1024
#define MAX_URL_LEN 255

// TODO remove if unused
#define CAFILE "ca.pem"
#define CRLFILE "crl.pem"

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"
#define EMPTY_PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"

/* Test Certificate */
const char cert_pem[] =
"-----BEGIN CERTIFICATE-----\n"
"MIICpjCCAZCgAwIBAgIESEPtjjALBgkqhkiG9w0BAQUwADAeFw0wODA2MDIxMjU0\n"
"MzhaFw0wOTA2MDIxMjU0NDZaMAAwggEfMAsGCSqGSIb3DQEBAQOCAQ4AMIIBCQKC\n"
"AQC03TyUvK5HmUAirRp067taIEO4bibh5nqolUoUdo/LeblMQV+qnrv/RNAMTx5X\n"
"fNLZ45/kbM9geF8qY0vsPyQvP4jumzK0LOJYuIwmHaUm9vbXnYieILiwCuTgjaud\n"
"3VkZDoQ9fteIo+6we9UTpVqZpxpbLulBMh/VsvX0cPJ1VFC7rT59o9hAUlFf9jX/\n"
"GmKdYI79MtgVx0OPBjmmSD6kicBBfmfgkO7bIGwlRtsIyMznxbHu6VuoX/eVxrTv\n"
"rmCwgEXLWRZ6ru8MQl5YfqeGXXRVwMeXU961KefbuvmEPccgCxm8FZ1C1cnDHFXh\n"
"siSgAzMBjC/b6KVhNQ4KnUdZAgMBAAGjLzAtMAwGA1UdEwEB/wQCMAAwHQYDVR0O\n"
"BBYEFJcUvpjvE5fF/yzUshkWDpdYiQh/MAsGCSqGSIb3DQEBBQOCAQEARP7eKSB2\n"
"RNd6XjEjK0SrxtoTnxS3nw9sfcS7/qD1+XHdObtDFqGNSjGYFB3Gpx8fpQhCXdoN\n"
"8QUs3/5ZVa5yjZMQewWBgz8kNbnbH40F2y81MHITxxCe1Y+qqHWwVaYLsiOTqj2/\n"
"0S3QjEJ9tvklmg7JX09HC4m5QRYfWBeQLD1u8ZjA1Sf1xJriomFVyRLI2VPO2bNe\n"
"JDMXWuP+8kMC7gEvUnJ7A92Y2yrhu3QI3bjPk8uSpHea19Q77tul1UVBJ5g+zpH3\n"
"OsF5p0MyaVf09GTzcLds5nE/osTdXGUyHJapWReVmPm3Zn6gqYlnzD99z+DPIgIV\n"
"RhZvQx74NQnS6g==\n" "-----END CERTIFICATE-----\n";

const char key_pem[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEowIBAAKCAQEAtN08lLyuR5lAIq0adOu7WiBDuG4m4eZ6qJVKFHaPy3m5TEFf\n"
"qp67/0TQDE8eV3zS2eOf5GzPYHhfKmNL7D8kLz+I7psytCziWLiMJh2lJvb2152I\n"
"niC4sArk4I2rnd1ZGQ6EPX7XiKPusHvVE6VamacaWy7pQTIf1bL19HDydVRQu60+\n"
"faPYQFJRX/Y1/xpinWCO/TLYFcdDjwY5pkg+pInAQX5n4JDu2yBsJUbbCMjM58Wx\n"
"7ulbqF/3lca0765gsIBFy1kWeq7vDEJeWH6nhl10VcDHl1PetSnn27r5hD3HIAsZ\n"
"vBWdQtXJwxxV4bIkoAMzAYwv2+ilYTUOCp1HWQIDAQABAoIBAArOQv3R7gmqDspj\n"
"lDaTFOz0C4e70QfjGMX0sWnakYnDGn6DU19iv3GnX1S072ejtgc9kcJ4e8VUO79R\n"
"EmqpdRR7k8dJr3RTUCyjzf/C+qiCzcmhCFYGN3KRHA6MeEnkvRuBogX4i5EG1k5l\n"
"/5t+YBTZBnqXKWlzQLKoUAiMLPg0eRWh+6q7H4N7kdWWBmTpako7TEqpIwuEnPGx\n"
"u3EPuTR+LN6lF55WBePbCHccUHUQaXuav18NuDkcJmCiMArK9SKb+h0RqLD6oMI/\n"
"dKD6n8cZXeMBkK+C8U/K0sN2hFHACsu30b9XfdnljgP9v+BP8GhnB0nCB6tNBCPo\n"
"32srOwECgYEAxWh3iBT4lWqL6bZavVbnhmvtif4nHv2t2/hOs/CAq8iLAw0oWGZc\n"
"+JEZTUDMvFRlulr0kcaWra+4fN3OmJnjeuFXZq52lfMgXBIKBmoSaZpIh2aDY1Rd\n"
"RbEse7nQl9hTEPmYspiXLGtnAXW7HuWqVfFFP3ya8rUS3t4d07Hig8ECgYEA6ou6\n"
"OHiBRTbtDqLIv8NghARc/AqwNWgEc9PelCPe5bdCOLBEyFjqKiT2MttnSSUc2Zob\n"
"XhYkHC6zN1Mlq30N0e3Q61YK9LxMdU1vsluXxNq2rfK1Scb1oOlOOtlbV3zA3VRF\n"
"hV3t1nOA9tFmUrwZi0CUMWJE/zbPAyhwWotKyZkCgYEAh0kFicPdbABdrCglXVae\n"
"SnfSjVwYkVuGd5Ze0WADvjYsVkYBHTvhgRNnRJMg+/vWz3Sf4Ps4rgUbqK8Vc20b\n"
"AU5G6H6tlCvPRGm0ZxrwTWDHTcuKRVs+pJE8C/qWoklE/AAhjluWVoGwUMbPGuiH\n"
"6Gf1bgHF6oj/Sq7rv/VLZ8ECgYBeq7ml05YyLuJutuwa4yzQ/MXfghzv4aVyb0F3\n"
"QCdXR6o2IYgR6jnSewrZKlA9aPqFJrwHNR6sNXlnSmt5Fcf/RWO/qgJQGLUv3+rG\n"
"7kuLTNDR05azSdiZc7J89ID3Bkb+z2YkV+6JUiPq/Ei1+nDBEXb/m+/HqALU/nyj\n"
"P3gXeQKBgBusb8Rbd+KgxSA0hwY6aoRTPRt8LNvXdsB9vRcKKHUFQvxUWiUSS+L9\n"
"/Qu1sJbrUquKOHqksV5wCnWnAKyJNJlhHuBToqQTgKXjuNmVdYSe631saiI7PHyC\n"
"eRJ6DxULPxABytJrYCRrNqmXi5TCiqR2mtfalEMOPxz8rUU8dYyx\n"
"-----END RSA PRIVATE KEY-----\n";

static ssize_t
file_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	FILE *file = (FILE *)cls;

	(void)fseek(file, pos, SEEK_SET);
	return fread(buf, 1, max, file);
}

static void
file_free_callback(void *cls)
{
	FILE *file = (FILE *)cls;
	fclose(file);
}

/* response handler callback wrapper */
static int ahc(void *cls,
struct MHD_Connection *connection,
	const char *url,
	const char *method,
	const char *version,
	const char *upload_data,
	size_t *upload_data_size,
	void **ptr)
{
	WebUI* test = (WebUI*)cls;
	//return test->http_ahc(cls, connection, url, method, version, upload_data, upload_data_size, ptr);
	return test->ahc_echo(cls, connection, url, method, version, upload_data, upload_data_size, ptr);
}

WebUI::WebUI() : d(nullptr)
	{
	}

/* HTTP static response handler call back */
int WebUI::ahc_echo(void * cls,
	struct MHD_Connection * connection,
		const char * url,
		const char * method,
		const char * version,
		const char * upload_data,
		size_t * upload_data_size,
		void ** ptr) {
		static int dummy;
		const char * page = (const char *)cls;
		struct MHD_Response * response;
		int ret;

		if (0 != strcmp(method, MHD_HTTP_METHOD_GET))
			return MHD_NO; /* unexpected method */
		if (&dummy != *ptr)
		{
			/* The first time only the headers are valid,
			do not respond in the first round... */
			*ptr = &dummy;
			return MHD_YES;
		}
		if (0 != *upload_data_size)
			return MHD_NO; /* upload data in a GET!? */
		*ptr = NULL; /* clear context pointer */
		response = MHD_create_response_from_data(strlen(page),
			(void*)page,
			MHD_NO,
			MHD_NO);
		ret = MHD_queue_response(connection,
			MHD_HTTP_OK,
			response);
		MHD_destroy_response(response);
		return ret;
	}

/* HTTP access handler call back */
int	WebUI::http_ahc(void *cls,
	struct MHD_Connection *connection,
		const char *url,
		const char *method,
		const char *version,
		const char *upload_data,
		size_t *upload_data_size, void **ptr)
	{
			static int aptr;
			struct MHD_Response *response;
			int ret;
			FILE *file;
			struct stat buf;

			if (0 != strcmp(method, MHD_HTTP_METHOD_GET))
				return MHD_NO;              /* unexpected method */
			if (&aptr != *ptr)
			{
				/* do never respond on first call */
				*ptr = &aptr;
				return MHD_YES;
			}
			*ptr = NULL;                  /* reset when done */

			if ((0 == stat(&url[1], &buf)))// && (S_ISREG(buf.st_mode)))
				fopen_s(&file, &url[1], "rb"); //file = fopen(&url[1], "rb");
			else
				file = NULL;
			if (file == NULL)
			{
				response = MHD_create_response_from_buffer(strlen(EMPTY_PAGE),
					(void *)EMPTY_PAGE,
					MHD_RESPMEM_PERSISTENT);
				ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
				MHD_destroy_response(response);
			}
			else
			{
				response = MHD_create_response_from_callback(buf.st_size, 32 * 1024,     /* 32k PAGE_NOT_FOUND size */
					&file_reader, file,
					&file_free_callback);
				if (response == NULL)
				{
					fclose(file);
					return MHD_NO;
				}
				ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
				MHD_destroy_response(response);
			}
			return ret;
		}

	int WebUI::start(uint16_t port)
	{
		d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG |	MHD_USE_SSL, 
			port, // Port to bind to
			NULL, // callback to call to check which clients allowed to connect
			NULL, // extra argument to apc
			&ahc, // handler called for all requests
			this, // extra argument to dh
			MHD_OPTION_CONNECTION_TIMEOUT, 256,
			MHD_OPTION_HTTPS_MEM_KEY, key_pem,
			MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
			MHD_OPTION_END);

		if (d == nullptr)
			return 1;
		return 0;
	}

	void WebUI::stop()
	{
		if (d == nullptr) return;
		MHD_stop_daemon(d);
		d = nullptr;
	}


/**
* @file https_fileserver_example.c
* @brief a simple HTTPS file server using TLS.
*
* Usage :
*
*  'http_fileserver_example HTTP-PORT SECONDS-TO-RUN'
*
* The certificate & key are required by the server to operate,  Omitting the
* path arguments will cause the server to use the hard coded example certificate & key.
*
* 'certtool' may be used to generate these if required.
*
* @author Sagie Amir
*/

int
main(int argc, char *const *argv)
{


	if (argc != 2)
	{
		printf("Usage: %s HTTP-PORT\n", argv[0]);
		return 1;
	}

	WebUI inst;
	int status = inst.start(atoi(argv[1]));

	if (status)
	{
		fprintf(stderr, "Error: failed to start TLS_daemon\n");
		return 1;
	}
	else
	{
		printf("MHD daemon listening on port %d\n", atoi(argv[1]));
	}

	(void)getc(stdin);

	inst.stop();

	return 0;
}

