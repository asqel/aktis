SRC = $(wildcard src/common/*.c src/srv/*.c)
OBJ = $(SRC:.c=.o)
OBJ := $(OBJ:src/%=%)
OBJ := $(addprefix obj/, $(OBJ))

CFLAGS = -Wall -Wextra -g -Iinclude/ -DSDL_MAIN_HANDLED=1 -Ioeuf-socket/ -lSDL3 -lSDL3_image
LIBS_DLL = ./liboeso.so

CC = gcc
LD = gcc

NAME = server

all: $(LIBS_DLL) $(OBJ)
	$(LD) $(OBJ) $(CFLAGS) -o $(NAME) $(LIBS_DLL)

obj/%.o: src/%.c
	@echo compiling src/$<...
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

./liboeso.so:
	make -C oeuf-socket/
	cp oeuf-socket/liboeso.so ./

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(LIBS_DLL)
	rm -rf $(NAME)

re: fclean all
