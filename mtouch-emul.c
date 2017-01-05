#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define die(str, args...) do { \
	perror(str); \
	exit(EXIT_FAILURE); \
} while(0)

char *events[EV_MAX + 1] = {
	[0 ... EV_MAX] = NULL,
	[EV_SYN] = "Sync",			[EV_KEY] = "Key",
	[EV_REL] = "Relative",			[EV_ABS] = "Absolute",
	[EV_MSC] = "Misc",			[EV_LED] = "LED",
	[EV_SND] = "Sound",			[EV_REP] = "Repeat",
	[EV_FF] = "ForceFeedback",		[EV_PWR] = "Power",
	[EV_FF_STATUS] = "ForceFeedbackStatus",
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
		die("error: open");

	fd_stdin = freopen(NULL, "r+b", stdin);
	if (!fd_stdin)
		die("error: open");

	if (ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
		die("error: ioctl");

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		die("error: ioctl");

	if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
		die("error: ioctl");

	if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
		die("error: ioctl");

	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0)
		die("error: ioctl");

	if (ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0)
		die("error: ioctl");
	if (ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
		die("error: ioctl");

	for (i = ABS_MT_SLOT; i < ABS_MAX; i++) {
		if (ioctl(fd, UI_SET_ABSBIT, i) < 0)
			die("error: ioctl");
	}

	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "mtouch-emul");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;

	if (write(fd, &uidev, sizeof(uidev)) < 0)
		die("error: write");

	if (ioctl(fd, UI_DEV_CREATE) < 0)
		die("error: ioctl");

	while (1) {
		struct input_event ev;
		char *ptr;
		int to_read, rd;

		to_read = sizeof(ev);
		ptr = (char *)&ev;
		do {
			rd = fread(ptr, 1, to_read, fd_stdin);
			to_read -= rd;
			ptr += rd;
		} while (to_read);

#if 0
		printf("Got event type %x code %x value %x\n",
			ev.type, ev.code, ev.value);
#endif
		if (write(fd, &ev, sizeof(ev)) < 0)
			die("error: write");

	}

	if (ioctl(fd, UI_DEV_DESTROY) < 0)
		die("error: ioctl");

	close(fd);
	fclose(fd_stdin);

	return 0;
}
