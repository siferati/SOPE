all: bin/gerador

bin/gerador: gerador.c
	cc gerador.c -o bin/gerador -D_REENTRANT -lpthread -Wall

PHONY: all