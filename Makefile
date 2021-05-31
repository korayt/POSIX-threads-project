CC = gcc
TARGET = main
N = 4
P = 0.75
Q = 5
T = 3
B = 0.05

all: clean install

install:
	$(CC) -o $(TARGET) $(TARGET).c
	./$(TARGET) -n $(N) -p $(P) -q $(Q) -t $(T) -b $(B)
clean:
	$(RM) $(TARGET)