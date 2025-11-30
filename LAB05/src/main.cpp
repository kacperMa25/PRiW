#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <omp.h>

double threadExecTime[128];

bool isPrime(int);

class UlamSpiral {
public:
    UlamSpiral(int size = 4, int threads = 1);
    ~UlamSpiral();

    void changeThreadsToUse(int threads);
    void changeSize(int newSize);
    void mapPrimes(int blockSize);
    void mapPrimesNest(int blockSize);

    void printMatrix();
    void saveToPPM(std::string fileName, int scale = 1);

    int getSize() { return Size; }
    int getNumOfThreads() { return threadsToUse; }

private:
    int** Matrix;

    unsigned char*** ColorMatrix;
    unsigned char** ColorThreads;

    int Size;
    int threadsToUse;

    void applyMapping();
    void applyColorThreads();

    void allocMatrix();
    void allocColorMatrix();
    void allocColorThreads();

    void deleteMatrix();
    void deleteColorMatrix();
    void deleteColorThreads();

    int getMapping(int x, int y);
};

///////////////////////////////////////////

template <typename Func>
double runExperiment(const std::string& name, Func func, UlamSpiral& spiral, int runs, std::ofstream& csv)
{
    int maxBlockSize = spiral.getSize();
    int blockJump = 2;
    double avgTime = 0;

    for (int t = 1; t <= 16; t *= 2) {
        spiral.changeThreadsToUse(t);
        for (int block = maxBlockSize / 16; block <= maxBlockSize / 2; block *= blockJump) {

            for (int i = 0; i < spiral.getNumOfThreads(); i++) {
                threadExecTime[i] = 0.0;
            }

            avgTime = 0;

            for (int r = 0; r < runs; ++r) {

                double start = omp_get_wtime();
                func(block);
                double end = omp_get_wtime();

                avgTime += (end - start);
            }

            avgTime /= runs;

            std::cout << name << std::endl
                      << "BlockSize = " << block
                      << ", avg time = " << avgTime << " s\n";

            for (int tid = 0; tid < spiral.getNumOfThreads(); ++tid) {
                std::cout << "Thread " << tid
                          << ", time = " << threadExecTime[tid]
                          << " s\n";
            }

            std::cout << "\n";

            csv << name << "," << spiral.getNumOfThreads() << "," << spiral.getSize() << ","
                << block << "," << avgTime << "\n";
        }
    }

    return 0;
}

int main()
{
    int size = 1024;

    omp_set_nested(1);
    UlamSpiral spiral(size);

    std::ofstream csv("results.csv");
    csv << "Name,Threads,Size,BlockSize,AvgTime\n";

    runExperiment(
        "UlamBlocks",
        [&](int blockSize) { spiral.mapPrimes(blockSize); },
        spiral,
        5,
        csv);

    runExperiment(
        "UlamBlocksNest",
        [&](int blockSize) { spiral.mapPrimesNest(blockSize); },
        spiral,
        5,
        csv);
    csv.close();

    spiral.changeThreadsToUse(4);
    spiral.mapPrimes(size / 2);
    spiral.saveToPPM("../PPMs/2x2.ppm");
}

///////////////////////////////////////////

UlamSpiral::UlamSpiral(int size, int threads)
    : Size(size)
    , threadsToUse(threads)
{
    allocColorMatrix();
    allocMatrix();
    allocColorThreads();
    omp_set_num_threads(threadsToUse);

    applyMapping();
    applyColorThreads();
}

UlamSpiral::~UlamSpiral()
{
    deleteColorMatrix();
    deleteMatrix();
}

void UlamSpiral::changeThreadsToUse(int threads)
{
    deleteColorThreads();
    threadsToUse = threads;
    allocColorThreads();
    applyColorThreads();
    omp_set_num_threads(threads);
}

void UlamSpiral::changeSize(int newSize)
{
    deleteColorMatrix();
    deleteMatrix();

    Size = newSize;

    allocColorMatrix();
    allocMatrix();

    applyMapping();
}

void UlamSpiral::mapPrimes(int blockSize)
{
#pragma omp parallel
    {
        int tid = omp_get_thread_num();

        double start = omp_get_wtime();
#pragma omp for collapse(2) schedule(dynamic) nowait
        for (int bi = 0; bi < Size; bi += blockSize) {
            for (int bj = 0; bj < Size; bj += blockSize) {

                int iMax = std::min(bi + blockSize, Size);
                int jMax = std::min(bj + blockSize, Size);

                for (int x = bi; x < iMax; ++x) {
                    for (int y = bj; y < jMax; ++y) {
                        if (isPrime(Matrix[x][y])) {
                            ColorMatrix[x][y][0] = 0;
                            ColorMatrix[x][y][1] = 0;
                            ColorMatrix[x][y][2] = 0;
                        } else {
                            ColorMatrix[x][y][0] = ColorThreads[tid][0];
                            ColorMatrix[x][y][1] = ColorThreads[tid][1];
                            ColorMatrix[x][y][2] = ColorThreads[tid][2];
                        }
                    }
                }
            }
        }
        double end = omp_get_wtime();
        threadExecTime[tid] = end - start;
    }
}

void UlamSpiral::mapPrimesNest(int block)
{

#pragma omp parallel
    {
        double start = omp_get_wtime();
        int tid1 = omp_get_thread_num();

#pragma omp for collapse(2) nowait
        for (int bi = 0; bi < Size; bi += block) {
            for (int bj = 0; bj < Size; bj += block) {

                int iMax = std::min(bi + block, Size);
                int jMax = std::min(bj + block, Size);

#pragma omp parallel
                {
                    int tid2 = omp_get_thread_num();

#pragma omp for collapse(2) nowait
                    for (int x = bi; x < iMax; ++x) {
                        for (int y = bj; y < jMax; ++y) {

                            if (isPrime(Matrix[x][y])) {
                                ColorMatrix[x][y][0] = 0;
                                ColorMatrix[x][y][1] = 0;
                                ColorMatrix[x][y][2] = 0;
                            } else {
                                ColorMatrix[x][y][0] = ColorThreads[tid1][0];
                                ColorMatrix[x][y][1] = ColorThreads[tid1][1];
                                ColorMatrix[x][y][2] = ColorThreads[tid1][2];
                            }
                        }
                    }
                }
            }
        }
        double end = omp_get_wtime();
        threadExecTime[tid1] = end - start;
    }
}
int UlamSpiral::getMapping(int x, int y)
{
    x -= (Size - 1) / 2;
    y -= Size / 2;
    int mx = abs(x), my = abs(y);

    int l = 2 * std::max(mx, my);
    int d = y >= x ? l * 3 + x + y : l - x - y;

    return std::pow(l - 1, 2) + d;
}

void UlamSpiral::applyMapping()
{
    for (int x = 0; x < Size; ++x) {
        for (int y = 0; y < Size; ++y) {
            Matrix[x][y] = getMapping(x, y);
        }
    }
}

void UlamSpiral::applyColorThreads()
{
    for (int t = 0; t < threadsToUse; ++t) {
        ColorThreads[t][0] = 255 * ((float)t / threadsToUse);
        ColorThreads[t][1] = 0;
        ColorThreads[t][2] = 255 - ColorThreads[t][0];
    }
}

void UlamSpiral::allocMatrix()
{
    Matrix = new int*[Size];
    for (int x = 0; x < Size; ++x) {
        Matrix[x] = new int[Size];
    }
}

void UlamSpiral::allocColorMatrix()
{
    ColorMatrix = new unsigned char**[Size];
    for (int x = 0; x < Size; ++x) {
        ColorMatrix[x] = new unsigned char*[Size];
        for (int y = 0; y < Size; ++y) {
            ColorMatrix[x][y] = new unsigned char[3];
            ColorMatrix[x][y][0] = 0;
            ColorMatrix[x][y][1] = 0;
            ColorMatrix[x][y][2] = 0;
        }
    }
}

void UlamSpiral::allocColorThreads()
{
    ColorThreads = new unsigned char*[threadsToUse];
    for (int t = 0; t < threadsToUse; ++t) {
        ColorThreads[t] = new unsigned char[3];
    }
}

void UlamSpiral::deleteColorMatrix()
{
    for (int x = 0; x < Size; ++x) {
        for (int y = 0; y < Size; ++y) {
            delete[] ColorMatrix[x][y];
        }
        delete[] ColorMatrix[x];
    }
    delete[] ColorMatrix;
}

void UlamSpiral::deleteMatrix()
{
    for (int x = 0; x < Size; ++x) {
        delete[] Matrix[x];
    }
    delete[] Matrix;
}

void UlamSpiral::deleteColorThreads()
{
    for (int t = 0; t < threadsToUse; ++t) {
        delete[] ColorThreads[t];
    }
    delete[] ColorThreads;
}

void UlamSpiral::printMatrix()
{
    for (int x = 0; x < Size; ++x) {
        for (int y = 0; y < Size; ++y) {
            std::cout << std::setw(4) << Matrix[x][y] << ' ';
        }
        std::cout << '\n';
    }
}

void UlamSpiral::saveToPPM(std::string fileName, int scale)
{
    std::ofstream out(fileName, std::ios::binary);
    if (!out)
        return;

    out << "P6\n"
        << Size * scale << " " << Size * scale << "\n255\n";

    for (int x = 0; x < Size; ++x) {
        for (int i = 0; i < scale; ++i) {
            for (int y = 0; y < Size; ++y) {
                for (int j = 0; j < scale; ++j) {
                    out.write(reinterpret_cast<char*>(ColorMatrix[x][y]), 3);
                }
            }
        }
    }

    out.close();
}

bool isPrime(int n)
{
    int p;
    for (p = 2; p * p <= n; p++)
        if (n % p == 0)
            return 0;
    return n > 2;
}
