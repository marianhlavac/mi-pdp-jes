pdpjes:
	gcc src/main.cpp -lstdc++ -shared-libgcc -std=c++11 -g -O0 -o bin/pdpjes

run: pdpjes
	./bin/pdpjes $(FILE)

clean:
	rm -rf bin