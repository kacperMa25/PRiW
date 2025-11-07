#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

class Maze {
public:
    Maze() = default;

    void loadFromFile(const string&);
    void printBoard();

private:
    vector<vector<int>> mazeMatrix;
    vector<thread> threads;

    void generateThreads(const int&);
    void threadTraverse(const int&);
};

void Maze::loadFromFile(const string& fileName)
{
    fstream file(fileName);
    string line;

    while (getline(file, line)) {
        vector<int> row;
        stringstream lineStream(line);
        string cell;

        while (getline(lineStream, cell, ',')) {
            row.push_back(stoi(cell));
        }
        mazeMatrix.push_back(row);
    }
}

void Maze::printBoard()
{
    for (const auto& row : mazeMatrix) {
        for (const auto& value : row) {
            cout << value << " ";
        }
        cout << endl;
    }
}

void Maze::generateThreads(const int& nrThreads)
{
}

void Maze::threadTraverse(const int& tid)
{
}

int main()
{
    Maze maze;
    maze.loadFromFile("labirynt.txt");
    maze.printBoard();

    return 0;
}
