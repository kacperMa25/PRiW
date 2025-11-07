#include <fstream>
#include <iostream>

const int iX = 6;
const int iY = 6;
char maze[6][6];

int main()
{
    std::fstream file("labirynt.txt");
    std::string text;
    char delimiter = ',';

    int x = 0;
    while (getline(file, text)) {
        char stringStart = 0;
        char stringEnd = text.find(delimiter);
        for (int i = 0; i < iY; ++i) {
            std::cout << text.substr(stringStart, stringEnd) << " ";
            stringStart = stringEnd + 1;
            stringEnd = text.find(delimiter, stringStart - 1);
        }
        std::cout << std::endl;
    }

    return 0;
}
