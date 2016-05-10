all: lab2b.c SortedList.c
	@ gcc -Wall SortedList.c lab2b.c -o lab2b -lpthread

.PHONY: clean dist

clean:
	@ rm -f *.o lab2b *~ core

dist:
	@ tar -zcvf lab2b-604588135.tar.gz *
