test: main.o
	g++ -o test main.o -Lsrc/lib -lsfml-graphics -lsfml-window -lsfml-system
main.o: main.cpp
	g++ -c main.cpp -Isrc/include