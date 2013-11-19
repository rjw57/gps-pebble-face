all: build

build: src/js/pebble-js-app.js
	pebble build

install:
	pebble install

clean:
	pebble clean
	rm -f src/js/pebble-js-app.js

src/js/pebble-js-app.js: src/js/main.js $(wildcard src/js/lib/*.js)
	echo "/* This file is generated automatically. DO NOT EDIT. */" >"$@"
	cat $^ >>"$@"

.PHONY: all build install clean

# vim:noet:sw=8:ts=8:sts=8
