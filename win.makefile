SRC = $(wildcard src/common/*.c src/clt/*.c src/clt/*/*.c)
OBJ = $(SRC:.c=.o_w)
OBJ := $(OBJ:src/%=%)
OBJ := $(addprefix obj/, $(OBJ))

CFLAGS = -Wall -Wextra -g -Iinclude/ -DSDL_MAIN_HANDLED=1 -lws2_32 -ISDL/include/ -ISDL3_image/ -Iraisin/
LIBS_DLL = ./SDL3.dll ./libra.dll ./SDL3_image.dll

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
	cp SDL/bin/SDL3.dll ./

./libra.dll:
	make -C raisin/ -f win.Makefile
	cp raisin/libra.dll ./

./SDL3_image.dll:
	cp SDL/bin/SDL3_image.dll ./

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(LIBS_DLL)
	rm -rf $(Name)

re: fclean all

dl-sdl:
	mkdir -p SDL/
	curl -L https://www.libsdl.org/release/SDL3-devel-3.4.20-mingw.tar.gz -o SDL/SDL3.tar.gz
	curl -L https://www.libsdl.org/projects/SDL_image/release/SDL3_image-devel-3.4.8-mingw.tar.gz -o SDL/SDL3-image.tar.gz
	cd SDL && mkdir -p tmp3 tmp_image include bin
	tar -xzf SDL/SDL3.tar.gz -C SDL/tmp3 --strip-components=1
	tar -xzf SDL/SDL3-image.tar.gz -C SDL/tmp_image --strip-components=1
	
	cp SDL/tmp3/x86_64-w64-mingw32/include/* -r SDL/include/
	cp SDL/tmp3/x86_64-w64-mingw32/bin/ -r SDL/
	cp SDL/tmp_image/x86_64-w64-mingw32/include/* -r SDL/include/
	cp SDL/tmp_image/x86_64-w64-mingw32/bin/ -r SDL/
	
	rm SDL/tmp3 SDL/tmp_image -rf
	rm -rf SDL/*.tar.gz


export:
	mkdir -p windows/
	cp *.dll windows/
	cp game.exe windows/
	cp font.png windows/
	zip game.zip windows/*

