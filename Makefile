# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 2224

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.c
server:
	gcc $(CFLAGS) server.c -lm -o server

# Compileaza client.c
subscriber: 
	gcc subscriber.c -o subscriber

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server: server
	./server ${PORT}

# Ruleaza clientul
run_client: subscriber
	./subscriber 123 ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
