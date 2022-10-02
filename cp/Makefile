CXX    = gcc
TARGET = mycp
OBJ    = obj
LIB    = lib

all:
	$(CXX) -c -o $(OBJ)/mycp.o  mycp.c
	$(CXX) -c -o $(OBJ)/mylib.o $(LIB)/mylib.c
	$(CXX) -o $(TARGET) $(OBJ)/mycp.o  $(OBJ)/mylib.o

clean:
	rm $(OBJ)/*.obj $(TARGET)
