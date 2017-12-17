./bin/Linux/main: src/main.cpp src/glad.c src/textrendering.cpp include/matrices.h include/utils.h include/dejavufont.h include/tiny_obj_loader.h include/stb_image.h
	mkdir -p bin/Linux
	g++ -std=c++11 -Wall -Wno-unused-function -g -I ./include/ -o ./bin/Linux/main src/main.cpp src/glad.c src/textrendering.cpp src/tiny_obj_loader.cpp src/stb_image.cpp ./lib-linux/libglfw3.a -lrt -lm -ldl -lX11 -lpthread -lXrandr -lXinerama -lXxf86vm -lXcursor -lsfml-audio -lsfml-window -lsfml-system 

.PHONY: clean run
clean:
	rm -f bin/Linux/main

run: ./bin/Linux/main
	cd bin/Linux && ./main
