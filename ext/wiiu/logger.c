
#include <stdio.h>
#include <string.h>
#include <wiiu/os/debug.h>
#include <wiiu/os/systeminfo.h>
#include <wiiu/os/thread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/iosupport.h>

//#define PC_DEVELOPMENT_IP_ADDRESS "192.168.2.3"

#ifndef PC_DEVELOPMENT_TCP_PORT
#define PC_DEVELOPMENT_TCP_PORT 4405
#endif

static ssize_t wiiu_log_write(struct _reent *r, void *fd, const char *ptr, size_t len);
static devoptab_t dotab_stdout = {
	"wiiu_log",     // device name
	0,              // size of file structure
	NULL,           // device open
	NULL,           // device close
	wiiu_log_write, // device write
	NULL,
	/* ... */
};

static int wiiu_log_socket = -1;
static volatile int wiiu_log_lock = 0;

static ssize_t wiiu_log_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
	if (wiiu_log_socket < 0) {
		if (ptr[len - 1] == '\n') {
			if (len > 1)
				OSConsoleWrite(ptr, len - 1);
		} else {
			OSConsoleWrite(ptr, len);
		}
	} else {
		while (wiiu_log_lock)
			OSSleepTicks(((248625000 / 4)) / 1000);

		wiiu_log_lock = 1;

		int ret;
		int remaining = len;

		while (remaining > 0) {
			int block = remaining < 1400 ? remaining : 1400; // take max 1400 bytes per UDP packet
			ret = send(wiiu_log_socket, ptr, block, 0);

			if (ret < 0)
				break;

			remaining -= ret;
			ptr += ret;
		}

		wiiu_log_lock = 0;
	}

	return len;
}

void net_print_exp(const char *str) {
	if (wiiu_log_socket < 0)
		OSConsoleWrite(str, strlen(str));
	else
		send(wiiu_log_socket, str, strlen(str), 0);
}

void wiiu_log_init(void) {
#if defined(PC_DEVELOPMENT_IP_ADDRESS)
	if (!OSIsHLE()) {
		wiiu_log_lock = 0;
		wiiu_log_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (wiiu_log_socket < 0)
			return;

		struct sockaddr_in connect_addr;
		memset(&connect_addr, 0, sizeof(connect_addr));
		connect_addr.sin_family = AF_INET;
		connect_addr.sin_port = PC_DEVELOPMENT_TCP_PORT;
		inet_aton(PC_DEVELOPMENT_IP_ADDRESS, &connect_addr.sin_addr);

		if (connect(wiiu_log_socket, (struct sockaddr *)&connect_addr, sizeof(connect_addr)) < 0) {
			socketclose(wiiu_log_socket);
			wiiu_log_socket = -1;
		}
	}
#endif

	devoptab_list[STD_OUT] = &dotab_stdout;
	devoptab_list[STD_ERR] = &dotab_stdout;
}

void wiiu_log_deinit(void) {
	fflush(stdout);
	fflush(stderr);

	if (wiiu_log_socket >= 0) {
		socketclose(wiiu_log_socket);
		wiiu_log_socket = -1;
	}
}
