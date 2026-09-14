CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = test_almacenamiento
SRCS = Almacenamiento.cc test_almacenamiento.cc

$(TARGET): $(SRCS) Almacenamiento.h
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) cafeteria.img

.PHONY: run clean
