#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

enum Direction {
    up,
    down,
    left,
    right
};

std::mutex coutMutex;

struct Position {
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

    int y;
    int x;
};

class Maze {
public:
    Maze() = default;
    ~Maze();

    void loadFromFile(const std::string&);
    void printBoard();
    void run();

private:
    std::vector<std::vector<int>> mazeMatrix;
    std::vector<std::vector<std::mutex*>> cellLock;
    std::vector<std::thread> threads;
    std::mutex threadsMutex;
    unsigned int threadCounter = 1;
    std::mutex threadCounterMutex;

    bool trySpawningThread(const Position&);
    void threadTraverse(const int, const Position&);
    int getPositionStatus(const Position&);
    void setPositionStatus(const Position&, int);
    bool trySettingPositionStatus(const Position&, int);
    unsigned int getID();
};

Maze::~Maze()
{
    for (auto& row : cellLock) {
        for (auto& cell : row) {
            delete cell;
        }
    }
}

void Maze::loadFromFile(const std::string& fileName)
{
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

void Maze::printBoard()
{
    for (const auto& row : mazeMatrix) {
        for (const auto& value : row) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

void Maze::run()
{
    Position startingPos = Position(1, 1);

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
    Position currentPos(startingPos);
    trySettingPositionStatus(currentPos, tid);
    bool moved = true;

    while (moved) {
        moved = false;

        if (trySettingPositionStatus(Position(currentPos, down), tid)) {
            currentPos.goDown();
            moved = true;
        }

        if (!moved) {
            if (trySettingPositionStatus(Position(currentPos, up), tid)) {
                currentPos.goUp();
                moved = true;
            }
        } else {
            trySpawningThread(Position(currentPos, up));
        }

        if (!moved) {
            if (trySettingPositionStatus(Position(currentPos, left), tid)) {
                currentPos.goLeft();
                moved = true;
            }
        } else {
            trySpawningThread(Position(currentPos, left));
        }

        if (!moved) {
            if (trySettingPositionStatus(Position(currentPos, right), tid)) {
                currentPos.goRight();
                moved = true;
            }
        } else {
            trySpawningThread(Position(currentPos, right));
        }
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
        std::lock_guard<std::mutex> guard(threadsMutex);
        threads.push_back(std::thread(&Maze::threadTraverse, this, getID(), pos));
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

int main()
{
    Maze maze;
    maze.loadFromFile("labirynt.txt");
    maze.run();
    maze.printBoard();

    return 0;
}
