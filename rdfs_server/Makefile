.PHONY: clean
LD      := gcc
LDLIBS  := ${LDLIBS}  -lpthread

APPS    := rdfs_server

all: ${APPS}


rdfs_server: event.o rdfs_server.o main.o
	${LD} -o $@ $^ ${LDLIBS}

clean:
	rm -f *.o ${APPS}