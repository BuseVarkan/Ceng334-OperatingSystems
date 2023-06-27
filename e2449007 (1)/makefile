CC = gcc
CFLAGS = -g

OBJS = bgame.o logging.o message.o

bgame: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o bgame

bgame.o: bgame.c logging.h message.h
	$(CC) $(CFLAGS) -c bgame.c

logging.o: logging.c logging.h
	$(CC) $(CFLAGS) -c logging.c

message.o: message.c message.h
	$(CC) $(CFLAGS) -c message.c
