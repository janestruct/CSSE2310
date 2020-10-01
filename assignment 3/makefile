CC = gcc
CFLAGS = -std=gnu99 -pedantic -Wall


.PHONY: all


all: playerA playerB dealer

dealer: dealer.c
	$(CC) $(CFLAGS) dealer.c  -o 2310dealer

playerA: 2310A.o main_player.c
	$(CC) $(CFLAGS) main_player.c 2310A.o -o 2310A
    
playerB: 2310B.o main_player.c
	$(CC) $(CFLAGS) main_player.c 2310B.o -o 2310B

2310A.o: 2310A.c
	$(CC) $(CFLAGS) -c 2310A.c -o 2310A.o
    
2310B.o: 2310B.c
	$(CC) $(CFLAGS) -c 2310B.c -o 2310B.o
