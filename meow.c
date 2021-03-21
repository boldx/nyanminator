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
		return -1;
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
		
		if (score > highscore && strlen(evfname) < evfname_len) {
			highscore = score;
			snprintf(evfname, evfname_len, "%s", fname);
		}
		
		release:
			libevdev_free(dev);
			close(fd);			
			free(namelist[i]);
	}
	
	return (highscore > 0) ? 0 : -2;
}


static void type_key(const struct libevdev_uinput *uidev,  unsigned int code) {
    libevdev_uinput_write_event(uidev, EV_KEY, code, 1);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(uidev, EV_KEY, code, 0);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
}


int main(int argc, char **argv)
{
    int err;
    struct libevdev *dev;
    struct libevdev_uinput *uidev;
    
    char fname[256]; 
    find_keyboard_event_file(fname, sizeof(fname));
    printf("%s", fname);
    //return 0;
    
    	int fd = open("/dev/input/event2", O_RDONLY);
	int rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		fprintf(stderr, "Failed to init O libevdev (%s)\n", strerror(-rc));
		exit(1);
	}
	printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
	printf("Input device ID: bus %#x vendor %#x product %#x\n",
	       libevdev_get_id_bustype(dev),
	       libevdev_get_id_vendor(dev),
	       libevdev_get_id_product(dev));

    //dev = libevdev_new();
    //libevdev_set_name(dev, "fake keyboard device");
/*
    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_code(dev, EV_KEY, KEY_A, NULL);
        libevdev_enable_event_code(dev, EV_KEY, KEY_S, NULL);
*/
    err = libevdev_uinput_create_from_device(dev,
        LIBEVDEV_UINPUT_OPEN_MANAGED,
        &uidev);

    if (err != 0)
        return err;
        
   usleep(100000);
        
   if(libevdev_grab(dev, LIBEVDEV_GRAB)) {
   	printf("grab fail");
   }

	 int c = 0;
	do {
		struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (rc == 0) {
			if(0) printf("Event: %s %s %d\n",
			       libevdev_event_type_get_name(ev.type),
			       libevdev_event_code_get_name(ev.type, ev.code),
			       ev.value);
			libevdev_uinput_write_event(uidev, ev.type, ev.code, ev.value);
			if (c++ > 30) {
				c = 0;
				
				type_key(uidev, KEY_SPACE);
				type_key(uidev, KEY_M);
				type_key(uidev, KEY_E);
				type_key(uidev, KEY_O);
				type_key(uidev, KEY_W);
				type_key(uidev, KEY_SPACE);
			}
		}
		if (rc == -EAGAIN) usleep(10000);
		
	} while (rc == 1 || rc == 0 || rc == -EAGAIN);
	
    

    libevdev_uinput_write_event(uidev, EV_KEY, KEY_A, 1);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(uidev, EV_KEY, KEY_A, 0);
    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
    
    usleep(100000);


    libevdev_uinput_destroy(uidev);
    printf("Complete\n");
}
