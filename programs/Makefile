# paths
INCLUDE = ../include
MODULES = ../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -Werror -g -I$(INCLUDE)
FLAGS = -lpthread

# Αρχεία .o
OBJS = parent.o $(MODULES)/child.o $(MODULES)/functions.o

# Το εκτελέσιμο πρόγραμμα
EXEC = prog

ARGS = random_text.txt 2 3

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(FLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

val:
	valgrind ./$(EXEC) $(ARGS)