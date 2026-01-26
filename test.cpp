#include <iostream>

enum Tile {
    Wall,
    Empty
};

class Map {
    public:
        int width;
        int height;
        Map(int x, int y) {
            width = x;
            height = y;
        }
        Tile Grid[width*height];
        Tile getTile(int gx, int gy) {
            return Grid[gy*width + gx];
        }
        bool isWall(int gx, int gy) {
            if (getTile(gx, gy) == Wall || gx >= width || gx < 0 || gy >= height || gy < 0) {
                return true;
            } 
            return false;
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
        void Move(double xa, double ya, Map* map) {
            if (map.isWall(X + xa, Y + ya) {
                return;
            } else {
                X += xa;
                Y += ya;
            }
        }
};


void drawMap(int x, int y);

int main() {
    drawMap(5, 5);
    return 0;
}

void drawMap(int x, int y) {
    for (int i = 0; i < y; i++) {
        for (int j = 0; j < x; j++) {
            std::cout << "#";
        }
        std::cout << "\n";
    }
}
