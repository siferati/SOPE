all: bin/viatura bin/gerador

bin/viatura: viatura.c
	cc viatura.c -o bin/viatura -Wall

bin/gerador: gerador.c
	cc gerador.c -o bin/gerador -D_REENTRANT -lpthread -Wall

PHONY: all
