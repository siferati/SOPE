all: bin/gerador bin/parque

bin/gerador: bin/gerador.o bin/viatura.o
	cc -o bin/gerador bin/gerador.o bin/viatura.o -D_REENTRANT -lpthread -Wall

bin/gerador.o: gerador.c viatura.h
	cc -c gerador.c -o bin/gerador.o -D_REENTRANT -lpthread -Wall

bin/parque: bin/parque.o bin/viatura.o
	cc -o bin/parque bin/parque.o bin/viatura.o -D_REENTRANT -lpthread -Wall

bin/parque.o: parque.c viatura.h
	cc -c parque.c -o bin/parque.o -D_REENTRANT -lpthread -Wall

bin/viatura.o: viatura.c viatura.h
	cc -c viatura.c -o bin/viatura.o -Wall

clean:
	-rm bin/gerador bin/parque bin/gerador.o bin/parque.o bin/viatura.o

.PHONY: all clean
