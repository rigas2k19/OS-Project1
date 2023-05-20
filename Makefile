
ALL: producer

producer: producer.o 
	gcc producer.o -o producer -lpthread

producer.o: producer.c
	gcc -g -Wall -c producer.c

clean:
	rm -f producer producer.o 
