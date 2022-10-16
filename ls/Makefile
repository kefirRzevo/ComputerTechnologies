CXX    = gcc
TARGET = myls
OBJ    = obj

all:
	$(CXX) -c -o $(OBJ)/main.o  main.c
	$(CXX) -o $(TARGET) $(OBJ)/main.o

clean:
	rm $(OBJ)/*.obj $(TARGET)
