all: server sensor

server:
	$(MAKE) -C src/server

sensor:
	$(MAKE) -C src/sensor

packing-test:
	$(MAKE) -C test
