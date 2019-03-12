SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
CFLAGS =	-O2 -g -Wall -fmessage-length=0
LIBS =

TARGET =	main

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
