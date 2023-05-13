CC = gcc
CCFLAGS = -Wall -std=c99

## A3: Creates executible by linking together object files: A3.o stats_functions.o print_utils.o
A3: A3.o stats_functions.o print_utils.o
	$(CC) $(CCFLAGS) -o A3 A3.o stats_functions.o print_utils.o -lm

## A3.o: Creates object file A3.o by compiling A3.c
A3.o: A3.c
	$(CC) $(CCFLAGS) -c A3.c

## stats_functions.o: Creates object file stats_functions.o by compiling stats_functions.c
stats_functions.o: stats_functions.c stats_functions.h A3.h
	$(CC) $(CCFLAGS) -c stats_functions.c

## print_utils.o: Creates object file print_utils.o by compiling print_utils.c
print_utils.o: print_utils.c print_utils.h stats_functions.h A3.h
	$(CC) $(CCFLAGS) -c print_utils.c

## clean: Deletes all object files
.PHONY: clean help
clean:
	rm *.o

help: makefile
	@sed -n 's/^##//p' $<