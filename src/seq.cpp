#include <bits/stdc++.h>
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

void place_rabbit(
    vector<vector<Cell>>& world_next,
    int x, int y,
    int proc_age
) {
    Cell &cell = world_next[x][y];
    if (cell.type == EMPTY) {
        cell.type = RABBIT;
        cell.rabbit.proc_age = proc_age;
    } else if (cell.type == RABBIT) {
        if (proc_age > cell.rabbit.proc_age) {
            cell.rabbit.proc_age = proc_age;
        }
    }
}

void place_fox(
    vector<vector<Cell>>& world_next,
    int x, int y,
    int proc_age,
    int food_age
) {
    Cell &cell = world_next[x][y];

    if (cell.type == ROCK) {
        return;
    }

    if (cell.type == EMPTY || cell.type == RABBIT) {
        cell.type = FOX;
        cell.fox.proc_age = proc_age;
        cell.fox.food_age = food_age;
    } else if (cell.type == FOX) {
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
}

vector<vector<Cell>> move_rabbits(
    const vector<vector<Cell>>& world_curr,
    int G
) {
    vector<vector<Cell>> world_next(R, vector<Cell>(C));

    for (int i = 0; i < R; ++i) {
        for (int j = 0; j < C; ++j) {
            const Cell &cell = world_curr[i][j];
            world_next[i][j].type = cell.type;

            if (cell.type == ROCK) {
                continue;
            }

            if (cell.type == FOX) {
                world_next[i][j].fox = Fox(cell.fox.proc_age, cell.fox.food_age);
            }

            world_next[i][j].rabbit.proc_age = 0;

            if (cell.type == RABBIT) {
                world_next[i][j].type = EMPTY;
            }
        }
    }

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

            bool can_procreate =
                (age >= GEN_PROC_RABBITS) && moved;

            int parent_age_after = age + 1;
            if (can_procreate) {
                parent_age_after = 0;
            }

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

            world_next[i][j].fox.proc_age = 0;
            world_next[i][j].fox.food_age = 0;

            if (cell.type == FOX) {
                world_next[i][j].type = EMPTY;
            }
        }
    }

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
                        tx = i;
                        ty = j;
                        moved = false;
                    }
                }
            }

            if (!alive) {
                continue;
            }

            bool can_procreate =
                (proc_age >= GEN_PROC_FOXES) && moved;

            int proc_age_after = proc_age + 1;
            if (can_procreate) {
                proc_age_after = 0;
            }

            place_fox(world_next, tx, ty, proc_age_after, new_food);

            if (can_procreate) {
                place_fox(world_next, ox, oy, 0, 0);
            }
        }
    }

    return world_next;
}

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
    return 0;
}
