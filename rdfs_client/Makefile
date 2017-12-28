.PHONY: clean

#CFLAGS  := -Wall -Werror -g
LD      := g++
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := rdfs

all: ${APPS}


rdfs: network.o  rdfs.o
	${LD} -o $@ $^ ${LDLIBS}

clean:
	rm -f *.o ${APPS}