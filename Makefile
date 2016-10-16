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
	./$(PRGM_NAME) -g 2 > .test-gen.xml
	- xmllint --valid .test-gen.xml -o /dev/null
	rm .test-gen.xml

code-analysis: clang-analyzer cppcheck vera

clang-analyzer:
	- clang $(INCLUDES) --analyze $(SOURCE)
	- rm $(PRGM_NAME).plist

cppcheck:
	- cppcheck $(SOURCE)

VERA_RULES = -R L001 -R L003 -R L004 -R L005 -R T001 -R T002 -R T003 -R T004 -R T005 -R T006 -R T007 -R T008 -R T009 -R T010 -R T013 -R T016 -R T019
vera:
	- vera++ $(VERA_RULES) -s $(SOURCE)

install: hbr
	cp hbr /usr/local/bin/hbr

clean:
	rm hbr.o hbr
