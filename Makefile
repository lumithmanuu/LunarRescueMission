CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = lunar_rescue
SRC = main.cpp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -framework OpenGL -framework GLUT
else
    LDFLAGS = -lGL -lGLU -lglut
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
