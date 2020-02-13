.PHONY: all clean re mrproper run valgrind strace

CC = gcc
RM = rm -rf

CFLAGS = -Wall -Wextra -g # or -O2
LDFLAGS = -pthread

OBJECTS = server.o http.o main.o cgi.o
OPTIONS =

NAME = http

all: $(NAME)

clean:
	$(RM) $(OBJECTS)

mrproper: clean
	$(RM) $(NAME)

re: mrproper all

run: $(NAME)
	./$(NAME)

valgrind: $(NAME)
	valgrind ./$(NAME)

strace: $(NAME)
	strace ./$(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c config.h
	$(CC) $(CFLAGS) $(OPTIONS) -o $@ $< -c
