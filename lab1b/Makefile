all: server.c client.c
	@gcc server.c -o server -lpthread -lmcrypt
	@gcc client.c -o client -lpthread -lmcrypt
client: client.c
	@gcc client.c -o client -lpthread -lmcrypt

server: server.c
	@gcc server.c -o server -lpthread -lmcrypt

PHONY.: clean

clean:
	@rm -f server client *.o *~ core

dist:
	@tar -zcvf lab1b-604588135.tar.gz *
