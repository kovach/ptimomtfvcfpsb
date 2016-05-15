main.o: main.cpp
	g++ -c main.cpp

app: main.o
	g++ main.o -o app -lsfml-graphics -lsfml-window -lsfml-system
