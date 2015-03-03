CC      = gcc
CFLAGS  = -Wall -std=c99 `pkg-config --cflags glib-2.0 libxml-2.0`
LDFLAGS = `pkg-config --libs libxml-2.0`

mxtool: mxutil.o mxtool.o
	$(CC) -o mxtool mxutil.o mxtool.o $(CFLAGS) $(LDFLAGS)

mxtoold: mxutild.o mxtoold.o
	$(CC) -o mxtool mxutil.o mxtool.o $(CFLAGS) -g -O0 $(LDFLAGS)

mxutil.o: mxutil.c mxutil.h
	$(CC) $(CFLAGS) -c mxutil.c

mxtool.o: mxtool.c mxtool.h mxutil.h
	$(CC) $(CFLAGS) -c mxtool.c

mxutild.o: mxutil.c mxutil.h
	$(CC) $(CFLAGS) -g -c mxutil.c

mxtoold.o: mxtool.c mxtool.h mxutil.h
	$(CC) $(CFLAGS) -g -c mxtool.c

clean:
	rm -rf *o mxtool
