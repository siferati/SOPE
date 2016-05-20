all: bin/gerador

bin/gerador: bin/gerador.o bin/viatura.o
	cc -o bin/gerador bin/gerador.o bin/viatura.o -lpthread -Wall

bin/gerador.o: gerador.c viatura.h
	cc -c gerador.c -o bin/gerador.o -D_REENTRANT -lpthread -Wall

bin/viatura.o: viatura.c viatura.h
	cc -c viatura.c -o bin/viatura.o -Wall

clean:
	-rm bin/gerador bin/gerador.o bin/viatura.o

.PHONY: all clean
