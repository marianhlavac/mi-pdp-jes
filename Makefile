pdpjes:
	clang src/main.cpp -std=c++11 -g -o bin/pdpjes

run: pdpjes
	./bin/pdpjes

clean:
	rm -rf bin