include nall/Makefile

application := usart
flags := -std=gnu++0x -I. -O3 -fomit-frame-pointer

all:
	g++-4.7 $(flags) -o $(application) $(application).cpp
