#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

enum CellType { EMPTY, ROCK, RABBIT, FOX };

struct Rabbit {
    int proc_age;
    Rabbit(int a = 0) : proc_age(a) {}
};

struct Fox {
    int proc_age;
    int food_age;
    Fox(int p = 0, int f = 0) : proc_age(p), food_age(f) {}
};

struct Cell {
    CellType type;
    Rabbit rabbit;
    Fox fox;
    Cell() : type(EMPTY), rabbit(0), fox(0,0) {}
};

int GEN_PROC_RABBITS, GEN_PROC_FOXES, GEN_FOOD_FOXES;
int N_GEN, R, C, N;

vector<vector<omp_lock_t>> cell_lock;

const int DX[4] = {-1, 0, 1, 0};
const int DY[4] = { 0, 1, 0,-1};

bool inside(int x, int y) {
    return (x >= 0 && x < R && y >= 0 && y < C);
}

vector<pair<int,int>> get_adjacent_of_type(
    int x, int y,
    const vector<vector<Cell>>& world,
    CellType t
) {
    vector<pair<int,int>> res;
    for (int k = 0; k < 4; ++k) {
        int nx = x + DX[k];
        int ny = y + DY[k];
        if (inside(nx, ny) && world[nx][ny].type == t) {
            res.push_back({nx, ny});
        }
    }
    return res;
}

pair<int,int> select_target(
    const vector<pair<int,int>>& cand,
    int G, int x, int y
) {
    if (cand.empty()) return {-1, -1};
    int P = (int)cand.size();
    int idx = (G + x + y) % P;
    return cand[idx];
}

inline void place_rabbit(
    vector<vector<Cell>>& world_next,
    int x, int y,
    int proc_age
) {
    omp_set_lock(&cell_lock[x][y]);
    Cell &cell = world_next[x][y];
    if (cell.type == EMPTY) {
        cell.type = RABBIT;
        cell.rabbit.proc_age = proc_age;
    } else if (cell.type == RABBIT) {
        if (proc_age > cell.rabbit.proc_age) {
            cell.rabbit.proc_age = proc_age;
        }
    }
    omp_unset_lock(&cell_lock[x][y]);
}

inline void place_fox(
    vector<vector<Cell>>& world_next,
    int x, int y,
    int proc_age,
    int food_age
) {
    omp_set_lock(&cell_lock[x][y]);
    Cell &cell = world_next[x][y];

    if (cell.type == ROCK) {
        // keep rock
    } else if (cell.type == EMPTY || cell.type == RABBIT) {
        cell.type = FOX;
        cell.fox.proc_age = proc_age;
        cell.fox.food_age = food_age;
    } else if (cell.type == FOX) {
        bool replace = false;
        if (proc_age > cell.fox.proc_age) replace = true;
        else if (proc_age == cell.fox.proc_age &&
                 food_age < cell.fox.food_age) replace = true;

        if (replace) {
            cell.fox.proc_age = proc_age;
            cell.fox.food_age = food_age;
        }
    }
    omp_unset_lock(&cell_lock[x][y]);
}

vector<vector<Cell>> move_rabbits(
    const vector<vector<Cell>>& world_curr,
    int G
) {
    vector<vector<Cell>> world_next(R, vector<Cell>(C));

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            world_next[i][j].type = cell.type;

            if (cell.type == FOX) {
                world_next[i][j].fox = Fox(cell.fox.proc_age, cell.fox.food_age);
            }

            world_next[i][j].rabbit.proc_age = 0;

            if (cell.type == RABBIT) {
                world_next[i][j].type = EMPTY;
            }
        }
    }

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            if (cell.type != RABBIT) continue;

            int age = cell.rabbit.proc_age;
            int ox = i, oy = j;
            int tx = i, ty = j;
            bool moved = false;

            auto empties = get_adjacent_of_type(i, j, world_curr, EMPTY);
            if (!empties.empty()) {
                auto p = select_target(empties, G, i, j);
                tx = p.first;
                ty = p.second;
                moved = true;
            }

            bool can_procreate = (age >= GEN_PROC_RABBITS) && moved;

            int parent_age_after = age + 1;
            if (can_procreate) parent_age_after = 0;

            place_rabbit(world_next, tx, ty, parent_age_after);

            if (can_procreate) {
                place_rabbit(world_next, ox, oy, 0);
            }
        }
    }

    return world_next;
}


vector<vector<Cell>> move_foxes(
    const vector<vector<Cell>>& world_curr,
    int G
) {
    vector<vector<Cell>> world_next(R, vector<Cell>(C));

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            world_next[i][j].type = cell.type;

            if (cell.type == ROCK) continue;

            if (cell.type == RABBIT) {
                world_next[i][j].rabbit.proc_age = cell.rabbit.proc_age;
            }

            world_next[i][j].fox.proc_age  = 0;
            world_next[i][j].fox.food_age = 0;

            if (cell.type == FOX) {
                world_next[i][j].type = EMPTY;
            }
        }
    }

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            if (cell.type != FOX) continue;

            int ox = i, oy = j;
            int proc_age = cell.fox.proc_age;
            int food_age = cell.fox.food_age;

            int tx = i, ty = j;
            bool moved = false;
            bool alive = true;
            int new_food = food_age;

            auto rabbits = get_adjacent_of_type(i, j, world_curr, RABBIT);
            if (!rabbits.empty()) {
                auto p = select_target(rabbits, G, i, j);
                tx = p.first;
                ty = p.second;
                moved = true;
                new_food = 0;
            } else {
                new_food = food_age + 1;
                if (new_food >= GEN_FOOD_FOXES) {
                    alive = false;
                } else {
                    auto empties = get_adjacent_of_type(i, j, world_curr, EMPTY);
                    if (!empties.empty()) {
                        auto p = select_target(empties, G, i, j);
                        tx = p.first;
                        ty = p.second;
                        moved = true;
                    } else {
                        tx = i; ty = j;
                        moved = false;
                    }
                }
            }

            if (!alive) continue;

            bool can_procreate = (proc_age >= GEN_PROC_FOXES) && moved;

            int proc_age_after = proc_age + 1;
            if (can_procreate) proc_age_after = 0;

            place_fox(world_next, tx, ty, proc_age_after, new_food);

            if (can_procreate) {
                place_fox(world_next, ox, oy, 0, 0);
            }
        }
    }

    return world_next;
}


struct Stats {
    int rabbits = 0, foxes = 0, rocks = 0;
};

Stats compute_stats(const vector<vector<Cell>>& world) {
    Stats s;
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            if (world[i][j].type == RABBIT) s.rabbits++;
            else if (world[i][j].type == FOX) s.foxes++;
            else if (world[i][j].type == ROCK) s.rocks++;
        }
    }
    return s;
}

void display_world_ascii(const vector<vector<Cell>>& world, int G) {
    cout << "Generation " << G << "\n";
    cout << string(C + 2, '-') << "\n";
    for (int i = 0; i < R; ++i) {
        cout << "|";
        for (int j = 0; j < C; ++j) {
            char ch = '.';
            if (world[i][j].type == ROCK) ch = '*';
            else if (world[i][j].type == RABBIT) ch = 'R';
            else if (world[i][j].type == FOX) ch  = 'F';
            cout << ch;
        }
        cout << "|\n";
    }
    cout << string(C + 2, '-') << "\n";
}

void show_generation(
    const vector<vector<Cell>>& world,
    int G,
    vector<Stats>& stats_over_time
) {
    display_world_ascii(world, G);
    Stats s = compute_stats(world);
    if ((int)stats_over_time.size() == G) stats_over_time.push_back(s);

    cout << "Stats -> Rabbits: " << s.rabbits
         << ", Foxes: " << s.foxes
         << ", Rocks: " << s.rocks << "\n\n";
}

// --- Salida final en formato del enunciado ---
void print_world_final(const vector<vector<Cell>>& world) {
    vector<tuple<string,int,int>> objects;
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world[i][j];
            if (cell.type == ROCK) objects.emplace_back("ROCK", i, j);
            else if (cell.type == RABBIT) objects.emplace_back("RABBIT", i, j);
            else if (cell.type == FOX) objects.emplace_back("FOX", i, j);
        }
    }

    int final_N = (int)objects.size();

    cout << GEN_PROC_RABBITS << " "
         << GEN_PROC_FOXES   << " "
         << GEN_FOOD_FOXES   << " "
         << 0                << " "
         << R << " " << C << " "
         << final_N << "\n";

    for (auto &t : objects) {
        cout << get<0>(t) << " "
             << get<1>(t) << " "
             << get<2>(t) << "\n";
    }
}

int main() {
    // 1) Leer TODO del stdin redirigido (< file.in)
    if (!(cin >> GEN_PROC_RABBITS >> GEN_PROC_FOXES >> GEN_FOOD_FOXES
              >> N_GEN >> R >> C >> N)) {
        return 0;
    }

    vector<vector<Cell>> world(R, vector<Cell>(C));

    // Inicializar locks por celda
    cell_lock.assign(R, vector<omp_lock_t>(C));
    for (int i = 0; i < R; ++i)
        for (int j = 0; j < C; ++j)
            omp_init_lock(&cell_lock[i][j]);

    for (int k = 0; k < N; ++k) {
        string obj; int x, y;
        cin >> obj >> x >> y;
        if (obj == "ROCK") {
            world[x][y].type = ROCK;
        } else if (obj == "RABBIT") {
            world[x][y].type = RABBIT;
            world[x][y].rabbit.proc_age = 0;
        } else if (obj == "FOX") {
            world[x][y].type = FOX;
            world[x][y].fox.proc_age = 0;
            world[x][y].fox.food_age = 0;
        }
    }

    // 2) Abrir consola para interacción (Windows)
#ifdef _WIN32
    ifstream tty("CON");
#else
    ifstream tty("/dev/tty");
#endif
    istream &menu_in = tty.is_open() ? (istream&)tty : (istream&)cin;

    vector<Stats> stats_over_time;
    stats_over_time.reserve(N_GEN + 1);

    int current_gen = 0;
    int step = 1;

    // Mostrar gen 0
    show_generation(world, current_gen, stats_over_time);

    // Menú interactivo
    while (current_gen < N_GEN) {
        cout << "Menu: [Enter]=advance, u/+ speed up, d/- slow down, p pause, q quit\n";
        cout << "Current gen: " << current_gen << " | step: " << step << "\n> ";

        string line;
        if (!getline(menu_in, line)) line.clear();

        if (!line.empty()) {
            char cmd = (char)tolower(line[0]);
            if (cmd == 'q') break;
            if (cmd == 'u' || cmd == '+') step++;
            else if (cmd == 'd' || cmd == '-') step = max(1, step - 1);
            else if (cmd == 'p') {
                show_generation(world, current_gen, stats_over_time);
                continue;
            }
        }

        int advance = min(step, N_GEN - current_gen);
        for (int t = 0; t < advance; ++t) {
            world = move_rabbits(world, current_gen);
            world = move_foxes(world, current_gen);
            current_gen++;
            if ((int)stats_over_time.size() == current_gen)
                stats_over_time.push_back(compute_stats(world));
        }

        show_generation(world, current_gen, stats_over_time);
    }

    // Tabla completa de estadísticas
    cout << "\n=== Population counts over time ===\n";
    cout << "Gen,Rabbits,Foxes,Rocks\n";
    for (int g = 0; g < (int)stats_over_time.size(); ++g) {
        auto &st = stats_over_time[g];
        cout << g << "," << st.rabbits << "," << st.foxes << "," << st.rocks << "\n";
    }
    cout << "==================================\n\n";

    // Estado final (formato juez)
    print_world_final(world);

    // Destruir locks
    for (int i = 0; i < R; ++i)
        for (int j = 0; j < C; ++j)
            omp_destroy_lock(&cell_lock[i][j]);

    return 0;
}
