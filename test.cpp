#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

enum Tile {
    Wall,
    Empty
};

struct Cell {
    int x;
    int y;
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
        double fov = M_PI/3;
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

class Ray {
    public:
        double originX;
        double originY;
        double directionX;
        double directionY;
        Ray(Player &player) {
            originX = player.getXPos();
            originY = player.getYPos();
            directionX = std::cos(player.angle);
            directionY = std::sin(player.angle);
        }
};

struct Hit {
    double perpDist;
    int side;
    int mapX;
    int mapY;
};

void clearScreen();
void drawTopDownMap(Map &map, Player &player, const std::vector<Cell> &rayCells);
Hit steppingDDA(Ray &ray, Map &map, int maxSteps = 1024);
bool isVisited(const std::vector<Cell> &cells, int x, int y);
std::vector<Cell> steppingDDApath(Ray &ray, Map &map, int maxSteps);

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
    const int screenW = 120;
    const int screenH = 40;
    std::string frame(screenW * screenH, ' ');
    for (;;) {
        std::fill(frame.begin(), frame.end(), ' ');
        for (int x = 0; x < screenW; x++) {
            double cameraX = 2.0 * x / double(screenW) - 1.0;
            double rayAngle = player.angle + std::atan(cameraX * std::tan(player.fov / 2.0));
            Ray ray(player);
            ray.directionX = std::cos(rayAngle);
            ray.directionY = std::sin(rayAngle);
            Hit h = steppingDDA(ray, map, 1024);
            int lineH = int(screenH / h.perpDist);
            int drawStart = -lineH / 2 + screenH / 2;
            int drawEnd = lineH / 2 + screenH / 2;
            if (drawStart < 0) {
                drawStart = 0;
            }
            if (drawEnd >= screenH) {
                drawEnd = screenH - 1;
            }
            char wall = (h.side == 0) ? '#' : '|';
            for (int y = drawStart; y <= drawEnd; y++) {
                frame[y * screenW + x] = wall;
            }
        }
        clearScreen();
        for (int y = 0; y < screenH; y++) {
            std::cout.write(&frame[y * screenW], screenW);
            std::cout << '\n';
        }
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
        std::cout.flush();
    }
    return 0;
}

void drawTopDownMap(Map &map, Player &player, const std::vector<Cell> &rayCells) {
    int pX = std::floor(player.getXPos());
    int pY = std::floor(player.getYPos());
    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            if (pX == j && pY == i) {
                std::cout << "P";
            } 
            else if (map.isWall(j, i)) {
                std::cout << "#";
            }
            else if (isVisited(rayCells, j, i)) {
                std::cout << "*";
            }
            else {
                std::cout << ".";
            }
        }
        std::cout << "\n";
    }
}

void clearScreen() {
    std::cout << "\x1B[2J\x1B[H";
}

Hit steppingDDA(Ray &ray, Map &map, int maxSteps) {
    int mapX = floor(ray.originX);
    int mapY = floor(ray.originY);
    int stepX = (ray.directionX < 0) ? -1 : 1;
    int stepY = (ray.directionY < 0) ? -1 : 1;
    const double INF = 1e30;
    double deltaDistX = (ray.directionX == 0.0) ? INF : std::abs(1.0/ray.directionX);
    double deltaDistY = (ray.directionY == 0.0) ? INF : std::abs(1.0/ray.directionY);
    double sideDistX;
    double sideDistY;

    if (ray.directionX < 0) {
        sideDistX = (ray.originX - mapX) * deltaDistX;
    }
    else {
        sideDistX = (mapX + 1.0 - ray.originX) * deltaDistX;
    }

    if (ray.directionY < 0) {
        sideDistY = (ray.originY - mapY) * deltaDistY;
    }
    else {
        sideDistY = (mapY + 1.0 - ray.originY) * deltaDistY;
    }
    int side = 0; 

    for (int i = 0; i < maxSteps; i++) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } 
        else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        if (map.isWall(mapX, mapY)) {
            break;
        }
    }
    double perpDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
    if (perpDist < 0.001) {
        perpDist = 0.001;
    }
    return Hit{perpDist, side, mapX, mapY};
}



std::vector<Cell> steppingDDApath(Ray &ray, Map &map, int maxSteps) {
    std::vector<Cell> visited;
    int mapX = floor(ray.originX);
    int mapY = floor(ray.originY);
    int stepX = (ray.directionX < 0) ? -1 : 1;
    int stepY = (ray.directionY < 0) ? -1 : 1;
    const double INF = 1e30;
    double deltaDistX = (ray.directionX == 0.0) ? INF : std::abs(1.0/ray.directionX);
    double deltaDistY = (ray.directionY == 0.0) ? INF : std::abs(1.0/ray.directionY);
    double sideDistX;
    double sideDistY;

    if (ray.directionX < 0) {
        sideDistX = (ray.originX - mapX) * deltaDistX;
    }
    else {
        sideDistX = (mapX + 1.0 - ray.originX) * deltaDistX;
    }

    if (ray.directionY < 0) {
        sideDistY = (ray.originY - mapY) * deltaDistY;
    }
    else {
        sideDistY = (mapY + 1.0 - ray.originY) * deltaDistY;
    }
    int side = 0; 

    for (int i = 0; i < maxSteps; i++) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } 
        else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        visited.push_back({mapX, mapY});
        if (map.isWall(mapX, mapY)) {
            break;
        }

    }
    return visited;
}

bool isVisited(const std::vector<Cell> &cells, int x, int y) {
    for (const auto &c : cells) {
        if (c.x == x && c.y == y) {
            return true;
        }
    }
    return false;
}
