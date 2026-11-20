HEADER_COMMON = include/common.h
CFLAGS_COMMON = -Wall -Wextra -g -Iinclude -Iraisin/

SRC_CLIENT = $(wildcard src/common/*.c src/common/*/*.c src/clt/*.c src/clt/*/*.c)
OBJ_CLIENT = $(SRC_CLIENT:.c=.o)
OBJ_CLIENT := $(OBJ_CLIENT:src/%=%)
OBJ_CLIENT := $(addprefix obj/, $(OBJ_CLIENT))
HEADER_CLIENT := include/client.h $(HEADER_COMMON)

SRC_SERVER = $(wildcard src/common/*.c src/common/*/*.c src/srv/*.c src/srv/*/*.c)
OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_SERVER := $(OBJ_SERVER:src/%=%)
OBJ_SERVER := $(addprefix obj/, $(OBJ_SERVER))
HEADER_SERVER := include/server.h $(HEADER_COMMON)

CFLAGS_CLIENT = $(CFLAGS_COMMON) -DSDL_MAIN_HANDLED=1 -mavx2 -msse2 -O3
LDFLAGS_CLIENT = -lSDL3 -lSDL3_image -lm
LIBS_SO_CLIENT = ./libra.so

CFLAGS_SERVER = $(CFLAGS_COMMON)
LDFLAGS_SERVER =
LIBS_SO_SERVER = ./libra.so


CC = gcc
LD = gcc

NAME_CLIENT = game
NAME_SERVER = server

all: $(NAME_CLIENT) $(NAME_SERVER)

$(NAME_CLIENT): $(LIBS_SO_CLIENT) $(OBJ_CLIENT)
	$(LD) $(OBJ_CLIENT) $(CFLAGS_CLIENT) $(LDFLAGS_CLIENT) -o $(NAME_CLIENT) $(LIBS_SO_CLIENT)

$(NAME_SERVER): $(LIBS_SO_SERVER) $(OBJ_SERVER)
	$(LD) $(OBJ_SERVER) $(CFLAGS_SERVER) $(LDFLAGS_SERVER) -o $(NAME_SERVER) $(LIBS_SO_SERVER)

obj/clt/%.o: src/clt/%.c $(HEADER_CLIENT)
	@echo compiling $<...
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS_CLIENT) -c $< -o $@

obj/srv/%.o: src/srv/%.c $(HEADER_SERVER)
	@echo compiling $<...
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS_SERVER)

obj/common/%.o: src/common/%.c $(HEADER_COMMON)
	@echo compiling src/$<...
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS_COMMON)

./libra.so:
	make -C raisin/
	cp raisin/libra.so ./

clean:
	rm -rf $(OBJ_CLIENT)
	rm -rf $(OBJ_SERVER)

fclean: clean
	rm -rf $(LIBS_SO_CLIENT)
	rm -rf $(LIBS_SO_SERVER)
	rm -rf $(NAME_CLIENT) $(NAME_SERVER)

re: fclean all
