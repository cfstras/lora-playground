all: run

PHONY: first second all

DEV=/dev/ttyACM0

first: DEV=/dev/ttyACM0
first: run

second: DEV=/dev/ttyACM1
second: run

run:
	platformio run -t upload --upload-port $(DEV)
	platformio device monitor --port $(DEV)