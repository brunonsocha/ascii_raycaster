#include <cmath>
#include <vector>
#include <iostream>

enum Tile {
    Wall,
    Empty
};

class Map {
    public:
        int width;
        int height;
        Map(int x, int y) : width(x), height(y), grid(x * y, Empty) {}
        std::vector<Tile> grid;
        Tile getTile(int gx, int gy) {
            return grid[gy*width + gx];
        }
        bool isWall(int gx, int gy) {
            if (gx >= width || gx < 0 || gy >= height || gy < 0) {
                return true;
            } 
            return getTile(gx, gy) == Wall;
        }
};

class Entity {
    protected:
        double X, Y;
    public:
        double getXPos(){
            return X;
        }
        double getYPos(){
            return Y;
        }
};

class Pickup : public Entity {
    private:
        int points;
};

class Player : public Entity {
    public:
        double angle;
        Player(double x, double y, double a) {
            X = x;
            Y = y;
            angle = a;
        }
        void Rotate(double da) {
            angle += da;
            if (angle < 0) {
                angle += 2.0 * M_PI;
            }
            if (angle >= 2.0 * M_PI) {
                angle -= 2.0 * M_PI;
            }
        }

        void Move(double step, Map* map) {
            double xa = std::cos(angle) * step;
            double ya = std::sin(angle) * step;
            int xs = std::floor(X + xa);
            int ys = std::floor(Y + ya);
            if (map->isWall(xs, ys)) {
                return;
            } else {
                X += xa;
                Y += ya;
            }
        }
};

void clearScreen();
void drawTopDownMap(Map &map, Player &player);

int main() {
    const char* layout[] = {
        "####################",
        "#........#.........#",
        "#........#.........#",
        "#........#.........#",
        "#........#.........#",
        "#........#.........#",
        "#..................#",
        "#..................#",
        "#..................#",
        "####################"
    };

    Map map(20, 10);
    Player player(3.5, 4.8, 0);

    for (int y = 0; y < map.height; y++) {
        for (int x = 0; x < map.width; x++) {
            map.grid[y * map.width + x] =
                (layout[y][x] == '#') ? Wall : Empty;
        }
    }
    const double step = 0.5;
    const double turnRad = 0.1;
    for (;;) {
        drawTopDownMap(map, player);
        std::cout.flush();
        char c;
        std::cin >> c;
        switch (c) {
            case 'q':
                return 0;
            case 'w':
                player.Move(step, &map);            
                break;
            case 's':
                player.Move(-step, &map);            
                break;
            case 'd':
                player.Rotate(turnRad);            
                break;
            case 'a':
                player.Rotate(-turnRad);            
                break;
            default:
                break;
            }
        clearScreen();
        std::cout.flush();
    }
    return 0;
}

void drawTopDownMap(Map &map, Player &player) {
    int pX = std::floor(player.getXPos());
    int pY = std::floor(player.getYPos());
    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            if (pX == j && pY == i) {
                std::cout << "P";
            } 
            else {
                if (map.isWall(j, i)) {
                    std::cout << "#";
                } 
                else {
                    std::cout << ".";
                }
            }
        }
        std::cout << "\n";
    }
}

void clearScreen() {
    std::cout << "\x1B[2J\x1B[H";
}
