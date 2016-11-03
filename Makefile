PRGM_NAME = hbr
CC = gcc
CFLAGS =-g -Wall -Wextra
SOURCE  = gen_xml.c xml.c hbr.c 
OBJECTS = gen_xml.o xml.o hbr.o
DEPS = gen_xml.h xml.h
INCLUDES = `xml2-config --cflags`
LFLAGS = `xml2-config --libs`

default: hbr test-gen

$(PRGM_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LFLAGS) -o $(PRGM_NAME) $(OBJECTS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test generated templates are well formed according to the DTD
test-gen: hbr
	./$(PRGM_NAME) -g 2 > .test-gen.xml
	xmllint --valid .test-gen.xml -o /dev/null
	rm .test-gen.xml

code-analysis: clang-analyzer cppcheck vera

clang-analyzer:
	- clang $(INCLUDES) --analyze $(SOURCE) $(DEPS)
	- rm *.plist

cppcheck:
	- cppcheck $(SOURCE) $(DEPS)

VERA_RULES = -R L001 -R L003 -R L004 -R L005 -R T001 -R T002 -R T003 -R T004 -R T005 -R T006 -R T007 -R T008 -R T009 -R T010 -R T013 -R T016 -R T019
vera:
	- vera++ $(VERA_RULES) -s $(SOURCE) $(DEPS)

install: hbr
	cp hbr /usr/local/bin/hbr

clean:
	- rm *.o *.gch hbr
