#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <vector>

const int iXmax = 10240;
const int iYmax = 10240;
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
const int MaxColorComponentValue = 255;
const int IterationMax = 500;
const double EscapeRadius = 2;
const int nr_threads_const = 12;

unsigned char color[iYmax][iXmax][3];
long int sum[nr_threads_const] = { 0 };
double threadExecTime[nr_threads_const] = { 0 };
std::mutex mtx;
int counter = 0;

void mandelbrotThreadBlock(int tid, unsigned char* threadColor);
void mandelbrotThreadDynamic(int tid, unsigned char* threadColor);
void mandelbrotThreadMutex(int tid, unsigned char* threadColor);

template <typename Func>
double runExperiment(const std::string& name, Func func, int runs, std::ofstream& csv)
{
    double avgTime = 0;
    for (int r = 1; r <= runs; ++r) {
        for (int i = 0; i < nr_threads_const; i++)
            sum[i] = 0;
        counter = 0;

        auto start = std::chrono::steady_clock::now();
        std::vector<std::thread> threads;
        unsigned char threadColor[nr_threads_const][3];
        for (int i = 0; i < nr_threads_const; ++i) {
            threadColor[i][0] = (255 / nr_threads_const) * i;
            threadColor[i][1] = 255 - threadColor[i][0];
            threadColor[i][2] = 0;
            threads.emplace_back(func, i, threadColor[i]);
        }
        for (auto& t : threads)
            t.join();
        auto end = std::chrono::steady_clock::now();

        avgTime += std::chrono::duration<double>(end - start).count();
    }
    csv << name << "," << nr_threads_const << "," << avgTime / runs << "\n";
    std::cout << name << ": " << avgTime / runs << " s\n";
    for (int tid = 0; tid < nr_threads_const; ++tid) {
        std::cout << "Thread " << tid << ": " << threadExecTime[tid] << std::endl;
    }
    std::cout << std::endl;
    return 0;
}

int main()
{
    bool newFile = !std::filesystem::exists("mandelbrot_times.csv");
    std::ofstream csv("mandelbrot_times.csv", std::ios::app);
    if (newFile)
        csv << "method,threads,run,time_seconds\n";

    runExperiment("Block", mandelbrotThreadBlock, 3, csv);
    runExperiment("Dynamic", mandelbrotThreadDynamic, 3, csv);
    runExperiment("Mutex", mandelbrotThreadMutex, 3, csv);

    csv.close();
    return 0;
}

void mandelbrotThreadBlock(int tid, unsigned char* threadColor)
{
    auto start = std::chrono::steady_clock::now();
    int lowerBound = (iYmax / nr_threads_const) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads_const);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    for (iY = lowerBound; iY < upperBound; ++iY) {
        Cy = CyMin + iY * PixelHeight;
        if (fabs(Cy) < PixelHeight / 2)
            Cy = 0.0;

        for (iX = 0; iX < iXmax; iX++) {
            Cx = CxMin + iX * PixelWidth;
            Zx = Zy = 0.0;
            Zx2 = Zy2 = 0.0;

            for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                Iteration++) {
                Zy = 2 * Zx * Zy + Cy;
                Zx = Zx2 - Zy2 + Cx;
                Zx2 = Zx * Zx;
                Zy2 = Zy * Zy;
            }
            sum[tid] += Iteration;

            if (Iteration == IterationMax) {
                color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
            } else {
                color[iY][iX][0] = threadColor[0];
                color[iY][iX][1] = threadColor[1];
                color[iY][iX][2] = threadColor[2];
            }
        }
    }
    auto end = std::chrono::steady_clock::now();
    threadExecTime[tid] = (end - start).count();
}
void mandelbrotThreadDynamic(int tid, unsigned char* threadColor)
{
    auto start = std::chrono::steady_clock::now();
    int lowerBound = (iYmax / nr_threads_const) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads_const);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    for (iY = tid; iY < iYmax; iY += nr_threads_const) {
        Cy = CyMin + iY * PixelHeight;
        if (fabs(Cy) < PixelHeight / 2)
            Cy = 0.0;

        for (iX = 0; iX < iXmax; iX++) {
            Cx = CxMin + iX * PixelWidth;
            Zx = Zy = 0.0;
            Zx2 = Zy2 = 0.0;

            for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                Iteration++) {
                Zy = 2 * Zx * Zy + Cy;
                Zx = Zx2 - Zy2 + Cx;
                Zx2 = Zx * Zx;
                Zy2 = Zy * Zy;
            }
            sum[tid] += Iteration;

            if (Iteration == IterationMax) {
                color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
            } else {
                color[iY][iX][0] = threadColor[0];
                color[iY][iX][1] = threadColor[1];
                color[iY][iX][2] = threadColor[2];
            }
        }
    }
    auto end = std::chrono::steady_clock::now();
    threadExecTime[tid] = (end - start).count();
}

void mandelbrotThreadMutex(int tid, unsigned char* threadColor)
{
    auto start = std::chrono::steady_clock::now();
    int lowerBound = (iYmax / nr_threads_const) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads_const);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    int myID = 0;

    // for (iY = tid; iY < iYmax; iY += nr_threads) {
    while (myID < iYmax) {
        mtx.lock();
        myID = counter++;
        mtx.unlock();

        iY = myID;
        if (iY < iYmax) {
            Cy = CyMin + iY * PixelHeight;
            if (fabs(Cy) < PixelHeight / 2)
                Cy = 0.0;

            for (iX = 0; iX < iXmax; iX++) {
                Cx = CxMin + iX * PixelWidth;
                Zx = Zy = 0.0;
                Zx2 = Zy2 = 0.0;

                for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                    Iteration++) {
                    Zy = 2 * Zx * Zy + Cy;
                    Zx = Zx2 - Zy2 + Cx;
                    Zx2 = Zx * Zx;
                    Zy2 = Zy * Zy;
                }
                sum[tid] += Iteration;

                if (Iteration == IterationMax) {
                    color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
                } else {
                    color[iY][iX][0] = threadColor[0];
                    color[iY][iX][1] = threadColor[1];
                    color[iY][iX][2] = threadColor[2];
                }
            }
        }
    }
    auto end = std::chrono::steady_clock::now();
    threadExecTime[tid] = (end - start).count();
}
