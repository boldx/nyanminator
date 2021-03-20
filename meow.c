#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/uinput.h>

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   write(fd, &ie, sizeof(ie));
}

void type(int fd, int code)
{
   emit(fd, EV_KEY, code, 1);
   emit(fd, EV_SYN, SYN_REPORT, 0);
   emit(fd, EV_KEY, code, 0);
   emit(fd, EV_SYN, SYN_REPORT, 0);
} 

int main(void)
{
   struct uinput_setup usetup;

   int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
   int rfd = open("/dev/input/event2", O_RDONLY  | O_NONBLOCK );

   /*
    * The ioctls below will enable the device that is about to be
    * created, to pass key events, in this case the space key.
    */
    int keys[] = {KEY_M, KEY_E, KEY_O, KEY_W, KEY_A, KEY_S, KEY_D };
   ioctl(fd, UI_SET_EVBIT, EV_KEY);
   ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);
   for(int i = 0; i < 7; i++) ioctl(fd, UI_SET_KEYBIT, keys[i]);            

   memset(&usetup, 0, sizeof(usetup));
   usetup.id.bustype = BUS_USB;
   usetup.id.vendor = 0x1234; /* sample vendor */
   usetup.id.product = 0x5678; /* sample product */
   strcpy(usetup.name, "Example device");

   ioctl(fd, UI_DEV_SETUP, &usetup);
   ioctl(fd, UI_DEV_CREATE);

   /*
    * On UI_DEV_CREATE the kernel will create the device node for this
    * device. We are inserting a pause here so that userspace has time
    * to detect, initialize the new device, and can start listening to
    * the event, otherwise it will not notice the event we are about
    * to send. This pause is only needed in our example code!
    */
   sleep(1);

struct input_event ie;
   /* Key press, report the event, send key release, and report again */
   while(1) {
	   if (read(rfd, &ie, sizeof(struct input_event)) <= 0) {
	    usleep(100000);
	    continue;
	   }
	  	printf("KEY %d\n", ie.code);
	  	usleep(1000000);
	  	type(fd, ie.code + 1);
   }

   for(int i = 0; i < 4; i++) type(fd, keys[i]);
   /*
    * Give userspace some time to read the events before we destroy the
    * device with UI_DEV_DESTOY.
    */
   sleep(1);

   ioctl(fd, UI_DEV_DESTROY);
   close(fd);

   return 0;
}
