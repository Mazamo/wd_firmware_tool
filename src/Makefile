SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
CFLAGS =	-g -Wall -fmessage-length=0 -Wno-unused-function -Wno-unused-variable
LIBS =

TARGET = 	wd_firmware_tool

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
