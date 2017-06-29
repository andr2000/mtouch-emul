adb kill-server
adb start-server
adb exec-out cat /dev/input/event0 | ./mtouch-emul
