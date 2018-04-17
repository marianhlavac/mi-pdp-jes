#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <vector>
#include <stack>
#include <queue>
#include <chrono>
#include <algorithm>
#include <mpi.h>
#include <omp.h>
#include <thread>

#define BUFFER_MAX              256
#define SHRT_MAX                32767 
#define P_DELIM                 ","
#define SYS_THR_INIT            40
#define SYS_MPI_GEN             512
#define SOLUTION_VALIDATE       true
#define DEBUG_NODISTRIBUTE      true

#define C_TAG_WORK              100
#define C_TAG_FINISH            101
#define C_TAG_IMDONE            103
#define C_TAG_IMREADY           104

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

        Game(bool* grid, unsigned short dimension, short up_bound) : grid(grid),
            dimension(dimension), upper_bound(up_bound) { }

        ~Game() {
            delete[] grid;
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
            std::ifstream file;
            int dim, bound;
            file.open(filename);

            if (file.fail()) {
                throw std::runtime_error("File doesn't exist.");
            }

            char* line = new char[BUFFER_MAX];
            short pieces = 0;

            // Read first line
            file >> dim >> bound;
            file.getline(line, BUFFER_MAX);

            // Create the game object
            int items_count = dim * dim;
            Game game(new bool[items_count](), dim, bound);

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
            delete[] line;
            return game;
        }
};

class Solution {
    protected:
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
        std::vector<short> path;

        /**
         * Converts Game to a Solution, which can be used
         * as a starting node in solving process.
         */
        Solution(Game* game) : 
            valid(true), dimension(game->dimension),
            upper_bound(game->upper_bound), pieces_left(game->pieces) {
            copy_grid(game->grid, game->dimension * game->dimension);
            add_node(game->start_coord);
        }

        Solution(Game* game, short* path, size_t path_length) :
            valid(true), dimension(game->dimension),
            upper_bound(game->upper_bound), pieces_left(game->pieces) {
            copy_grid(game->grid, game->dimension * game->dimension);
            for (int i = 0; i < path_length; ++i) {
                add_node(path[i]);
            }
            updateGrid();
        }

        Solution(short upper_bound) : 
            upper_bound(upper_bound), valid(false) { 
        }

        Solution(const Solution* s) : 
            path(s->path), valid(s->valid), pieces_left(s->pieces_left),
            dimension(s->dimension), upper_bound(s->upper_bound) { 
            copy_grid(s->grid, s->dimension * s->dimension);
        }

        Solution() {
            throw std::runtime_error("no.");
        }

        ~Solution() {
            delete[] grid;
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
        
        void invalidate() {
            valid = false;
        }

        void updateGrid() {
            for (short coords : path) {
                grid[coords] = 0;
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

            dump += std::to_string(pieces_left);

            return dump;
        }

        short* path_to_arr() {
            return &path[0];
        }
};

class Solver {
    protected:
        Game* game;
        Solution* best;

        std::vector<Solution*> process_node(Solution* current) {
            std::vector<Solution*> explore;

            // Prune
            if (current->get_size() + current->pieces_left >= best->get_size()) {
                delete current;
                return explore;
            }

            // Explore each available next step
            for (Solution* next : get_available_steps(current)) {
                if (next->pieces_left == 0) {
                    #pragma omp critical
                    suggest_solution(next);
                } else {
                    explore.push_back(next);
                }
            }

            delete current;
            return explore;
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

            return steps;
        }

    public:
        long iterations = 0;

        Solver(Game* instance) : game(instance) {
            best = new Solution(instance->upper_bound + 1);
        }

        ~Solver() {
            // TODO: Sanitize BEST
        }

        std::deque<Solution*> generate_queue(Solution* root, int count) {
            std::deque<Solution*> queue;
            queue.push_back(root);

            // Generate some states to parallel explore
            while (queue.size() < count) {
                if (queue.size() == 0) {
                    break;
                }

                Solution* current = queue.front();
                queue.pop_front();

                for (Solution* next : process_node(current)) {
                    queue.push_back(next);
                }
            }

            return queue;
        }

        void solve() {
            return solve(new Solution(game));
        }

        /**
         * Tries to solve the problem using BB-DFS algorithm.
         * 
         * @returns Best found solution.
         */
        void solve(Solution* root) {
            std::deque<Solution*> queue = generate_queue(root, SYS_THR_INIT);

            #pragma omp parallel for default(shared)
            for (int i = 0; i < queue.size(); i++) {
                solve_seq(queue[i]);
            }
        }

        void solve_seq(Solution* root) {
            std::stack<Solution*> stack;
            stack.push(root);

            while (!stack.empty()) {
                Solution* current = stack.top();
                stack.pop();

                for (Solution* next : process_node(current)) { 
                    stack.push(next);
                }
            }
        }

        Solution* get_solution() {
            if (SOLUTION_VALIDATE) { best->validate(game); }
            return best;
        }

        void suggest_solution(Solution* solution) {
            if (solution->valid && solution->get_size() < best->get_size()) {
                best = solution;
            }
        }
};

void master(int world_size, int argc, char** argv) {
    // Prepare results output
    std::stringstream results;
    results << 
        "filename,validity,upper_bound,solution_length,solution,iterations,elapsed"
        << std::endl;

    for (int i = 1; i < argc; i++) {
        Game game = Game::create_from_file(argv[i]);
        Solver solver(&game);
        std::deque<Solution*> queue;
        MPI_Status status;
        int working = 0;

        // Broadcast this filename
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(argv[i], strlen(argv[i]), MPI_CHAR, 0, MPI_COMM_WORLD);

        // Generate some states for the distribution
        if (DEBUG_NODISTRIBUTE) { 
            queue.push_back(new Solution(&game));
        } else {
            queue = solver.generate_queue(new Solution(&game), SYS_MPI_GEN);
        }

        // Measure the time
        auto started_at = hr_clock::now();

        // Distribute work to slaves
        do {
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == C_TAG_IMDONE) {
                working--;
                int path_size;
                MPI_Get_count(&status, MPI_SHORT, &path_size);

                short path[path_size];
                MPI_Recv(path, path_size, MPI_SHORT, status.MPI_SOURCE, 
                    C_TAG_IMDONE, MPI_COMM_WORLD, &status);
                
                Solution* solution = new Solution(&game, path, path_size);
                solution->validate(&game);
                solver.suggest_solution(solution);
            } else {
                MPI_Recv(nullptr, 0, MPI_BYTE, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            }

            if ((status.MPI_TAG == C_TAG_IMDONE || status.MPI_TAG == C_TAG_IMREADY) && queue.size() > 0) {
                size_t size = queue.front()->get_size();
                short* message = new short[size + 2];
                message[size] = solver.get_solution()->get_size();
                message[size + 1] = queue.front()->pieces_left; 
                std::copy(queue.front()->path.begin(), queue.front()->path.end(), message);
                MPI_Send(message, size + 2, MPI_SHORT, status.MPI_SOURCE, C_TAG_WORK, MPI_COMM_WORLD);
                queue.pop_front();
                working++;
            }
        } while (working > 0);

        std::chrono::duration<double> elapsed = hr_clock::now() - started_at;

        // Announce that's all for this file (shame I can't broadcast tags)
        for (int i = 1; i < world_size; ++i) {
            MPI_Send(nullptr, 0, MPI_BYTE, i, C_TAG_FINISH, MPI_COMM_WORLD);
        }

        // Gather results
        Solution* solution = solver.get_solution();
        results << argv[i] << P_DELIM << solution->dump() << P_DELIM <<
            solver.iterations << P_DELIM << elapsed.count() << std::endl;
    }

    // Announce that's all of the files
    MPI_Barrier(MPI_COMM_WORLD);
    char end[1] = "";
    MPI_Bcast(&end, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Print out the results
    std::cout << results.str();
}

bool slave(int world_rank) {
    char filename[BUFFER_MAX];

    // Wait for broadcasted filename
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&filename, BUFFER_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

    // If that's all of files, end running
    if (strlen(filename) == 0) { return false; }

    // Load the file
    Game game = Game::create_from_file(filename);
    Solver solver(&game);

    // Wait for tasks
    MPI_Status status;
    MPI_Send(nullptr, 0, MPI_BYTE, 0, C_TAG_IMREADY, MPI_COMM_WORLD);

    while (true) {
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == C_TAG_FINISH) {
            MPI_Recv(nullptr, 0, MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            break;
        }

        if (status.MPI_TAG == C_TAG_WORK) {
            int state_size; 
            MPI_Get_count(&status, MPI_SHORT, &state_size);

            short* state = new short[state_size];
            MPI_Recv(state, state_size, MPI_SHORT, 0, C_TAG_WORK, MPI_COMM_WORLD, &status);
            Solution* root = new Solution(&game, state, state_size - 2);
            root->pieces_left = state[state_size - 1];
            Solution* suggestion = new Solution(state[state_size - 2]);
            solver.suggest_solution(suggestion);
            int breakpoint = root->path[0] + root->path[1] + root->path[2];

            solver.solve(root);

            Solution* solution = solver.get_solution();
            size_t solution_size = solution->get_size();
            if (solution->valid) {
                MPI_Send(solution->path_to_arr(), solution_size, MPI_SHORT, 0, C_TAG_IMDONE, MPI_COMM_WORLD);
            } else {
                MPI_Send(nullptr, 0, MPI_SHORT, 0, C_TAG_IMDONE, MPI_COMM_WORLD);
            }
        }
    }

    return true;
}

int main(int argc, char** argv) {
    int world_rank = 0;
    int world_size;

    // Check command line arguments
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " [filename]" << std::endl;
        return 64;
    }

    // Initialize MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Branch out the program execution
    if (world_rank == 0) {
        master(world_size, argc, argv); 
    } else {
        while (slave(world_rank)) { }
    }

    // Finalize MPI
    MPI_Finalize();
    
    return 0;
}