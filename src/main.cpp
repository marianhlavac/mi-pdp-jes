#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <stack>
#include <chrono>

#define BUFFER_MAX      256
#ifndef SHRT_MAX 
#define SHRT_MAX        32767 
#endif
#define P_DELIM         ","

typedef std::chrono::high_resolution_clock hr_clock;

const short KNIGHT_MOVES = 8;
const short KNIGHT_MOVES_COORDS[8][2] = { 
    {-1,-2}, {1,-2}, {-2,-1}, {2,-1}, {-1,2}, {1,2}, {-2,1}, {2,1}
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
            file >> game.dimension >> game.upper_bound;
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

        std::pair<short, short> to_coords(short pos) {
            return std::make_pair(pos % dimension, pos / dimension);
        }

        std::string to_coords_str(short pos) {
            std::pair<short, short> coords = to_coords(pos);
            return "(" + std::to_string(coords.first) + 
                ":" + std::to_string(coords.second) + ")";
        }
};

class Solution {
    protected:
        std::vector<short> path;
        Game* game = nullptr;

    public:
        short upper_bound = SHRT_MAX;
        bool valid = false;
        short pieces_left;

        Solution(Game* game) : 
            pieces_left(game->pieces), valid(true), 
            upper_bound(game->upper_bound), game(game) { }

        Solution(short upper_bound) : 
            upper_bound(upper_bound) { }

        Solution(const Solution* s) : 
            path(s->path), pieces_left(s->pieces_left), valid(s->valid), 
            upper_bound(s->upper_bound), game(s->game) { }

        /**
         * Gets the price of this solution.
         */
        size_t get_size() const {
            return valid ? path.size() : upper_bound;
        }

        /**
         * Gets the last node on the path.
         */
        short get_last() const {
            return path.back();
        }

        /**
         * Adds another node to the path.
         */
        void add_node(short node) {
            path.push_back(node);
        }

        bool is_in_path(short node) {
            return std::find(path.begin(), path.end(), node) != path.end();
        }

        std::string dump() {
            std::string dump = std::string(valid ? "valid" : "invalid") + P_DELIM + std::to_string(upper_bound) +
                P_DELIM + std::to_string(get_size()) + P_DELIM;

            for (short coord : path) {
                dump += game->to_coords_str(coord) + ";";
            }

            return dump;
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

                Solution* next = new Solution(current);
                short coords = last + move[0] + dim * move[1];
                next->add_node(coords);

                // Check if stepped on a piece
                if (game->is_piece_at(coords)) {
                    if (!current->is_in_path(coords)) {
                        next->pieces_left--;
                    }
                }

                steps.push_back(next);
            }
        }

    public:
        long iterations = 0;
        Solver(Game* instance) : game(instance) { }

        /**
         * Tries to solve the problem using BB-DFS algorithm.
         * 
         * @returns Best found solution.
         */
        Solution solve() {
            std::deque<Solution*> stack;
            std::vector<Solution*> steps;
            Solution* best_solution = new Solution(game->upper_bound + 1);

            Solution* root = new Solution(game);
            root->add_node(game->start_coord);
            stack.push_back(root);

            while (!stack.empty()) {
                iterations++;
                Solution* node = stack.back();
                stack.pop_back();

                // Prune
                if (node->get_size() + node->pieces_left >= best_solution->get_size()) {
                    continue;
                }

                // Find each available next step
                get_available_steps(node, steps);
                for (Solution* next : steps) {
                    if (next->pieces_left == 0) {
                        // Found solution, compare to others
                        if (next->get_size() < best_solution->get_size()) {
                            best_solution = next;
                        }
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

    // Print CSV header
    std::cout << "filename,validity,upper_bound,solution_length,solution,iterations,elapsed" << std::endl;

    // Execute and print out results
    try {
        for (int i = 1; i < argc; i++) {
            Game game = Game::create_from_file(argv[i]);
            Solver solver(&game);

            auto started_at = hr_clock::now();
            Solution solution = solver.solve();
            std::chrono::duration<double> elapsed = hr_clock::now() - started_at;

            std::cout << argv[i] << P_DELIM << solution.dump() << P_DELIM <<
                solver.iterations << P_DELIM << elapsed.count() << std::endl;
        }
    } 
    catch (std::runtime_error err) {
        std::cerr << err.what();
    }

    // Clean up

    
    return 0;
}