# My makefile
SKETCH = src/garagebox.ino

UPLOAD_PORT = /dev/ttyUSB1
BOARD = nodemcu

include $(HOME)/Arduino/espmake/makeEspArduino.mk
