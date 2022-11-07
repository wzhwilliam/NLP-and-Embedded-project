CC=gcc
CFLAGS = -g -Wextra -Wall -Werror -lm -Wno-error
EXEC = ./pc-terminal
COMM_DIR = ../communication

default:
	$(CC) $(CFLAGS) -o $(EXEC) pc_terminal.c $(COMM_DIR)/protocol.c $(COMM_DIR)/joy.c -lrt
	
clean:
	rm $(EXEC)

run: default
	$(EXEC)
