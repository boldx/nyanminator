#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>


void type(const struct libevdev_uinput *uidev,  unsigned int code) {
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
    libevdev_set_name(dev, "fake keyboard device");
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
				
				type(uidev, KEY_SPACE);
				type(uidev, KEY_M);
				type(uidev, KEY_E);
				type(uidev, KEY_O);
				type(uidev, KEY_W);
				type(uidev, KEY_SPACE);
				
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
