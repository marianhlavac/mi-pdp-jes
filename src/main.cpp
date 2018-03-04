#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <stack>

#define BUFFER_MAX  256

const short KNIGHT_MOVES = 8;
const short KNIGHT_MOVES_COORDS[8][2] = { 
    {-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}
};

class Game {
    protected:
        short* grid;

    public:
        unsigned short dimension = 0;
        short upper_bound = 0;
        short pieces = 0;
        short start_coord = 0;

        Game() { }
        Game(short* board) : grid(board) { }

        bool is_piece_at(short coords) {
            return grid[coords] == 1;
        }
        
        /**
         * Creates a game board from the specified file.
         * 
         * The file must contain a two numbers on the first line - dimension
         * of the board and upper bound property. Other lines specifies
         * the game board, '0' stands for empty space, '1' for black piece
         * and '3' for knight piece (there must be only one knight).
         */
        static Game create_from_file(const char* filename) {
            Game game;
            std::ifstream file;
            file.open(filename);
            short pieces = 0;

            if (file.fail()) {
                throw std::runtime_error("File doesn't exist.");
            }

            char * line = (char*) malloc(sizeof(char) * BUFFER_MAX);

            // Read first line
            file >> game.dimension;
            file >> game.upper_bound;
            file.getline(line, BUFFER_MAX);

            game.grid = (short*) malloc(sizeof(short) * game.dimension);

            // Read the rest
            int line_num = 0;
            while (file.getline(line, BUFFER_MAX)) {
                for (int i = 0; i <= game.dimension; ++i) {
                    int idx = i + (game.dimension * line_num);
                    game.grid[idx] = line[i] - '0';
                    if (line[i] == '1') { pieces++; }
                    if (line[i] == '3') { 
                        game.start_coord = i + line_num * game.dimension; 
                    }
                }
                line_num++;
            }

            game.pieces = pieces;

            return game;
        }
};

struct Solution {
    std::vector<short> path;
    short pieces_left;
    bool valid = false;

    Solution(short pieces) : pieces_left(pieces), valid(true) { }
    Solution() : valid(false) { }

    /**
     * Gets the price of this solution.
     */
    size_t get_price() const {
        return valid ? path.size() : 32767;
    }

    /**
     * Gets the last node on the path.
     */
    short get_last() const {
        return path.back();
    }

    /**
     * Dumps the contents of the struct for debugging purposes.
     */
    std::string dump() {
        std::stringstream ss;
        ss << "Solution {" << std::endl << "  path: ";
        for (short node : path) {
            ss << node << ", ";
        }
        ss << std::endl << "  pieces_left: " << pieces_left << std::endl;
        ss << "}" << std::endl;
        return ss.str();
    }
};

class Solver {
    protected:
        Game* game;

        /**
         * Returns all available steps for a knight on the grid, 
         * relative to current solution.
         */
        std::vector<Solution> all_available_steps(const Solution &current) {
            std::vector<Solution> steps;
            short dim = game->dimension;
            short last = current.get_last();

            for (int i = 0; i < KNIGHT_MOVES; ++i) {
                const short* move = KNIGHT_MOVES_COORDS[i];

                // Check if it doesn't overflows the gird
                short xpos = last % dim + move[0];
                short ypos = last / dim + move[1];
                if (xpos < 0 || xpos >= dim || ypos < 0 || ypos >= dim) {
                    continue;
                }

                Solution next = current;
                short coords = last + move[0] + dim * move[1];
                next.path.push_back(coords);

                // Check if stepped on a piece
                if (game->is_piece_at(coords)) {
                    if (find(current.path.begin(), current.path.end(), coords) == current.path.end()) {
                        next.pieces_left--;
                    }
                }

                steps.push_back(next);
            }

            return steps;
        }

    public:
        Solver(Game* instance) : game(instance) { }

        /**
         * Tries to solve the problem using BB-DFS algorithm.
         * 
         * @returns Best found solution.
         */
        Solution solve() {
            std::stack<Solution> stk;
            Solution best_solution;

            Solution root(game->pieces);
            root.path.push_back(game->start_coord);
            stk.push(root);

            while (!stk.empty()) {
                Solution node = stk.top();
                stk.pop();
                

                // Check if not off limits or useless (pruning)
                if (/* off_limits || useless */node.get_price() >= 11) {
                    continue;
                }

                // Find each available next step
                for (Solution next : all_available_steps(node)) {
                    if (next.pieces_left == 0) {
                        // Found solution, compare to others
                        if (next.get_price() < best_solution.get_price()) {
                            best_solution = next;
                        }
                    } else {
                        // Push to stack to be explored further
                        stk.push(next);
                    }
                }
            }

            return best_solution;
        }
};

int main(int argc, char** argv) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " [filename]" << std::endl;
        return 64;
    }
    const char* filename = argv[1];

    // Execute
    // try {
        Game game = Game::create_from_file(filename);
        Solver solver(&game);

        Solution solution = solver.solve();

        std::cout << "Solution is: " << solution.dump() << std::endl;
    // } 
    // catch (std::runtime_error err) {
    //     std::cerr << err.what();
    // }
    
    return 0;
}