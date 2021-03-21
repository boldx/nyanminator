/*
                                   _             _             
 _ __  _   _  __ _ _ __  _ __ ___ (_)_ __   __ _| |_ ___  _ __ 
| '_ \| | | |/ _` | '_ \| '_ ` _ \| | '_ \ / _` | __/ _ \| '__|
| | | | |_| | (_| | | | | | | | | | | | | | (_| | || (_) | |   
|_| |_|\__, |\__,_|_| |_|_| |_| |_|_|_| |_|\__,_|\__\___/|_|   
       |___/  
*/

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"


struct key_seq {
	size_t len;
	int seq[8];
};


struct key_seq meow_seqs[] = {
	{.len = 3, .seq = {KEY_N, KEY_Y, KEY_A}},
	{.len = 5, .seq = {KEY_M, KEY_E, KEY_O, KEY_W}}
};


struct key_seq mode_switch_seq = {.len = 3, .seq = {KEY_N, KEY_Y, KEY_A}};


static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name, sizeof(EVENT_DEV_NAME) - 1) == 0;
}


static int find_keyboard_event_file(char *evfname, size_t evfname_len)
{
	struct dirent **namelist;
	int ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, NULL);
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


struct buff {
	int *buff;
	int tail;
	size_t len;
}; 


static int buff_init(struct buff *buff, size_t len)
{
	if ((len & (len - 1)) != 0) {
		return 1;
	}
	buff->buff = calloc(len, sizeof(*buff->buff));
	if (buff->buff == NULL) {
		return 2;
	}
	buff->tail = 0;
	buff->len = len;
	return 0;
}



static void buff_free(struct buff *buff)
{
	buff->len = 0;
	if (buff->buff == NULL) {
		return;
	}
	free(buff->buff);
}



static void buff_put(struct buff *buff, int ent)
{
	buff->buff[buff->tail] = ent;
	buff->tail = (buff->tail + 1) & (buff->len - 1);
}


static int buff_endswith(struct buff *buff, int *pattern, size_t pattern_len)
{
	if (pattern_len > buff->len) {
		return 0;
	}
	int i;
	for(i = 0; i < pattern_len; i++) {
		int act = buff->buff[(buff->tail - i - 1 + buff->len) & (buff->len - 1)];
		int exp = pattern[pattern_len - i - 1];
		if (act != exp) {
			break;
		}
	}
	return i == pattern_len;
}


static int next_threshold(int threshold) 
{
	switch(threshold) {
		case 0:
			threshold = 34;
			break;
		case 34:
			threshold = 55;
			break;
		case 55:
			threshold = 89;
			break;
		default:
			threshold = 256 + rand() % 256;	
	}
	return threshold;
}



static void do_meow(struct libevdev_uinput *dstdev)
{
	struct key_seq seq = meow_seqs[rand() % COUNT_OF(meow_seqs)];
	for(int i = 0; i < seq.len; i++) {
		type_key(dstdev, seq.seq[i]);
	}
	type_key(dstdev, KEY_SPACE);
}


static int handle_evdev_event(struct input_event *ev, struct libevdev_uinput *dstdev)
{
	static int mode, cnt, threshold;
	static struct buff buff;

	if (buff.buff == NULL) {
		buff_init(&buff, 8);
	}
	
	if (ev->type == EV_KEY && ev->value > 0) {
		buff_put(&buff, ev->code);
		cnt++;
	}
	
	if (mode == 0 && cnt >= threshold && ev->type == EV_KEY && ev->code == KEY_SPACE && ev->value != 1) {
		libevdev_uinput_write_event(dstdev, ev->type, ev->code, ev->value);
		do_meow(dstdev);
		cnt = 0;
		threshold = next_threshold(threshold);
	} else if (mode == 1 && ev->type == EV_KEY && ev->value != 1) {
		do_meow(dstdev);
	} else if (mode == 0) {
		libevdev_uinput_write_event(dstdev, ev->type, ev->code, ev->value);
	}
	
	if (ev->type == EV_KEY && ev->value == 0 && buff_endswith(&buff, mode_switch_seq.seq, mode_switch_seq.len)) {
		libevdev_uinput_write_event(dstdev, ev->type, ev->code, ev->value);
		libevdev_uinput_write_event(dstdev, EV_SYN, SYN_REPORT, 0);
		mode ^= 1;
		threshold = 0;
	}
	
	return 0;
}


int main(int argc, char **argv)
{
	srand(time(NULL)); 
	
	int rc;
	char fname[256];
	rc = find_keyboard_event_file(fname, sizeof(fname));
	if (rc) {
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
			handle_evdev_event(&ev, dstdev);
		}
	} while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
	
	printf("OK, byee!");

out3:
	libevdev_uinput_destroy(dstdev);
	libevdev_grab(srcdev, LIBEVDEV_UNGRAB);
out2:
	libevdev_free(srcdev);
	close(fd);
out1:
	return rc;
}
