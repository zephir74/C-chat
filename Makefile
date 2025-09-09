all: server client

server: server.c
	gcc -Wall -Werror -pedantic -g server.c -o server

client: client.c
	gcc -Wall -Werror -pedantic -g client.c -o client

clean:
	rm -f server client
