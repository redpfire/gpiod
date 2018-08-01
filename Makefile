
all:
	cc -o gpiod gpiod.c

install:
	install -m755 gpiod /usr/bin
	mkdir -p /etc/gpiod
	install -m754 start.sh /etc/gpiod

systemd:
	install -m644 gpiod.service /etc/systemd/system

