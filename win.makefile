SRC = $(wildcard src/common/*.c src/clt/*.c src/clt/*/*.c)
OBJ = $(SRC:.c=.o_w)
OBJ := $(OBJ:src/%=%)
OBJ := $(addprefix obj/, $(OBJ))

CFLAGS = -Wall -Wextra -g -Iinclude/ -DSDL_MAIN_HANDLED=1 -lws2_32 -Isdl3/x86_64-w64-mingw32/include -Isdl3-image/x86_64-w64-mingw32/include -Ioeuf-socket/
LIBS_DLL = ./SDL3.dll ./liboeso.dll ./SDL3_image.dll

CC = x86_64-w64-mingw32-gcc 
LD = x86_64-w64-mingw32-gcc 

NAME = game.exe

all: $(LIBS_DLL) $(OBJ)
	$(LD) $(OBJ) $(CFLAGS) -o $(NAME) $(LIBS_DLL)

obj/%.o_w: src/%.c
	@echo compiling src/$<...
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

./SDL3.dll:
	cp sdl3/x86_64-w64-mingw32/bin/SDL3.dll ./

./liboeso.dll:
	make -C oeuf-socket/ -f win.Makefile
	cp oeuf-socket/liboeso.dll ./

./SDL3_image.dll:
	cp sdl3-image/x86_64-w64-mingw32/bin/SDL3_image.dll ./

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(LIBS_DLL)
	rm -rf $(Name)

re: fclean all

export:
	mkdir -p windows/
	cp *.dll windows/
	cp game.exe windows/
	cp font.png windows/
	zip game.zip windows/*

