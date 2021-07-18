CC=gcc
CFLAGS=-g -Wall
OBJS=20161634.o shell_cmd.o mem_cmd.o opcode.o assembler.o loader.o
TARGET=20161634.out

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

20161634.o: 20161634.c shell_cmd.h mem_cmd.h opcode.h assembler.h defines.h loader.o
shell_cmd.o: shell_cmd.c shell_cmd.h
mem_cmd.o: mem_cmd.c mem_cmd.h
opcode.o: opcode.c opcode.h 
assembler.o: assembler.c assembler.h
loader.o: loader.c loader.h

clean:
	rm -f *.o
	rm -f $(TARGET)

