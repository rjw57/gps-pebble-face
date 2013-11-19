all: build

build: src/js/pebble-js-app.js
	pebble build

install: build
	pebble install

clean:
	pebble clean
	rm -f src/js/pebble-js-app.js

src/js/pebble-js-app.js: src/js/build.js $(wildcard src/js/app/*.js) $(wildcard src/js/lib/*.js)
	r.js -o "$<"

.PHONY: all build install clean

# vim:noet:sw=8:ts=8:sts=8
