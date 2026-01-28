#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <chrono>
#include <thread>

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
int rampIndexFromDistance(double d, double maxDist);
char rampChar(int idx);
const std::string RAMP = "@%#*+=-:. ";
const int RAMP_LEN = RAMP.size();
const double MAX_DEPTH = 10.0;

static termios g_oldTerm{};

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &g_oldTerm);
    termios raw = g_oldTerm;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTerm);
}

int readKey() {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1) {
        return c;
    }
    return -1;
}

int main() {
    enableRawMode();
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
    const double step = 0.2;
    const double turnRad = 0.05;
    const int screenW = 120;
    const int screenH = 40;
    std::string frame(screenW * screenH, ' ');
    for (;;) {
        std::fill(frame.begin(), frame.end(), ' ');
        
        for (int y = 0; y < screenH; y++) {
            for (int x = 0; x < screenW; x++) {
                if (y < screenH / 2) {
                    frame[y * screenW + x] = ' ';
                }
                else {
                    double floorDist = (double)screenH / (2.0 * y - screenH);
                    int fIdx = rampIndexFromDistance(floorDist, MAX_DEPTH);
                    frame[y * screenW + x] = (fIdx < RAMP_LEN - 3) ? '.' : ' '; 
                }
            }
        }


        double dirX = std::cos(player.angle);
        double dirY = std::sin(player.angle);
        double planeX = -dirY * std::tan(player.fov / 2.0);
        double planeY = dirX * std::tan(player.fov / 2.0);
        for (int x = 0; x < screenW; x++) {
            double cameraX = 2.0 * x / double(screenW) - 1.0;
            Ray ray(player);
            ray.directionX = dirX + planeX * cameraX;
            ray.directionY = dirY + planeY * cameraX;
            Hit h = steppingDDA(ray, map, 1024);
            double hitX;
            if (h.side == 0) {
                hitX = ray.originY + h.perpDist * ray.directionY;
            }
            else {
                hitX = ray.originX + h.perpDist * ray.directionX;
            }
            hitX -= std::floor(hitX);
            double d = h.perpDist;
            const double aspect = 0.8;
            int lineH = int((screenH * aspect) / h.perpDist);
            int drawStart = -lineH / 2 + screenH / 2;
            int drawEnd = lineH / 2 + screenH / 2;
            if (drawStart < 0) {
                drawStart = 0;
            }
            if (drawEnd >= screenH) {
                drawEnd = screenH - 1;
            }
            int idx = rampIndexFromDistance(h.perpDist, MAX_DEPTH);

            if (h.side == 1) idx += 3;
            if (hitX < 0.25) idx += 1;
            else if (hitX > 0.75) idx -= 1;

            idx = std::clamp(idx, 0, RAMP_LEN - 1);
            int idx2 = std::min(idx + 1, RAMP_LEN - 1);
            char wall1 = rampChar(idx);
            char wall2 = rampChar(idx2);

            for (int y = drawStart; y <= drawEnd; y++) {
                frame[y * screenW + x] = wall1; 
            }
        }
        clearScreen();
        for (int y = 0; y < screenH; y++) {
            std::cout.write(&frame[y * screenW], screenW);
            std::cout << '\n';
        }
        std::cout.flush();
        int key;
        while ((key = readKey()) != -1) {
            switch (key) {
                case 'q':
                    disableRawMode();
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
        }
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    disableRawMode();
    return 0;
}


int rampIndexFromDistance(double d, double maxDist) {
    double t = d / maxDist;
    if (t > 1.0) t = 1.0;
    if (t < 0.0) t = 0.0;
    const double gamma = 1.8;
    t = std::pow(t, gamma);
    return int(t * (RAMP_LEN - 1));
}

char rampChar(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= RAMP_LEN) idx = RAMP_LEN - 1;
    return RAMP[idx];
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
    int mapX = (int)std::floor(ray.originX);
    int mapY = (int)std::floor(ray.originY);

    double rayDirX = ray.directionX;
    double rayDirY = ray.directionY;

    double deltaDistX = (rayDirX == 0.0) ? 1e30 : std::abs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0.0) ? 1e30 : std::abs(1.0 / rayDirY);

    int stepX = (rayDirX < 0) ? -1 : 1;
    int stepY = (rayDirY < 0) ? -1 : 1;

    double sideDistX = (rayDirX < 0) ? (ray.originX - mapX) * deltaDistX : (mapX + 1.0 - ray.originX) * deltaDistX;

    double sideDistY = (rayDirY < 0) ? (ray.originY - mapY) * deltaDistY : (mapY + 1.0 - ray.originY) * deltaDistY;

    int side = 0;

    for (int i = 0; i < maxSteps; i++) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }

        if (map.isWall(mapX, mapY)) break;
    }

    double perpWallDist;
    if (side == 0) {
        perpWallDist = (mapX - ray.originX + (1 - stepX) / 2.0) / rayDirX;
    } else {
        perpWallDist = (mapY - ray.originY + (1 - stepY) / 2.0) / rayDirY;
    }

    if (perpWallDist < 0.001) perpWallDist = 0.001;

    return Hit{perpWallDist, side, mapX, mapY};
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
