CC=/usr/local/opt/llvm/bin/clang

LDFLAGS=-L/usr/local/opt/llvm/lib -lstdc++ -shared-libgcc
CPPFLAGS=-I/usr/local/opt/llvm/include -std=c++11

SRC=src/main.cpp

pdpjes:
	OMPI_CC=/usr/local/opt/llvm/bin/clang mpicc $(SRC) $(CPPFLAGS) -fopenmp -O3 -o bin/pdpjes $(LDFLAGS)

dev:
	OMPI_CC=/usr/local/opt/llvm/bin/clang mpicc $(SRC) $(CPPFLAGS) -fopenmp -O0 -g -o bin/pdpjes $(LDFLAGS)

atstar:
	mpicc $(SRC) $(CPPFLAGS) -fopenmp -O3 -o bin/pdpjes $(LDFLAGS)

run: pdpjes
	./bin/pdpjes $(FILE)

clean:
	rm -rf bin