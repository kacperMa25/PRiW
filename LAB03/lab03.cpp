#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class Maze {
public:
  Maze() = default;

  void loadFromFile(const string &);
  void printBoard();

private:
  vector<vector<int>> mazeMatrix;
};

void Maze::loadFromFile(const string &fileName) {
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

void Maze::printBoard() {
  for (const auto &row : mazeMatrix) {
    for (const auto &value : row) {
      cout << value << " ";
    }
    cout << endl;
  }
}

int main() {
  Maze maze;
  maze.loadFromFile("labirynt.txt");
  maze.printBoard();

  return 0;
}
