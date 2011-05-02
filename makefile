all:
	 cc main.c -o gminibat -Os `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0`
install: all
	install -Dm 755 gminibat /usr/bin
