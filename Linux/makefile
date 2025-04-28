CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
SRC = src/main.cpp src/lexer.cpp src/parser.cpp src/compiler.cpp
OUT = build/yenlang

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
