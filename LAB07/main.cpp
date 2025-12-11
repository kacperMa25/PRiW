#include <chrono>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <math.h>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <vector>

#include "tbb/tbb.h"

const int iXmax = 20000;
const int iYmax = 20000;
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
const int MaxColorComponentValue = 255;
const int IterationMax = 500;
const double EscapeRadius = 2;
const int nr_threads = 16;

unsigned char color[iYmax][iXmax][3];
long int sum[nr_threads] = { 0 };
double threadExecTime[nr_threads] = { 0 };
std::mutex mtx;
int counter = 0;

void mandelbrotThread(int tid, unsigned char* threadColor);

template <typename Func>
double runExperiment(const std::string& name, Func func, int runs,
    std::ofstream& csv)
{
    double avgTime = 0;
    for (int r = 1; r <= runs; ++r) {
        for (int i = 0; i < nr_threads; i++)
            sum[i] = 0;
        counter = 0;

        auto start = std::chrono::steady_clock::now();
        std::vector<std::thread> threads;
        unsigned char threadColor[nr_threads][3];
        for (int i = 0; i < nr_threads; ++i) {
            threadColor[i][0] = (255 / nr_threads) * i;
            threadColor[i][1] = 255 - threadColor[i][0];
            threadColor[i][2] = 0;
            threads.emplace_back(func, i, threadColor[i]);
        }
        for (auto& t : threads)
            t.join();
        auto end = std::chrono::steady_clock::now();

        avgTime += std::chrono::duration<double>(end - start).count();
    }
    csv << name << "," << nr_threads << "," << avgTime / runs << "\n";
    std::cout << name << ": " << avgTime / runs << " s\n";
    for (int tid = 0; tid < nr_threads; ++tid) {
        std::cout << "Thread " << tid << ": " << threadExecTime[tid] << " s"
                  << std::endl;
    }
    std::cout << std::endl;

    /**
  FILE *fp;
  char *comment = "# ";
  fp = fopen((name + ".ppm").c_str(), "wb");
  fprintf(fp, "P6\n %s\n %d\n %d\n %d\n", comment, iXmax, iYmax,
          MaxColorComponentValue);
  for (int y = 0; y < iYmax; ++y) {
    for (int x = 0; x < iXmax; ++x) {
      fwrite(color[y][x], 1, 3, fp);
    }
  }
    **/
    return 0;
}

int main()
{
    std::ofstream csv("mandelbrot_times_pc.csv", std::ios::app);
    csv << "method,threads,run,time_seconds\n";

    mandelbrotThread(1, unsigned char* threadColor)
        csv.close();
    return 0;
}

void mandelbrotThread(int tid, unsigned char* threadColor)
{
    auto start = std::chrono::steady_clock::now();
    int lowerBound = (iYmax / nr_threads) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    tbb::parallel_for(tbb::blocked_range<int>(0, iYmax),
        [&](tbb::blocked_range<int> r) {
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
        });

    auto end = std::chrono::steady_clock::now();
}
