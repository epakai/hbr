PRGM_NAME = hbr
CC = gcc
CFLAGS =-g -Wall -Wextra
SOURCE  = hbr.c
OBJECTS = hbr.o
INCLUDES = `xml2-config --cflags`
LFLAGS = `xml2-config --libs`

default: hbr test-gen

$(PRGM_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LFLAGS) -o $(PRGM_NAME) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test generated templates are well formed according to the DTD
test-gen: hbr
	./hbr -g 2 > .test-gen.xml
	- xmllint --valid .test-gen.xml -o /dev/null
	rm .test-gen.xml

install: hbr
	cp hbr /usr/local/bin/hbr

clean:
	rm hbr.o hbr
