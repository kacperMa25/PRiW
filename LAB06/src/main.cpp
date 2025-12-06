#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <omp.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>

enum Direction {
    up = 1,
    down = 2,
    left = 3,
    right = 4,
    none = 5
};

std::mutex coutMutex;

struct Position {
    Position(const int _y = 0, const int _x = 0)
        : y(_y)
        , x(_x)
    {
    }
    Position(const Position& other)
        : y(other.y)
        , x(other.x)
    {
    }
    Position(const Position& other, Direction dir)
    {
        switch (dir) {
        case up:
            y = other.y - 1;
            x = other.x;
            break;

        case down:
            y = other.y + 1;
            x = other.x;
            break;

        case left:
            y = other.y;
            x = other.x - 1;
            break;

        case right:
            y = other.y;
            x = other.x + 1;
            break;
        default:
            break;
        }
    }

    void goDown()
    {
        y++;
    }

    void goUp()
    {
        y--;
    }

    void goRight()
    {
        x++;
    }

    void goLeft()
    {
        x--;
    }

    void go(Direction dir)
    {
        switch (dir) {
        case up:
            y--;
            break;

        case down:
            y++;
            break;

        case left:
            x--;
            break;

        case right:
            x++;
            break;
        default:
            break;
        }
    }

    int y;
    int x;
};

class Maze {
public:
    Maze();
    ~Maze();

    void loadFromFile(const std::string&);
    void printBoard();
    void generateMaze(int, int);
    void run();
    void printStats();

    void saveToPPM(const std::string&, const int);

private:
    std::vector<std::vector<int>> mazeMatrix;
    std::vector<std::vector<omp_lock_t>> cellLock;
    std::vector<int> childrenCount;
    omp_lock_t childrenCountMutex;
    unsigned int threadCounter = 1;
    omp_lock_t threadCounterMutex;

    void threadTraverse(const int, const Position&);

    int getPositionStatus(const Position&);
    void setPositionStatus(const Position&, int);
    bool trySettingPositionStatus(const Position&, int);
    bool trySpawningThread(const Position&);

    unsigned int getID();

    void clearCellMutex();
    void clear();

    void generateDFS(int x, int y);
};

Maze::Maze()
{
    omp_init_lock(&threadCounterMutex);
    omp_init_lock(&childrenCountMutex);
}

Maze::~Maze()
{
    clear();
    omp_destroy_lock(&threadCounterMutex);
    omp_destroy_lock(&childrenCountMutex);
}

void Maze::loadFromFile(const std::string& fileName)
{
    clear();
    std::fstream file(fileName);
    std::string line;

    while (getline(file, line)) {
        std::vector<int> row;
        std::stringstream lineStream(line);
        std::string cell;

        while (getline(lineStream, cell, ',')) {
            row.push_back(stoi(cell));
        }
        mazeMatrix.push_back(row);
    }

    cellLock.resize(mazeMatrix.size());
    for (int i = 0; i < mazeMatrix.size(); ++i) {
        cellLock[i].resize(mazeMatrix[i].size());
        for (int j = 0; j < mazeMatrix[i].size(); ++j) {
            omp_init_lock(&cellLock[i][j]);
        }
    }
}

void Maze::generateDFS(int y, int x)
{
    static const int dirs[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<int> order = { 0, 1, 2, 3 };
    std::shuffle(order.begin(), order.end(), g);

    for (int i : order) {
        int ny = y + dirs[i][0] * 2;
        int nx = x + dirs[i][1] * 2;
        if (ny > 0 && ny < mazeMatrix.size() - 1 && nx > 0 && nx < mazeMatrix[0].size() - 1 && mazeMatrix[ny][nx] == -1) {
            mazeMatrix[y + dirs[i][0]][x + dirs[i][1]] = 0;
            mazeMatrix[ny][nx] = 0;
            generateDFS(ny, nx);
        }
    }
}

void Maze::generateMaze(int h, int w)
{
    clear();
    mazeMatrix.assign(h, std::vector<int>(w, -1));

    cellLock.resize(h);
    for (int y = 0; y < h; ++y) {
        cellLock[y].resize(w);
        for (int x = 0; x < w; ++x)
            omp_init_lock(&cellLock[y][x]);
    }

    generateDFS(1, 1);
}

void Maze::printBoard()
{
    for (const auto& row : mazeMatrix) {
        for (const auto& value : row) {
            std::cout << std::setw(3) << value;
        }
        std::cout << std::endl;
    }
}

void Maze::printStats()
{
    std::cout << "Threads spawned: " << threadCounter - 1 << std::endl;
    int childrenSum = 0;
    for (int tid = 0; tid < childrenCount.size(); ++tid) {
        std::cout << "Thread " << tid + 1 << " had " << childrenCount[tid] << " children" << std::endl;
        childrenSum += childrenCount[tid];
    }
    std::cout << "Control sum: " << childrenSum + 1 << std::endl;
}

void Maze::run()
{
    Position startingPos;
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> randomX(1, mazeMatrix[0].size() - 1);
    std::uniform_int_distribution<> randomY(1, mazeMatrix.size() - 1);

    while (mazeMatrix[startingPos.y][startingPos.x] != 0) {
        startingPos.y = randomY(g);
        startingPos.x = randomX(g);
    }

    int id = getID();
    trySettingPositionStatus(startingPos, id);

#pragma omp parallel
    {
#pragma omp single
        {
            threadTraverse(id, startingPos);
        }
    }
}

void Maze::saveToPPM(const std::string& fileName, const int scale = 1)
{
    std::ofstream file(fileName);
    if (!file.is_open())
        return;

    const size_t height = mazeMatrix.size() * scale;
    const size_t width = mazeMatrix[0].size() * scale;

    file << "P3\n"
         << width << " " << height << "\n255\n";

    for (const auto& row : mazeMatrix) {
        for (int i = 0; i < scale; ++i) {
            for (const auto& cell : row) {
                unsigned r, g, b;
                if (cell == 0) {
                    r = g = b = 255;
                } else if (cell > 0) {
                    r = (255 / threadCounter) * (cell - 1);
                    g = 255 - r;
                    b = 255 - r;
                } else {
                    r = g = b = 0;
                }
                for (int j = 0; j < scale; ++j)
                    file << r << " " << g << " " << b << " ";
            }
            file << "\n";
        }
    }

    file.close();
}

void Maze::threadTraverse(const int tid, const Position& startingPos)
{
    omp_set_lock(&childrenCountMutex);
    childrenCount.push_back(0);
    omp_unset_lock(&childrenCountMutex);

    Position currentPos(startingPos);
    bool moved = true;

    while (moved) {
        Direction whereToGo = none;
        moved = false;
        for (int i = up; i != none; i++) {
            Direction dir = static_cast<Direction>(i);
            Position next = Position(currentPos, dir);

            if (moved) {
                if (trySpawningThread(next)) {
                    childrenCount[tid]++;
                }
            } else if (trySettingPositionStatus(next, tid)) {
                moved = true;
                whereToGo = dir;
            }
        }
        currentPos.go(whereToGo);
    }

    std::lock_guard<std::mutex> guard(coutMutex);
    std::cout << "Thread: " << tid << " ended" << std::endl;
}

int Maze::getPositionStatus(const Position& pos)
{
    omp_set_lock(&cellLock[pos.y][pos.x]);
    int status = mazeMatrix[pos.y][pos.x];
    omp_unset_lock(&cellLock[pos.y][pos.x]);

    return status;
}

void Maze::setPositionStatus(const Position& pos, int status)
{
    omp_set_lock(&cellLock[pos.y][pos.x]);
    mazeMatrix[pos.y][pos.x] = status;
    omp_unset_lock(&cellLock[pos.y][pos.x]);
}

bool Maze::trySettingPositionStatus(const Position& pos, int newStatus)
{
    bool set = false;
    omp_set_lock(&cellLock[pos.y][pos.x]);
    if (mazeMatrix[pos.y][pos.x] == 0) {
        mazeMatrix[pos.y][pos.x] = newStatus;
        set = true;
    }
    omp_unset_lock(&cellLock[pos.y][pos.x]);
    return set;
}

bool Maze::trySpawningThread(const Position& pos)
{
    bool spawned = false;
    omp_set_lock(&cellLock[pos.y][pos.x]);
    if (mazeMatrix[pos.y][pos.x] == 0) {
        int id = getID();
        mazeMatrix[pos.y][pos.x] = id;
#pragma omp task
        omp_unset_lock(&cellLock[pos.y][pos.x]);
        threadTraverse(id, Position(pos.y, pos.x));
        spawned = true;
    }

    if (!spawned) {
        omp_unset_lock(&cellLock[pos.y][pos.x]);
    }

    return spawned;
}

unsigned int Maze::getID()
{
    omp_set_lock(&threadCounterMutex);
    unsigned int id = threadCounter++;
    omp_unset_lock(&threadCounterMutex);
    return id;
}

void Maze::clearCellMutex()
{
    for (auto& row : cellLock) {
        for (auto& cell : row) {
            omp_destroy_lock(&cell);
        }
    }
    cellLock.clear();
}

void Maze::clear()
{
    clearCellMutex();
    mazeMatrix.clear();
}

int main()
{
    Maze maze;
    maze.generateMaze(40, 40);
    maze.run();
    maze.printBoard();
    maze.saveToPPM("maze.ppm", 16);
    maze.printStats();

    return 0;
}
