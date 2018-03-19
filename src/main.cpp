#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <stack>
#include <chrono>
#include <algorithm>
#include <omp.h>

#define BUFFER_MAX              256
#ifndef SHRT_MAX
#define SHRT_MAX                32767 
#endif
#define P_DELIM                 ","

#define SOLUTION_VALIDATE       true

typedef std::chrono::high_resolution_clock hr_clock;

const short KNIGHT_MOVES = 8;
const short KNIGHT_MOVES_COORDS[8][2] = { 
    {-1,-2}, {1,-2}, {-2,-1}, {2,-1}, {-1,2}, {1,2}, {-2,1}, {2,1}
};

class Game {
    public:
        bool* grid;
        unsigned short dimension = 0;
        short upper_bound = 0;
        short pieces = 0;
        short start_coord = 0;

        ~Game() {
            free(grid);
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

            char* line = new char[BUFFER_MAX];
            short pieces = 0;

            // Read first line
            file >> game.dimension >> game.upper_bound;
            file.getline(line, BUFFER_MAX);

            // Allocate memory for the grid
            int items_count = game.dimension * game.dimension;
            game.grid = new bool[items_count]();

            // Read the grid
            int line_num = 0;
            while (file.getline(line, BUFFER_MAX)) {
                for (int i = 0; i <= game.dimension; ++i) {
                    int idx = i + (game.dimension * line_num);
                    if (line[i] == '1') { pieces++; game.grid[idx] = 1; }
                    if (line[i] == '3') { game.start_coord = idx; }
                }
                line_num++;
            }

            game.pieces = pieces;
            free(line);
            return game;
        }
};

class Solution {
    protected:
        std::vector<short> path;
        bool* grid = nullptr;

        std::pair<short, short> to_coords(short pos) {
            return std::make_pair(pos % dimension, pos / dimension);
        }

        std::string to_coords_str(short pos) {
            std::pair<short, short> coords = to_coords(pos);
            return "(" + std::to_string(coords.first) + 
                ":" + std::to_string(coords.second) + ")";
        }

    public:
        short upper_bound = SHRT_MAX;
        short dimension;
        short pieces_left = 0;
        bool valid;

        /**
         * Converts Game to a Solution, which can be used
         * as a starting node in solving process.
         */
        Solution(Game* game) : 
            valid(true), dimension(game->dimension),
            upper_bound(game->upper_bound), pieces_left(game->pieces) {
            copy_grid(game->grid, game->dimension * game->dimension);
        }

        Solution(short upper_bound) : 
            upper_bound(upper_bound), valid(false) { }

        Solution(const Solution* s) : 
            path(s->path), valid(s->valid), pieces_left(s->pieces_left),
            dimension(s->dimension), upper_bound(s->upper_bound) { 
            copy_grid(s->grid, s->dimension * s->dimension);
        }

        Solution() {
            throw std::runtime_error("no.");
        }

        ~Solution() {
            free(grid);
        }

        void copy_grid(bool* source, size_t size) {
            grid = new bool[size];
            std::copy(source, source + size, grid);
        }

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

        bool is_piece_at(short coords) {
            return grid[coords];
        }

        void remove_piece_at(short coords) {
            if (grid[coords]) { pieces_left--; }
            grid[coords] = 0;
        }

        void validate(Game* game) {
            int size = game->dimension * game->dimension;

            for (short i = 0; i < size; i++) {
                if (game->grid[i]) {
                    if (std::find(path.begin(), path.end(), i) == path.end()) {
                        valid = false;
                        break;
                    }
                }
            }
        }

        std::string dump() {
            std::string validity = std::string(valid ? "valid" : "invalid");
            if (!SOLUTION_VALIDATE) validity = "undef";

            std::string dump = validity + 
                P_DELIM + std::to_string(upper_bound) +
                P_DELIM + std::to_string(get_size()) + P_DELIM;

            for (short coord : path) {
                dump += to_coords_str(coord) + ";";
            }

            return dump;
        }
};

class Solver {
    protected:
        Game* game;
        Solution* best;

        /**
         * Processes the specified solution. Finds out all available next steps
         * and call recursively itself to solve in depth.
         */
        void process(Solution* current) {
            iterations++;
            size_t best_result = best->get_size();

            // Prune
            if (current->get_size() + current->pieces_left >= best_result) {
                return;
            }

            // Explore each available next step
            for (Solution* next : get_available_steps(current)) {
                if (next->pieces_left == 0) {
                    // Found solution, compare to others
                    #pragma omp critical
                    if (next->get_size() < best_result) {
                        best = next;
                    }
                } else {
                    #pragma omp task if (current->get_size() <= 3)
                    process(next);
                }
            }
        }

        /**
         * Returns all available steps for a knight on the grid, 
         * relative to current solution.
         */
        std::vector<Solution*> get_available_steps(Solution* current) const {
            std::vector<Solution*> steps;
            short dim = game->dimension;
            short last = current->get_last();

            for (int i = 0; i < KNIGHT_MOVES; ++i) {
                const short* move = KNIGHT_MOVES_COORDS[i];

                // Check if it doesn't overflows the grid
                short xpos = last % dim + move[0];
                short ypos = last / dim + move[1];
                if (xpos < 0 || xpos >= dim || ypos < 0 || ypos >= dim) {
                    continue;
                }

                Solution* next = new Solution(current);
                short coords = last + move[0] + dim * move[1];
                next->add_node(coords);
                next->remove_piece_at(coords);
                steps.push_back(next);
            }

            std::reverse(steps.begin(), steps.end());

            // sort(steps.begin(), steps.end(), [](const Solution* a, const Solution* b) -> bool
            // { 
            //     return a->pieces_left < b->pieces_left;
            // });

            return steps;
        }

    public:
        long iterations = 0;

        Solver(Game* instance) : game(instance) {
            best = new Solution(instance->upper_bound + 1);
        }

        /**
         * Tries to solve the problem using BB-DFS algorithm.
         * 
         * @returns Best found solution.
         */
        void solve() {
            Solution* root = new Solution(game);
            root->add_node(game->start_coord);

            #pragma omp parallel
                #pragma omp single
                process(root);
            
            if (SOLUTION_VALIDATE) { best->validate(game); }
        }

        Solution get_solution() {
            return *best;
        }
};

int main(int argc, char** argv) {
    // Prepare results output
    std::stringstream results;
    results << "filename,validity,upper_bound,solution_length,solution,iterations,elapsed" << std::endl;

    // Check command line arguments
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " [filename]" << std::endl;
        return 64;
    }

    // Print out OpenMP stats
    #ifdef _OPENMP
    std::cerr << "-!- OpenMP ready. "  <<
        omp_get_num_procs() << " CPUs available, " << 
        omp_get_max_threads() << " threads available on " << 
        omp_get_num_devices() << " available devices." << std::endl;
    #endif

    // Execute and print out results
    try {
        for (int i = 1; i < argc; i++) {
            std::cerr << "Calculating file " << i << "/" << argc - 1 << "\r";
            Game game = Game::create_from_file(argv[i]);
            Solver solver(&game);

            auto started_at = hr_clock::now();
            solver.solve();
            std::chrono::duration<double> elapsed = hr_clock::now() - started_at;

            Solution solution = solver.get_solution();

            results << argv[i] << P_DELIM << solution.dump() << P_DELIM <<
                solver.iterations << P_DELIM << elapsed.count() << std::endl;
        }
        std::cerr << std::endl;
    } 
    catch (std::runtime_error err) {
        std::cerr << err.what();
    }

    // Clean up
    // TODO: JUST DO IT. Yesterday you said tomorrow. Don't let your dreams be dreams.

    // Print out the results
    std::cout << results.str();
    
    return 0;
}