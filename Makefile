PRGM_NAME = hbr
CC = gcc
CFLAGS = -g
SOURCE  = hbr.c
OBJECTS = hbr.o
INCLUDES = `xml2-config --cflags`
LFLAGS = `xml2-config --libs`

$(PRGM_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LFLAGS) -o $(PRGM_NAME) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
