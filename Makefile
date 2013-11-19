all: build

build:
	pebble build

install:
	pebble install

clean:
	pebble clean

.PHONY: all build install clean

# vim:noet:sw=8:ts=8:sts=8
