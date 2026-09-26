CXX      = g++
CXXFLAGS = -O2 -std=c++17
PORT     = 8888

ifeq ($(OS),Windows_NT)
    TARGET = bank_server.exe
    LIBS   = -lws2_32
else
    TARGET = bank_server
    LIBS   = -pthread
endif

all: run

$(TARGET): bank_server.cpp
	$(CXX) $(CXXFLAGS) bank_server.cpp -o $(TARGET) $(LIBS)

run: $(TARGET)
	./$(TARGET) $(PORT)

clean:
	rm -f bank_server bank_server.exe

.PHONY: all run clean