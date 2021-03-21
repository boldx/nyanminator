#define _GNU_SOURCE

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include <linux/version.h>
#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"


static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name, sizeof(EVENT_DEV_NAME) - 1) == 0;
}


static int find_keyboard_event_file(char *evfname, size_t evfname_len)
{
	struct dirent **namelist;
	int ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
	if (ndev <= 0) {
		return 1;
	}
	
	int highscore = 0;
	for (int i = 0; i < ndev; i++)
	{
		char fname[256];
		if(snprintf(fname, sizeof(fname), "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name) >= sizeof(fname)) {
			continue;
		}
		
		int fd = open(fname, O_RDONLY);
		if (fd < 0) {
			continue;
		}
		
		struct libevdev *dev = NULL;
		if (libevdev_new_from_fd(fd, &dev) < 0) {
			goto release;
		}
		
		if (!libevdev_has_event_type(dev, EV_KEY)) {
			goto release;
		}
		
		unsigned int c, score = 0;
		for (c = 0; c <= KEY_MAX; c++) {
			if (libevdev_has_event_code(dev, EV_KEY, c)) score++;
		}
		
		if (strstr(libevdev_get_name(dev), "keyboard")) {
			score += 1000;
		}
		
		if (score > highscore && strlen(fname) < evfname_len) {
			highscore = score;
			strcpy(evfname, fname);
		}
		
release:
		libevdev_free(dev);
		close(fd);			
		free(namelist[i]);
	}
	
	return (highscore > 0) ? 0 : 2;
}


static void type_key(const struct libevdev_uinput *uidev,  unsigned int code) {
	libevdev_uinput_write_event(uidev, EV_KEY, code, 1);
	libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
	libevdev_uinput_write_event(uidev, EV_KEY, code, 0);
	libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
}


int main(int argc, char **argv)
{
	int rc;
	char fname[256]; 
	rc = find_keyboard_event_file(fname, sizeof(fname));
	if(rc) {
		fprintf(stderr, "Failed to find a keyboard\n");
		goto out1;
	}
	printf("Source events: %s\n", fname);

	int fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror("Failed to open keyboard event device");
		rc = errno;
	}
    
    	struct libevdev *srcdev;
	rc = libevdev_new_from_fd(fd, &srcdev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		goto out2;
	}
	printf("Source device name: \"%s\"\n", libevdev_get_name(srcdev));
	
	struct libevdev_uinput *dstdev;
	rc = libevdev_uinput_create_from_device(srcdev, LIBEVDEV_UINPUT_OPEN_MANAGED, &dstdev);
	if (rc) {
		perror("Failed to create new event device");
		goto out3;
	}
	usleep(100000);
	
	rc = libevdev_grab(srcdev, LIBEVDEV_GRAB);
	if (rc) {
   		fprintf(stderr, "Failed to grab keyboard event device\n");
   		goto out3;
   	}
	
	do {
		struct input_event ev;
		rc = libevdev_next_event(srcdev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
			libevdev_uinput_write_event(dstdev, ev.type, ev.code, ev.value);
		}
	} while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
	
	printf("OK, byee!");
	rc = 0;
out3:
	libevdev_uinput_destroy(dstdev);
	libevdev_grab(srcdev, LIBEVDEV_UNGRAB);
out2:
	libevdev_free(srcdev);
	close(fd);
out1:
	return rc;
}
