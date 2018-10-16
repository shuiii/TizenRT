/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/**
 * @file cloud-api.c
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/boardctl.h>
#include <shell/tash.h>
#include <cJSON.h>

#include <artik_module.h>
#include <artik_cloud.h>
#include <artik_websocket.h>
#include <artik_http.h>
#include <artik_lwm2m.h>

#include "command.h"

#ifdef CONFIG_EXAMPLES_ARTIK_CLOUD
#include "wifi-auto.h"
#endif

#define	FAIL_AND_EXIT(x)			\
	do {								\
		fprintf(stderr, (x));			\
		usage(argv[1], cloud_commands); \
		ret = -1;						\
	} while (0)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define OTA_FIRMWARE_VERSION		"1.0.0"
#define OTA_FIRMWARE_HEADER_SIZE	4096
#define UUID_MAX_LEN				64
#define LWM2M_RES_DEVICE_REBOOT	"/3/0/4"

struct ota_info {
	char header[OTA_FIRMWARE_HEADER_SIZE];
	size_t remaining_header_size;
	size_t offset;
	int fd;
};

static const char akc_root_ca[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n"
	"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
	"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n"
	"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
	"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
	"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n"
	"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n"
	"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n"
	"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n"
	"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n"
	"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n"
	"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n"
	"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n"
	"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n"
	"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n"
	"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n"
	"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n"
	"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n"
	"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n"
	"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n"
	"-----END CERTIFICATE-----\r\n";

static int device_command(int argc, char *argv[]);
static int devices_command(int argc, char *argv[]);
static int message_command(int argc, char *argv[]);
static int action_command(int argc, char *argv[]);
static int connect_command(int argc, char *argv[]);
static int disconnect_command(int argc, char *argv[]);
static int send_command(int argc, char *argv[]);
static int sdr_command(int argc, char *argv[]);
static int dm_command(int argc, char *argv[]);

static artik_websocket_handle ws_handle;
static artik_lwm2m_handle g_dm_client = NULL;
static struct ota_info *g_dm_info = NULL;

const struct command cloud_commands[] = {
	{ "device", "device <token> <device id> [<properties>]", device_command },
	{ "devices", "<token> <user id> [<count> <offset> <properties>]", devices_command },
	{ "message", "<token> <device id> <message>", message_command },
	{ "action", "<token> <destination device id> <action>", action_command },
	{ "connect", "<token> <device id> [use_se]", connect_command },
	{ "disconnect", "", disconnect_command },
	{ "send", "<message>", send_command },
	{ "sdr", "start|status|complete <dtid> <vdid>|<regid>|<regid> <nonce>", sdr_command },
	{ "dm", "connect|read|change|disconnect <token> <did>|<uri>|<uri> [<use_se>]|<value>", dm_command },
	{ "", "", NULL }
};

static void websocket_rx_callback(void *user_data, void *result)
{
	if (result) {
		cJSON *msg = NULL, *error = NULL;

		fprintf(stdout, "RX: %s\n", (char *)result);

		msg = cJSON_Parse((char *)result);
		if (!msg) {
			free(result);
			return;
		}

		/* Check error */
		error = cJSON_GetObjectItem(msg, "error");
		if (error && (error->type == cJSON_Object)) {
			fprintf(stderr, "Cloud sends an error. Close connection. %p\n", ws_handle);
			task_create("cloud_disconnect", SCHED_PRIORITY_DEFAULT, 4096, disconnect_command, NULL);
		}

		cJSON_Delete(msg);
		free(result);
	}
}

static void websocket_connection_callback(void *user_data, void *result)
{
	fprintf(stdout, "Call websocket_connection_callback\n");
	if (result) {
		artik_websocket_connection_state state = (artik_websocket_connection_state)result;

		fprintf(stdout, "Connection state change (%d)\n", state);

		if (state == ARTIK_WEBSOCKET_CLOSED && ws_handle) {
			fprintf(stderr, "Connectivity error. Close connection. %p\n", ws_handle);
			task_create("cloud_disconnect", SCHED_PRIORITY_DEFAULT, 4096, disconnect_command, NULL);
		}
	}
}

static int device_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	bool properties = false;
	artik_ssl_config ssl;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 5) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		goto exit;
	}

	/* Parse optional arguments */
	if (argc > 5) {
		properties = (atoi(argv[5]) > 0);
	}

	memset(&ssl, 0, sizeof(ssl));
	ssl.ca_cert.data = (char *)akc_root_ca;
	ssl.ca_cert.len = sizeof(akc_root_ca);

	err = cloud->get_device(argv[3], argv[4], properties, &response, &ssl);
	if (err != S_OK) {
		FAIL_AND_EXIT("Failed to get user devices\n");
		goto exit;
	}

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int devices_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	int count = 10;
	bool properties = false;
	int offset = 0;
	artik_ssl_config ssl;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 5) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		goto exit;
	}

	/* Parse optional arguments */
	if (argc > 5) {
		count = atoi(argv[5]);
		if (argc > 6) {
			offset = atoi(argv[6]);
			if (argc > 7)
				properties = (atoi(argv[7]) > 0);
		}
	}

	memset(&ssl, 0, sizeof(ssl));
	ssl.ca_cert.data = (char *)akc_root_ca;
	ssl.ca_cert.len = sizeof(akc_root_ca);

	err = cloud->get_user_devices(argv[3], count, properties, offset, argv[4], &response, &ssl);
	if (err != S_OK) {
		FAIL_AND_EXIT("Failed to get user devices\n");
		goto exit;
	}

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int action_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	artik_ssl_config ssl;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 6) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		goto exit;
	}

	memset(&ssl, 0, sizeof(ssl));
	ssl.ca_cert.data = (char *)akc_root_ca;
	ssl.ca_cert.len = sizeof(akc_root_ca);

	err = cloud->send_action(argv[3], argv[4], argv[5], &response, &ssl);
	if (err != S_OK) {
		FAIL_AND_EXIT("Failed to send action\n");
		goto exit;
	}

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int message_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	artik_ssl_config ssl;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 6) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		goto exit;
	}

	memset(&ssl, 0, sizeof(ssl));
	ssl.ca_cert.data = (char *)akc_root_ca;
	ssl.ca_cert.len = sizeof(akc_root_ca);

	err = cloud->send_message(argv[3], argv[4], argv[5], &response, &ssl);
	if (err != S_OK) {
		FAIL_AND_EXIT("Failed to send message\n");
		goto exit;
	}

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int connect_command(int argc, char *argv[])
{
	int ret = 0;
	artik_error err = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_ssl_config ssl;
	artik_secure_element_config se_config;

	if (ws_handle) {
		fprintf(stderr, "Websocket to cloud is already connected. %p", ws_handle);
		ret = -1;
		goto exit;
	}

	/* Check number of arguments */
	if (argc < 5) {
		fprintf(stderr, "Wrong number of arguments\n");
		ret = -1;
		goto exit;
	}

	memset(&ssl, 0, sizeof(ssl));

	if ((argc == 6) && !strncmp(argv[5], "use_se", strlen("use_se"))) {
		artik_security_handle handle;
		artik_security_module *security =
			(artik_security_module *)artik_request_api_module("security");

		se_config.key_id = ARTIK_DEVICE_KEY_ID;
		se_config.key_algo = ECC_SEC_P256R1;
		ssl.se_config = &se_config;

		security = (artik_security_module *)artik_request_api_module("security");
		ret = security->request(&handle);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to load security module (err=%d)\n", ret);
			artik_release_api_module(handle);
			goto exit;
		}

		ret = security->get_certificate(
			handle, se_config.key_id, ARTIK_SECURITY_CERT_TYPE_PEM,
			(unsigned char **)&ssl.client_cert.data, &ssl.client_cert.len);
		if (ret != S_OK || !ssl.client_cert.data || ssl.client_cert.len == 0) {
			fprintf(stderr, "Failed to get device certificate (err=%d)\n", ret);
			artik_release_api_module(handle);
			goto exit;
		}
	}

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		ret = -1;
		goto exit;
	}

	ssl.verify_cert = ssl.se_config ? ARTIK_SSL_VERIFY_NONE :
			ARTIK_SSL_VERIFY_REQUIRED;
	ssl.ca_cert.data = (char *)akc_root_ca;
	ssl.ca_cert.len = sizeof(akc_root_ca);

	err = cloud->websocket_open_stream(&ws_handle, argv[3], argv[4], 30000,
			10000, &ssl);
	if (err != S_OK) {
		fprintf(stderr, "Failed to connect websocket\n");
		ws_handle = NULL;
		ret = -1;
		goto exit;
	}

	if (ssl.client_cert.data) {
		free(ssl.client_cert.data);
	}

	err = cloud->websocket_set_connection_callback(ws_handle, websocket_connection_callback, NULL);
	if (err != S_OK) {
		fprintf(stderr, "Failed to set websocket connection callback\n");
		ws_handle = NULL;
		ret = -1;
		goto exit;
	}

	err = cloud->websocket_set_receive_callback(ws_handle, websocket_rx_callback, NULL);
	if (err != S_OK) {
		fprintf(stderr, "Failed to set websocket receive callback\n");
		ws_handle = NULL;
		ret = -1;
		goto exit;
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int disconnect_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	if (!ws_handle) {
		fprintf(stderr, "Websocket to cloud is not connected\n");
		goto exit;
	}

	cloud->websocket_close_stream(ws_handle);
	ws_handle = NULL;

exit:
	artik_release_api_module(cloud);

	return ret;
}

static int send_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_error err = S_OK;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	if (!ws_handle) {
		fprintf(stderr, "Websocket to cloud is not connected\n");
		goto exit;
	}

	/* Check number of arguments */
	if (argc < 4) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		goto exit;
	}

	fprintf(stderr, "Sending %s\n", argv[3]);

	err = cloud->websocket_send_message(ws_handle, argv[3]);
	if (err != S_OK) {
		FAIL_AND_EXIT("Failed to send message to cloud through websocket\n");
		goto exit;
	}

exit:
	artik_release_api_module(cloud);

	return ret;
}

static int sdr_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = NULL;
	artik_secure_element_config se_config = {
			ARTIK_DEVICE_KEY_ID,
			ECC_SEC_P256R1
	};

	/* Check number of arguments */
	if (argc < 4) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		return ret;
	}

	if (!strcmp(argv[3], "start")) {
		char *response = NULL;

		if (argc < 6) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) {
			FAIL_AND_EXIT("Failed to request cloud module\n");
			goto exit;
		}

		cloud->sdr_start_registration(&se_config, argv[4], argv[5],
				&response);

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
	} else if (!strcmp(argv[3], "status")) {
		char *response = NULL;

		if (argc < 5) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) {
			FAIL_AND_EXIT("Failed to request cloud module\n");
			goto exit;
		}

		cloud->sdr_registration_status(&se_config, argv[4], &response);

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
	} else if (!strcmp(argv[3], "complete")) {
		char *response = NULL;

		if (argc < 6) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) {
			FAIL_AND_EXIT("Failed to request cloud module\n");
			goto exit;
		}

		cloud->sdr_complete_registration(&se_config, argv[4], argv[5],
				&response);

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
	} else {
		fprintf(stdout, "Unknown command: sdr %s\n", argv[3]);
		usage(argv[1], cloud_commands);
		ret = -1;
		goto exit;
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static void dm_on_error(void *data, void *user_data)
{
	artik_error err = (artik_error)data;
	artik_lwm2m_module *lwm2m = NULL;

	fprintf(stderr, "LWM2M error: %s\n", error_msg(err));
	lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
	if (!lwm2m) {
		fprintf(stderr, "Failed to request LWM2M module\n");
		return;
	}

	lwm2m->client_disconnect(g_dm_client);
	lwm2m->client_release(g_dm_client);
	g_dm_client = NULL;
}

static void dm_on_connect(void *data, void *user_data)
{
	artik_error err = (artik_error)(intptr_t)data;

	fprintf(stderr, "Connection status: %s \r\n", error_msg(err));
}

static void dm_on_disconnect(void *data, void *user_data)
{
	artik_error err = (artik_error)(intptr_t)data;

	fprintf(stderr, "Disconnection status: %s \r\n", error_msg(err));
}

static pthread_addr_t delayed_reboot(pthread_addr_t arg)
{
	sleep(3);
	boardctl(BOARDIOC_RESET, EXIT_SUCCESS);

	return NULL;
}

static void reboot(void)
{
	pthread_t tid;

	if (pthread_create(&tid, NULL, delayed_reboot, NULL) != 0) {
		return;
	}
	pthread_detach(tid);
	fprintf(stderr, "Rebooting in 3 seconds\n");
}

static void dm_on_execute_resource(void *data, void *user_data)
{
	char *uri = (char *)data;

	fprintf(stderr, "LWM2M resource execute: %s\n", uri);

	if (!strncmp(uri, LWM2M_RES_DEVICE_REBOOT, strlen(LWM2M_RES_DEVICE_REBOOT))) {
		reboot();
	}

	if (!strncmp(uri, ARTIK_LWM2M_URI_FIRMWARE_UPDATE, ARTIK_LWM2M_URI_LEN)) {
		artik_lwm2m_module *lwm2m = NULL;

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			fprintf(stderr, "Failed to request LWM2M module\n");
			return;
		}
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_DEFAULT,
			strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_DEFAULT));
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_STATE,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_UPDATING,
			strlen(ARTIK_LWM2M_FIRMWARE_STATE_UPDATING));
		int fd = open("/dev/mtdblock7", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Open failed\n");
			return;
		}

		if (write(fd, g_dm_info->header, OTA_FIRMWARE_HEADER_SIZE) < 0) {
			fprintf(stderr, "Write failed\n");
			close(fd);
			return;
		}
		close(fd);
		reboot();
	}
}

static int write_firmware(char *data, size_t len, void *user_data)
{
	int header_size = 0;

	if (g_dm_info->remaining_header_size > 0) {
		header_size = len > g_dm_info->remaining_header_size ? g_dm_info->remaining_header_size : len;
		memcpy(g_dm_info->header + g_dm_info->offset, data, header_size);
		len -= header_size;
		g_dm_info->remaining_header_size -= header_size;
		g_dm_info->offset += header_size;
		printf("Skip OTA header (header_size %d, len %d, remaining_header_size %d)\n", header_size, len, g_dm_info->remaining_header_size);
	}

	if (len > 0) {
		if (write(g_dm_info->fd, data+header_size, len) < 0) {
			fprintf(stderr, "Write failed\n");
			return -1;
		}
	}

	return len;
}

static int download_firmware(int argc, char *argv[])
{
	artik_lwm2m_module *lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
	artik_http_module *http = (artik_http_module *)artik_request_api_module("http");
	artik_ssl_config ssl_conf;
	int status = 0;

	artik_error ret;
	artik_http_headers headers;
	artik_http_header_field fields[] = {
		{ "User-Agent", "Artik Firmware Updater"}
	};

	if (!lwm2m) {
		fprintf(stderr, "Failed to request LWM2M module\n");
		return 1;
	}

	/* Only allocate if was not allocated before */
	if (!g_dm_info) {
		g_dm_info = malloc(sizeof(struct ota_info));
		if (!g_dm_info) {
			fprintf(stderr, "Failed to allocate memory\n");
			return 1;
		}
	}

	memset(g_dm_info, 0, sizeof(struct ota_info));
	g_dm_info->remaining_header_size = OTA_FIRMWARE_HEADER_SIZE;

	if (!http) {
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR,
			strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR));

		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_STATE,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_IDLE,
			strlen(ARTIK_LWM2M_FIRMWARE_STATE_IDLE));
		artik_release_api_module(lwm2m);
		return 1;
	}

	headers.fields = fields;
	headers.num_fields = ARRAY_SIZE(fields);
	memset(&ssl_conf, 0, sizeof(artik_ssl_config));
	ssl_conf.verify_cert = ARTIK_SSL_VERIFY_NONE;
	g_dm_info->fd = open("/dev/mtdblock7", O_RDWR);
	if (g_dm_info->fd < 0) {
		fprintf(stderr, "Open failed\n");
		artik_release_api_module(lwm2m);
		artik_release_api_module(http);
		return 1;
	}
	if (lseek(g_dm_info->fd, 4096, SEEK_SET) == (off_t)-1) {
		fprintf(stderr, "Seek failed\n");
		artik_release_api_module(lwm2m);
		artik_release_api_module(http);
		return 1;
	}
	ret = http->get_stream(argv[1], &headers, &status, write_firmware, NULL, &ssl_conf);
	if (ret != S_OK) {
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR,
			strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR));

		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_STATE,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_IDLE,
			strlen(ARTIK_LWM2M_FIRMWARE_STATE_IDLE));
		artik_release_api_module(lwm2m);
		artik_release_api_module(http);
		close(g_dm_info->fd);
		return 1;
	}

	lwm2m->client_write_resource(
		g_dm_client,
		ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
		(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_SUCCESS,
		strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_SUCCESS));

	lwm2m->client_write_resource(
		g_dm_client,
		ARTIK_LWM2M_URI_FIRMWARE_STATE,
		(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_DOWNLOADED,
		strlen(ARTIK_LWM2M_FIRMWARE_STATE_DOWNLOADED));
	artik_release_api_module(lwm2m);
	artik_release_api_module(http);
	close(g_dm_info->fd);
	return 0;
}

static void dm_on_changed_resource(void *data, void *user_data)
{
	artik_lwm2m_resource_t *res = (artik_lwm2m_resource_t *)data;

	fprintf(stderr, "LWM2M resource changed: %s\n", res->uri);
	if (!strncmp(res->uri, ARTIK_LWM2M_URI_FIRMWARE_PACKAGE_URI, ARTIK_LWM2M_URI_LEN)) {
		artik_lwm2m_module *lwm2m = NULL;
		char *firmware_uri;
		char *argv[2] = { NULL };

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			fprintf(stderr, "Failed to request LWM2M module\n");
			return;
		}

		/* Initialize attribute "Update Result" */
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_DEFAULT,
			strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_DEFAULT));

		/* Change attribute "State" to "Downloading" */
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_STATE,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_DOWNLOADING,
			strlen(ARTIK_LWM2M_FIRMWARE_STATE_DOWNLOADING));

		/* The FW URI is empty, come back to IDLE */
		if (res->length == 0) {
			lwm2m->client_write_resource(
				g_dm_client,
				ARTIK_LWM2M_URI_FIRMWARE_STATE,
				(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_IDLE,
				strlen(ARTIK_LWM2M_FIRMWARE_STATE_IDLE));
			return;
		}


		/* The FW URI is not valid */
		if (res->length > 255) {
			fprintf(stderr, "ERROR: Unable to retrieve firmware package uri\n");

			lwm2m->client_write_resource(
				g_dm_client,
				ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
				(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR,
				strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_URI_ERR));

			lwm2m->client_write_resource(
				g_dm_client,
				ARTIK_LWM2M_URI_FIRMWARE_STATE,
				(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_IDLE,
				strlen(ARTIK_LWM2M_FIRMWARE_STATE_IDLE));

			return;
		}

		firmware_uri = strndup((char *)res->buffer, res->length);
		fprintf(stdout, "Downloading firmware from %s\n", firmware_uri);
		argv[0] = firmware_uri;
		task_create("download-firmware", SCHED_PRIORITY_DEFAULT, 16384, download_firmware, argv);
		artik_release_api_module(lwm2m);
	}
}

static int dm_command(int argc, char *argv[])
{
	int ret = 0;
	artik_lwm2m_module *lwm2m = NULL;
	artik_ssl_config *ssl_config = NULL;
	artik_lwm2m_config dm_config;
	artik_secure_element_config se_config = {
			ARTIK_DEVICE_KEY_ID,
			ECC_SEC_P256R1
	};

	/* Check number of arguments */
	if (argc < 4) {
		FAIL_AND_EXIT("Wrong number of arguments\n");
		return ret;
	}

	memset(&dm_config, 0, sizeof(dm_config));

	if (!strcmp(argv[3], "connect")) {

		if (g_dm_client) {
			fprintf(stderr, "DM Client is already started, stop it first\n");
			ret = -1;
			goto exit;
		}

		if (argc < 6) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			FAIL_AND_EXIT("Failed to request lwm2m module\n");
			goto exit;
		}

		ssl_config = zalloc(sizeof(artik_ssl_config));
		if (!ssl_config) {
			fprintf(stderr, "Failed to allocate memory for DM SSL config\n");
			ret = -1;
			goto exit;
		}

		if ((argc == 7) && !strncmp(argv[6], "use_se", strlen("use_se"))) {
			artik_security_handle handle;
			artik_security_module *security =
				(artik_security_module *)artik_request_api_module("security");
			unsigned char *se_cert = NULL;
			unsigned int se_cert_len = 0;

			ssl_config->se_config = &se_config;

			ret = security->request(&handle);
			if (ret != S_OK) {
				fprintf(stderr, "Failed to load security module (err=%d)\n", ret);
				artik_release_api_module(security);
				goto exit;
			}

			ret = security->get_certificate(
				handle, se_config.key_id, ARTIK_SECURITY_CERT_TYPE_PEM,
				&se_cert, &se_cert_len);
			if (ret != S_OK || !se_cert || se_cert_len == 0) {
				fprintf(stderr, "Failed to get device certificate (err=%d)\n", ret);
				security->release(handle);
				artik_release_api_module(security);
				goto exit;
			}

			/* Use  the device cert */
			ssl_config->client_cert.data = (char *)se_cert;
			ssl_config->client_cert.len = se_cert_len;

			security->release(handle);
			artik_release_api_module(security);
		}

		ssl_config->ca_cert.data = (char *)akc_root_ca;
		ssl_config->ca_cert.len = sizeof(akc_root_ca);
		ssl_config->verify_cert = ssl_config->se_config ? ARTIK_SSL_VERIFY_NONE :
				ARTIK_SSL_VERIFY_REQUIRED;

		dm_config.ssl_config = ssl_config;
		dm_config.server_id = 123;
		dm_config.server_uri = ssl_config->se_config ?
				"coaps+tcp://coaps-api.artik.cloud:5689" :
				"coaps://coaps-api.artik.cloud:5686";
		dm_config.lifetime = 30;
		dm_config.name = strndup(argv[5], UUID_MAX_LEN);
		dm_config.tls_psk_identity = dm_config.name;
		dm_config.tls_psk_key = strndup(argv[4], UUID_MAX_LEN);
		dm_config.objects[ARTIK_LWM2M_OBJECT_DEVICE] =
			lwm2m->create_device_object("Samsung", "ARTIK05x", "1234567890", "1.0",
							"1.0", "1.0", "A05x", 0, 5000, 1500, 100, 1000000, 200000,
							"Europe/Paris", "+01:00", "U");
		if (!dm_config.objects[ARTIK_LWM2M_OBJECT_DEVICE]) {
			fprintf(stderr, "Failed to allocate memory for object device.");
			ret = -1;
			goto exit;
		}

		dm_config.objects[ARTIK_LWM2M_OBJECT_FIRMWARE] =
			lwm2m->create_firmware_object(false, "Artik/TizenRT",
					OTA_FIRMWARE_VERSION);
		if (!dm_config.objects[ARTIK_LWM2M_OBJECT_FIRMWARE]) {
			fprintf(stderr, "Failed to allocate memory for object firmware.");
			ret = -1;
			goto exit;
		}

		ret = lwm2m->client_request(&g_dm_client, &dm_config);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to request lwm2m handle (%d)\n", ret);
			ret = -1;
			goto exit;
		}

		if (ssl_config->client_cert.data)
			free(ssl_config->client_cert.data);

		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_ERROR,
					dm_on_error, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_CONNECT,
					dm_on_connect, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_DISCONNECT,
					dm_on_disconnect, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_RESOURCE_EXECUTE,
					dm_on_execute_resource, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_RESOURCE_CHANGED,
					dm_on_changed_resource, (void *)g_dm_client);

		ret = lwm2m->client_connect(g_dm_client);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to connect to the DM server (%d)\n", ret);
			lwm2m->client_release(g_dm_client);
			g_dm_client = NULL;
			ret = -1;
			goto exit;
		}
	} else if (!strcmp(argv[3], "disconnect")) {

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			FAIL_AND_EXIT("Failed to request lwm2m module\n");
			goto exit;
		}

		lwm2m->client_disconnect(g_dm_client);
		lwm2m->client_release(g_dm_client);
		g_dm_client = NULL;
	} else if (!strcmp(argv[3], "read")) {
		char value[256] = {0};
		int len = 256;

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		if (argc < 5) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			FAIL_AND_EXIT("Failed to request lwm2m module\n");
			goto exit;
		}

		ret = lwm2m->client_read_resource(g_dm_client, argv[4], (unsigned char *)value, &len);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to read %s\n", argv[4]);
			ret = -1;
			goto exit;
		}

		printf("URI: %s - Value: %s\n", argv[4], value);

	} else if (!strcmp(argv[3], "change")) {

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		if (argc < 6) {
			FAIL_AND_EXIT("Wrong number of arguments\n");
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			FAIL_AND_EXIT("Failed to request lwm2m module\n");
			goto exit;
		}

		ret = lwm2m->client_write_resource(g_dm_client, argv[4], (unsigned char *)argv[5],
											strlen(argv[5]));
		if (ret != S_OK) {
			fprintf(stderr, "Failed to write %s (%d)\n", argv[4], ret);
			ret = -1;
			goto exit;
		}
	} else if (!strcmp(argv[3], "otaend")) {
		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			FAIL_AND_EXIT("Failed to request lwm2m module\n");
			goto exit;
		}

		lwm2m->client_write_resource(
							g_dm_client,
							ARTIK_LWM2M_URI_FIRMWARE_UPDATE_RES,
							(unsigned char *)ARTIK_LWM2M_FIRMWARE_UPD_RES_SUCCESS,
							strlen(ARTIK_LWM2M_FIRMWARE_UPD_RES_SUCCESS));
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_FIRMWARE_STATE,
			(unsigned char *)ARTIK_LWM2M_FIRMWARE_STATE_IDLE,
			strlen(ARTIK_LWM2M_FIRMWARE_STATE_IDLE));
		lwm2m->client_write_resource(
			g_dm_client,
			ARTIK_LWM2M_URI_DEVICE_FW_VERSION,
			(unsigned char *)OTA_FIRMWARE_VERSION,
			strlen(OTA_FIRMWARE_VERSION));
	} else {
		fprintf(stdout, "Unknown command: dm %s\n", argv[3]);
		usage(argv[1], cloud_commands);
		ret = -1;
		goto exit;
	}

exit:
	if (dm_config.objects[ARTIK_LWM2M_OBJECT_DEVICE]) {
		if (lwm2m)
			lwm2m->free_object(dm_config.objects[ARTIK_LWM2M_OBJECT_DEVICE]);
	}
	if (dm_config.objects[ARTIK_LWM2M_OBJECT_FIRMWARE]) {
		if (lwm2m)
			lwm2m->free_object(dm_config.objects[ARTIK_LWM2M_OBJECT_FIRMWARE]);
	}
	if (dm_config.name)
		free(dm_config.name);
	if (dm_config.tls_psk_key)
		free(dm_config.tls_psk_key);
	if (dm_config.ssl_config)
		free(dm_config.ssl_config);

	if (lwm2m)
		artik_release_api_module(lwm2m);

	return ret;
}

int cloud_main(int argc, char *argv[])
{
	return commands_parser(argc, argv, cloud_commands);
}

#ifdef CONFIG_EXAMPLES_ARTIK_CLOUD
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int artik_cloud_main(int argc, char *argv[])
#endif
{
	StartWifiConnection();
	return 0;
}
#endif
