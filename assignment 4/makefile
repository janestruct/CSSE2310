.PHONY: all


all: roc control mapper

roc: roc2310.c
	gcc roc2310.c -std=gnu99 -Wall -pedantic -lpthread -o roc2310

control: control2310.c
	gcc control2310.c -std=gnu99 -Wall -pedantic -lpthread -o control2310

mapper: mapper2310.c
	gcc mapper2310.c -std=gnu99 -Wall -pedantic -lpthread -o mapper2310
