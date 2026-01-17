#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "tbb/tbb.h"

const int iXmax = 5000;
const int iYmax = 5000;
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
const int IterationMax = 500;
const double EscapeRadius = 2.0;
const int nr_threads = 8;

unsigned char color[iYmax][iXmax][3];

long long sum[nr_threads] = { 0 };
double threadExecTime[nr_threads] = { 0.0 };

thread_local int tid_local = -1;

int get_tid()
{
    if (tid_local == -1) {
        static std::atomic<int> next { 0 };
        tid_local = next++;
    }
    return tid_local;
}

void mandelbrotThread(int blockSize)
{
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;

    double ER2 = EscapeRadius * EscapeRadius;

    tbb::parallel_for(
        tbb::blocked_range<int>(0, iYmax, blockSize),
        [&](const tbb::blocked_range<int>& r) {
            auto start = tbb::tick_count::now();
            int tid = get_tid();

            long long localSum = 0;

            unsigned char threadColor[3];
            threadColor[0] = (255 / nr_threads) * tid;
            threadColor[1] = 255 - threadColor[0];
            threadColor[2] = 0;

            for (int iY = r.begin(); iY < r.end(); iY++) {

                double Cy = CyMin + iY * PixelHeight;
                if (std::abs(Cy) < PixelHeight / 2)
                    Cy = 0.0;

                for (int iX = 0; iX < iXmax; iX++) {

                    double Cx = CxMin + iX * PixelWidth;
                    double Zx = 0.0, Zy = 0.0, Zx2 = 0.0, Zy2 = 0.0;

                    int Iteration;
                    for (Iteration = 0;
                        Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                        Iteration++) {
                        Zy = 2 * Zx * Zy + Cy;
                        Zx = Zx2 - Zy2 + Cx;
                        Zx2 = Zx * Zx;
                        Zy2 = Zy * Zy;
                    }

                    localSum += Iteration;

                    if (Iteration == IterationMax) {
                        color[iY][iX][0] = 0;
                        color[iY][iX][1] = 0;
                        color[iY][iX][2] = 0;
                    } else {
                        color[iY][iX][0] = threadColor[0];
                        color[iY][iX][1] = threadColor[1];
                        color[iY][iX][2] = threadColor[2];
                    }
                }
            }

            sum[tid] += localSum;

            auto end = tbb::tick_count::now();
            threadExecTime[tid] += (end - start).seconds();
        });
}

template <typename Func>
double runExperiment(
    const std::string& name,
    Func func,
    int runs,
    std::ofstream& csv)
{
    int maxBlockSize = 256;
    int blockJump = 2;

    for (int blockSize = 1; blockSize <= maxBlockSize; blockSize *= blockJump) {

        for (int i = 0; i < nr_threads; i++) {
            sum[i] = 0;
            threadExecTime[i] = 0.0;
        }

        double avgTime = 0;

        for (int r = 0; r < runs; r++) {

            auto start = tbb::tick_count::now();
            func(blockSize);
            auto end = tbb::tick_count::now();

            avgTime += (end - start).seconds();
        }

        avgTime /= runs;

        std::cout << "Block size: " << blockSize << std::endl;
        std::cout << name << ": " << avgTime << " s\n";

        for (int tid = 0; tid < nr_threads; ++tid) {
            std::cout << "Thread " << tid
                      << " iterations executed: " << sum[tid]
                      << ", execution time: " << threadExecTime[tid]
                      << std::endl;
        }

        std::cout << std::endl;

        csv << name << "," << nr_threads << "," << iXmax
            << "," << blockSize << "," << avgTime << "\n";
    }

    return 0;
}

int main()
{
    std::string fileName = "./mandelbrot_times_tbb.csv";
    bool newFile = !std::filesystem::exists(fileName);

    std::ofstream csv(fileName, std::ios::app);
    if (newFile) {
        csv << "method,threads,size,blockSize,time_seconds\n";
    }

    tbb::global_control ctrl(tbb::global_control::max_allowed_parallelism,
        nr_threads);

    runExperiment("TBB", mandelbrotThread, 1, csv);

    return 0;
}
