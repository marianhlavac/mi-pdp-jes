CC=/usr/local/opt/llvm/bin/clang
LDFLAGS=-L/usr/local/opt/llvm/lib -lstdc++ -shared-libgcc
CPPFLAGS=-I/usr/local/opt/llvm/include -std=c++11

SRC=src/main.cpp

pdpjes:
	$(CC) $(SRC) $(CPPFLAGS) -fopenmp -O3 -o bin/pdpjes $(LDFLAGS)
	$(CC) $(SRC) $(CPPFLAGS) -O3 -o bin/pdpjes-seq $(LDFLAGS)

dev:
	$(CC) $(SRC) $(CPPFLAGS) -fopenmp -O0 -g -o bin/pdpjes $(LDFLAGS)
	$(CC) $(SRC) $(CPPFLAGS) -O0 -g -o bin/pdpjes-seq $(LDFLAGS)

retarded:
	gcc $(SRC) -std=c++11 -fopenmp -O3 -o bin/pdpjes

run: pdpjes
	./bin/pdpjes $(FILE)

clean:
	rm -rf bin