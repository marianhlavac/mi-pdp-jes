CC=/usr/local/opt/llvm/bin/clang
LDFLAGS=-L/usr/local/opt/llvm/lib -lstdc++ -shared-libgcc
CPPFLAGS=-I/usr/local/opt/llvm/include -std=c++11

SRC=src/main.cpp
TARGET=bin/pdpjes

pdpjes:
	$(CC) $(SRC) $(CPPFLAGS) -fopenmp -O3 -o $(TARGET) $(LDFLAGS)

dev:
	$(CC) $(SRC) $(CPPFLAGS) -fopenmp -O0 -g -o $(TARGET) $(LDFLAGS)

noomp:
	$(CC) $(SRC) $(CPPFLAGS) $(LDFLAGS) -O0 -g -o $(TARGET)

run: pdpjes
	./bin/pdpjes $(FILE)

clean:
	rm -rf bin