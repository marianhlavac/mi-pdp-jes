#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <stack>

#define BUFFER_MAX  256
#ifndef SHRT_MAX 
#define SHRT_MAX 32767 
#endif

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

        ~Game() {
            free(grid);
        }

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

            if (file.fail()) {
                throw std::runtime_error("File doesn't exist.");
            }

            char * line = (char*) malloc(sizeof(char) * BUFFER_MAX);
            short pieces = 0;

            // Read first line
            file >> game.dimension;
            file >> game.upper_bound;
            file.getline(line, BUFFER_MAX);

            // Allocate memory for the grid
            int items_count = game.dimension * game.dimension;
            game.grid = (short*) malloc(sizeof(short) * items_count);

            // Read the grid
            int line_num = 0;
            while (file.getline(line, BUFFER_MAX)) {
                for (int i = 0; i <= game.dimension; ++i) {
                    int idx = i + (game.dimension * line_num);
                    game.grid[idx] = line[i] - '0';

                    switch (game.grid[idx]) {
                        case 1: pieces++; break;
                        case 3: 
                            game.start_coord = i + line_num * game.dimension;
                        break;
                    }
                }
                line_num++;
            }

            game.pieces = pieces;
            free(line);
            return game;
        }
};

struct Solution {
    std::vector<short> path;
    short pieces_left;
    bool valid = false;

    Solution(short pieces, short upper_bound) : pieces_left(pieces), valid(true) {
        path.reserve(upper_bound);
    }
    Solution() : valid(false) { }

    /**
     * Gets the price of this solution.
     */
    size_t get_size() const {
        return valid ? path.size() : SHRT_MAX;
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
        if (!valid) return "Solution {\n  Invalid\n}";
        std::stringstream ss;
        ss << "Solution {" << std::endl << "  path: ";
        for (short node : path) {
            ss << node << ", ";
        }
        ss << std::endl << "  pieces_left: " << pieces_left << "," << std::endl;
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
        void get_available_steps(Solution* current, std::vector<Solution*> &steps) {
            short dim = game->dimension;
            short last = current->get_last();
            steps.clear();

            for (int i = 0; i < KNIGHT_MOVES; ++i) {
                const short* move = KNIGHT_MOVES_COORDS[i];

                // Check if it doesn't overflows the gird
                short xpos = last % dim + move[0];
                short ypos = last / dim + move[1];
                if (xpos < 0 || xpos >= dim || ypos < 0 || ypos >= dim) {
                    continue;
                }

                Solution* next = new Solution(*current);
                short coords = last + move[0] + dim * move[1];
                next->path.push_back(coords);

                // Check if stepped on a piece
                if (game->is_piece_at(coords)) {
                    if (find(current->path.begin(), current->path.end(), coords) == current->path.end()) {
                        next->pieces_left--;
                    }
                }

                // Apply slight heuristic operator
                // sort(steps.begin(), steps.end(), [](const Solution * a, const Solution * b) -> bool { 
                //     return a->get_size() < b->get_size(); 
                // });

                steps.push_back(next);
            }
        }

    public:
        Solver(Game* instance) : game(instance) { }

        /**
         * Tries to solve the problem using BB-DFS algorithm.
         * 
         * @returns Best found solution.
         */
        Solution solve() {
            std::deque<Solution*> stack;
            std::vector<Solution*> steps;
            Solution* best_solution = new Solution();

            Solution* root = new Solution(game->pieces, game->upper_bound);
            root->path.push_back(game->start_coord);
            stack.push_back(root);

            while (!stack.empty()) {
                Solution* node = stack.back();
                stack.pop_back();

                // Check if not off limits or useless (pruning)
                if (node->get_size() + (game->pieces - node->pieces_left) >= best_solution->get_size() || 
                    node->get_size() >= game->upper_bound) {
                    continue;
                }

                // Find each available next step
                get_available_steps(node, steps);
                for (Solution* next : steps) {
                    if (next->pieces_left == 0) {
                        // Found solution, compare to others
                        if (next->get_size() - best_solution->get_size()) {
                            best_solution = next;
                        }

                        // 
                    } else {
                        // Push to stack to be explored further
                        stack.push_back(next);
                    }
                }
            }

            for (Solution* sol : stack) { delete sol; }
            stack.clear();

            return *best_solution;
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

    // Clean up

    
    return 0;
}