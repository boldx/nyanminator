Nyanminator
===

This is a program that occasionally meows at you as you type on your keyboard.


Inspiration
---

I got jelaous of Marina Fujiwara's [The Meow Keyboard](https://www.youtube.com/watch?v=ihUHBtyPNLM).
But it seemed like too much of a hassle to modify an actual keyboard. So I did this instead.
It is similar in quality to the original. :P


Build & run
---
On Ubuntu 20.04:
```
sudo apt install build-essential libevdev-dev
gcc nyanminator.c $(pkg-config --cflags --libs libevdev)
sudo ./a.out
```

On Alpine 3.13:
```
# enable community in /etc/apk/repositories
apk add clang gcc musl-dev libevdev-dev linux-headers
clang nyanminator.c -I /usr/include/libevde-1.0/ -levdev
modprobe uinput
./a.out
```
You got the idea.


Usage
---
Start the program as root. ;) Use your keyboard.
Tere are two modes: the default is when it meows only occasionally after a space, the other is when it replaces every key press with a meow (or nya).
You can switch between them by typing ```nya```.


Known issues
---
This is a very naive Thing:
- It handles only one keyboard.
- It fucks with the scan codes directry, if your keyboard layout is not QWERTY then it will not work properly.


 .／|、     \
(ﾟ､ 。7     \
︱ ︶ヽ     \
_U U c )ノ


References
---
https://unix.stackexchange.com/questions/400744/how-to-proxy-key-presses \
https://www.kernel.org/doc/html/v4.12/input/uinput.html \
https://github.com/kernc/logkeys \
https://gitlab.freedesktop.org/libevdev/libevdev \
https://gitlab.freedesktop.org/libevdev/evtest \
https://www.youtube.com/watch?v=cjoJh1LBe68

