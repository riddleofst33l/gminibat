all:
	 cc main.c -o gminibat `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0`
