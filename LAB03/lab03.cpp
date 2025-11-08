#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
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
    Position()
        : y(0)
        , x(0)
    {
    }
    Position(const int _y, const int _x)
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
    Maze() = default;
    ~Maze();

    void loadFromFile(const std::string&);
    void printBoard();
    void generateMaze(int, int);
    void run();

private:
    std::vector<std::vector<int>> mazeMatrix;
    std::vector<std::vector<std::mutex*>> cellLock;
    std::vector<std::thread> threads;
    std::mutex threadsMutex;
    unsigned int threadCounter = 1;
    std::mutex threadCounterMutex;

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

Maze::~Maze()
{
    clear();
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
            cellLock[i][j] = new std::mutex();
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
            cellLock[y][x] = new std::mutex();
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

    {
        std::lock_guard<std::mutex> guard(threadsMutex);
        threads.push_back(std::thread(&Maze::threadTraverse, this, getID(), startingPos));
    }

    for (;;) {
        std::vector<std::thread> localThreads;
        {
            std::lock_guard<std::mutex> guard(threadsMutex);
            if (threads.empty())
                break;
            localThreads.swap(threads);
        }
        for (auto& t : localThreads)
            t.join();
    }
}

void Maze::threadTraverse(const int tid, const Position& startingPos)
{
    trySettingPositionStatus(startingPos, tid);
    Position currentPos(startingPos);
    bool moved = true;

    while (moved) {
        Direction whereToGo = none;
        moved = false;
        for (int i = up; i != none; i++) {
            Direction dir = static_cast<Direction>(i);
            Position next = Position(currentPos, dir);

            if (moved) {
                trySpawningThread(next);
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
    cellLock[pos.y][pos.x]->lock();
    int status = mazeMatrix[pos.y][pos.x];
    cellLock[pos.y][pos.x]->unlock();

    return status;
}

void Maze::setPositionStatus(const Position& pos, int status)
{
    cellLock[pos.y][pos.x]->lock();
    mazeMatrix[pos.y][pos.x] = status;
    cellLock[pos.y][pos.x]->unlock();
}

bool Maze::trySettingPositionStatus(const Position& pos, int newStatus)
{
    bool set = false;
    cellLock[pos.y][pos.x]->lock();
    if (mazeMatrix[pos.y][pos.x] == 0) {
        mazeMatrix[pos.y][pos.x] = newStatus;
        set = true;
    }
    cellLock[pos.y][pos.x]->unlock();
    return set;
}

bool Maze::trySpawningThread(const Position& pos)
{
    bool spawned = false;
    cellLock[pos.y][pos.x]->lock();
    if (mazeMatrix[pos.y][pos.x] == 0) {
        int id = getID();
        mazeMatrix[pos.y][pos.x] = id;
        std::lock_guard<std::mutex> guard(threadsMutex);
        threads.push_back(std::thread(&Maze::threadTraverse, this, id, pos));
        spawned = true;
    }
    cellLock[pos.y][pos.x]->unlock();

    return spawned;
}

unsigned int Maze::getID()
{
    threadCounterMutex.lock();
    unsigned int id = threadCounter++;
    threadCounterMutex.unlock();
    return id;
}

void Maze::clearCellMutex()
{
    for (auto& row : cellLock) {
        for (auto& cell : row) {
            delete cell;
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
    maze.generateMaze(10, 20);
    maze.run();
    maze.printBoard();

    return 0;
}
