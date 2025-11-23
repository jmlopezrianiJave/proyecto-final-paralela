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

const int DX[4] = {-1, 0, 1, 0};  // N, E, S, W
const int DY[4] = { 0, 1, 0,-1};

bool inside(int x, int y) {
    return (x >= 0 && x < R && y >= 0 && y < C);
}

// --- Utilidades para vecinos ---

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

// Selección determinista del vecino (misma que en el secuencial)
pair<int,int> select_target(
    const vector<pair<int,int>>& cand,
    int G, int x, int y
) {
    if (cand.empty()) return {-1, -1};
    int P = (int)cand.size();
    int idx = (G + x + y) % P;
    return cand[idx];
}

// --- Colocación con resolución de conflictos (paralela, con locks por celda) ---

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
    // Si hay ROCK o FOX, no deberíamos intentarlo
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
    // Si es roca, no hacemos nada (mantenemos la roca)
    if (cell.type == ROCK) {
        // no cambiamos la roca
    } else if (cell.type == EMPTY || cell.type == RABBIT) {
        cell.type = FOX;
        cell.fox.proc_age = proc_age;
        cell.fox.food_age = food_age;
    } else if (cell.type == FOX) {
        // 1) mayor proc_age
        // 2) si empatan, menor food_age (menos hambriento)
        bool replace = false;
        if (proc_age > cell.fox.proc_age) {
            replace = true;
        } else if (proc_age == cell.fox.proc_age &&
                   food_age < cell.fox.food_age) {
            replace = true;
        }
        if (replace) {
            cell.fox.proc_age = proc_age;
            cell.fox.food_age = food_age;
        }
    }
    omp_unset_lock(&cell_lock[x][y]);
}

// --- Movimiento de conejos (paralelo) ---

vector<vector<Cell>> move_rabbits(
    const vector<vector<Cell>>& world_curr,
    int G
) {
    vector<vector<Cell>> world_next(R, vector<Cell>(C));

    // Inicializar world_next: copiar rocas y zorros, limpiar conejos
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            world_next[i][j].type = cell.type;

            if (cell.type == ROCK) {
                // nada más
            } else if (cell.type == FOX) {
                world_next[i][j].fox = Fox(cell.fox.proc_age, cell.fox.food_age);
            }

            // en el siguiente mundo iniciamos sin conejos
            world_next[i][j].rabbit.proc_age = 0;

            // si hay conejo en world_curr, vaciamos la casilla en world_next
            if (cell.type == RABBIT) {
                world_next[i][j].type = EMPTY;
            }
        }
    }

    // Mover todos los conejos en paralelo
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            if (cell.type != RABBIT) continue;

            int age = cell.rabbit.proc_age;  // edad ANTES de la generación
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

            // Edad del padre DESPUÉS de la generación
            int parent_age_after = age + 1;
            if (can_procreate) {
                parent_age_after = 0;
            }

            // Colocar al padre
            place_rabbit(world_next, tx, ty, parent_age_after);

            // Si procrea, dejar hijo en el origen
            if (can_procreate) {
                place_rabbit(world_next, ox, oy, 0);
            }
        }
    }

    return world_next;
}

// --- Movimiento de zorros (paralelo) ---

vector<vector<Cell>> move_foxes(
    const vector<vector<Cell>>& world_curr,
    int G
) {
    vector<vector<Cell>> world_next(R, vector<Cell>(C));

    // Inicializar world_next: copiar rocas y conejos, limpiar zorros
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            world_next[i][j].type = cell.type;

            if (cell.type == ROCK) {
                continue;
            }

            if (cell.type == RABBIT) {
                world_next[i][j].rabbit.proc_age = cell.rabbit.proc_age;
            }

            // En el siguiente mundo empezamos sin zorros
            world_next[i][j].fox.proc_age  = 0;
            world_next[i][j].fox.food_age = 0;

            if (cell.type == FOX) {
                // Quitamos el zorro de la celda; se recolocará según movimiento
                world_next[i][j].type = EMPTY;
            }
        }
    }

    // Mover todos los zorros en paralelo
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            if (cell.type != FOX) continue;

            int ox = i, oy = j;
            int proc_age = cell.fox.proc_age;   // edad de reproducción ANTES
            int food_age = cell.fox.food_age;   // hambre ANTES

            int tx = i, ty = j;
            bool moved = false;
            bool alive = true;
            int new_food = food_age;

            // 1. Intentar comer conejo
            auto rabbits = get_adjacent_of_type(i, j, world_curr, RABBIT);
            if (!rabbits.empty()) {
                auto p = select_target(rabbits, G, i, j);
                tx = p.first;
                ty = p.second;
                moved = true;
                new_food = 0; // comió
            } else {
                // 2. No hay conejo adyacente -> aumenta hambre
                new_food = food_age + 1;
                if (new_food >= GEN_FOOD_FOXES) {
                    // muere de hambre antes de intentar moverse a celda vacía
                    alive = false;
                } else {
                    // 3. Intentar moverse a celda vacía
                    auto empties = get_adjacent_of_type(i, j, world_curr, EMPTY);
                    if (!empties.empty()) {
                        auto p = select_target(empties, G, i, j);
                        tx = p.first;
                        ty = p.second;
                        moved = true;
                    } else {
                        // Se queda donde está
                        tx = i;
                        ty = j;
                        moved = false;
                    }
                }
            }

            if (!alive) {
                // No se coloca en world_next
                continue;
            }

            // La condición de reproducción usa la edad ANTES de la generación
            bool can_procreate = (proc_age >= GEN_PROC_FOXES) && moved;

            int proc_age_after = proc_age + 1;
            if (can_procreate) {
                proc_age_after = 0;
            }

            // Colocar al padre
            place_fox(world_next, tx, ty, proc_age_after, new_food);

            // Si procrea, dejar cría en la posición original
            if (can_procreate) {
                place_fox(world_next, ox, oy, 0, 0);
            }
        }
    }

    return world_next;
}

// --- Salida ---

void print_world(const vector<vector<Cell>>& world) {
    vector<tuple<string,int,int>> objects;
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world[i][j];
            if (cell.type == ROCK) {
                objects.emplace_back("ROCK", i, j);
            } else if (cell.type == RABBIT) {
                objects.emplace_back("RABBIT", i, j);
            } else if (cell.type == FOX) {
                objects.emplace_back("FOX", i, j);
            }
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
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (!(cin >> GEN_PROC_RABBITS >> GEN_PROC_FOXES >> GEN_FOOD_FOXES
              >> N_GEN >> R >> C >> N)) {
        return 0;
    }

    vector<vector<Cell>> world(R, vector<Cell>(C));
    // Inicializar locks por celda para escritura concurrente segura
    cell_lock.assign(R, vector<omp_lock_t>(C));
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            omp_init_lock(&cell_lock[i][j]);
        }
    }

    for (int k = 0; k < N; ++k) {
        string obj;
        int x, y;
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

    for (int G = 0; G < N_GEN; ++G) {
        world = move_rabbits(world, G);
        world = move_foxes(world, G);
    }

    print_world(world);
    // Destruir locks
    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            omp_destroy_lock(&cell_lock[i][j]);
        }
    }
    return 0;
}
