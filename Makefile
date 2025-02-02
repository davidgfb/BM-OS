.PHONY: build install demos run clean

build:
	./baremetal.sh build

install: build
	./baremetal.sh install

demos: install
	./baremetal.sh demos

run: demos
	./baremetal.sh run

clean:
	rm -rf sys/*.sys sys/*.app sys/*.bin
