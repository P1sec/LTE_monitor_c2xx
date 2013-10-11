

all:
	yacc -d -t atcmd.y
	flex atcmd.l
	gcc lex.yy.c  y.tab.c atcmd.c dlpi.c  gsmtap.c  hexview.c  lte.c  modem.c  protocol.c  tap.c  tapcfg.c  taplog.c -I. `pkg-config --libs --cflags libusb-1.0` -o lte  -g -ggdb