#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define die(str, args...) do { \
	perror(str); \
	exit(EXIT_FAILURE); \
} while(0)

const int set_evbit[] = {
	EV_ABS,
	EV_KEY,
	EV_SYN,
};

const int set_keybit[] = {
	BTN_TOUCH,
};

const int set_absbit[] = {
	ABS_X,
	ABS_Y,
	ABS_MT_POSITION_X,
	ABS_MT_POSITION_Y,
	ABS_MT_TOUCH_MINOR,
	ABS_MT_TOUCH_MAJOR,
	ABS_MT_WIDTH_MINOR,
	ABS_MT_WIDTH_MAJOR,
	ABS_MT_ORIENTATION,
	ABS_MT_PRESSURE,
	ABS_MT_SLOT,
	ABS_MT_TRACKING_ID,
};

#define WIDTH	1920
#define HEIGHT	1080

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct timeval32 {
	uint32_t tv_sec;         /* seconds */
	uint32_t tv_usec;        /* microseconds */
};

struct input_event32 {
	struct timeval32 time;
	uint16_t type;
	uint16_t code;
	int32_t  value;
};

int main(void)
{
	int fd;
	FILE *fd_stdin;
	struct uinput_user_dev uidev;
	int dx, dy;
	int i;

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0)
		die("error: open uinput");

	fd_stdin = freopen(NULL, "r+b", stdin);
	if (!fd_stdin)
		die("error: open stdin");

	for (i = 0; i < ARRAY_SIZE(set_evbit); i++)
		if (ioctl(fd, UI_SET_EVBIT, set_evbit[i]) < 0)
			die("error: ioctl");

	for (i = 0; i < ARRAY_SIZE(set_keybit); i++)
		if (ioctl(fd, UI_SET_KEYBIT, set_keybit[i]) < 0)
			die("error: ioctl");

	for (i = 0; i < ARRAY_SIZE(set_absbit); i++)
		if (ioctl(fd, UI_SET_ABSBIT, set_absbit[i]) < 0)
			die("error: ioctl");

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "mtouch-emul");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;

	uidev.absmax[ABS_X] = WIDTH;
	uidev.absmax[ABS_Y] = HEIGHT;
	uidev.absmax[ABS_PRESSURE] = 255;

	uidev.absmax[ABS_MT_POSITION_X] = WIDTH;
	uidev.absmax[ABS_MT_POSITION_Y] = HEIGHT;
	uidev.absmax[ABS_MT_TOUCH_MINOR] = WIDTH;
	uidev.absmax[ABS_MT_TOUCH_MAJOR] = WIDTH;
	uidev.absmax[ABS_MT_WIDTH_MINOR] = WIDTH;
	uidev.absmax[ABS_MT_WIDTH_MAJOR] = WIDTH;
	uidev.absmax[ABS_MT_ORIENTATION] = 180;
	uidev.absmin[ABS_MT_ORIENTATION] = -180;
	uidev.absmax[ABS_MT_PRESSURE] = 255;
	uidev.absmax[ABS_MT_SLOT] = 10;
	uidev.absmax[ABS_MT_TRACKING_ID] = 0xffff;

	if (write(fd, &uidev, sizeof(uidev)) < 0)
		die("error: write");

	if (ioctl(fd, UI_DEV_CREATE) < 0)
		die("error: UI_DEV_CREATE");

	while (1) {
		struct input_event32 ev;
		struct input_event ev_inj;
		char *ptr;
		int to_read, rd;

		to_read = sizeof(ev);
		ptr = (char *)&ev;
		do {
			rd = fread(ptr, 1, to_read, fd_stdin);
			to_read -= rd;
			ptr += rd;
		} while (to_read);

		printf("Report type %u code %u\n", ev.type, ev.code);

		/* tablet is 32-bit, host is 64-bit */
		ev_inj.time.tv_sec = ev.time.tv_sec;
		ev_inj.time.tv_usec = ev.time.tv_usec;
		ev_inj.type = ev.type;
		ev_inj.code = ev.code;
		ev_inj.value = ev.value;

		if (write(fd, &ev_inj, sizeof(ev_inj)) < 0)
			die("error: write");

	}

	if (ioctl(fd, UI_DEV_DESTROY) < 0)
		die("error: UI_DEV_DESTROY");

	close(fd);
	fclose(fd_stdin);

	return 0;
}
