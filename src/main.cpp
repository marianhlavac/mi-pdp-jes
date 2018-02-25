#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <chrono>

#define BUFFER_MAX  256

using namespace std;

class Game {
    protected:
        short** grid;
        int dimension;
        int upper_bound;

    public:
        Game() { }
        Game(short** board) : grid(board) { }
        
        static Game createFromFile(const char* filename) {
            Game game;
            ifstream file;
            file.open(filename);

            if (file.fail()) {
                throw runtime_error("File doesn't exist.");
            }

            char * line = (char*) malloc(sizeof(char) * BUFFER_MAX);

            // Read first line
            file >> game.dimension;
            file >> game.upper_bound;
            file.getline(line, BUFFER_MAX);

            // Init the grid
            game.grid = (short**) malloc(sizeof(short*) * game.dimension);

            // Read the rest
            int line_num = 0;
            while (file.getline(line, BUFFER_MAX)) {
                short* row = (short*) malloc(sizeof(short) * game.dimension);
                
                for (int i = 0; i <= game.dimension; ++i) {
                    row[i] = line[i] - '0';
                }

                game.grid[line_num++] = row;
            }

            return game;
        }
};

int main(int argc, char** argv) {
    // Check command line arguments
    if (argc < 2) {
        cerr << "usage: " << argv[0] << " [filename]" << endl;
        return 64;
    }
    const char* filename = argv[1];

    // Execute
    try {
        Game game = Game::createFromFile(filename);

        cout << &game;
    } 
    catch (runtime_error err) {
        cerr << err.what();
    }
    
    return 0;
}