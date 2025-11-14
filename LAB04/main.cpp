#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <vector>

const int iXmax = 20000;
const int iYmax = 20000;
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
const int MaxColorComponentValue = 255;
const int IterationMax = 500;
const double EscapeRadius = 2;
const int nr_threads = 4;

unsigned char color[iYmax][iXmax][3];

long int sum[nr_threads] = { 0 };
double threadExecTime[nr_threads] = { 0 };
std::mutex mtx;
int counter = 0;

void mandelbrotThreadBlock(int tid, unsigned char* threadColor);
void mandelbrotThreadDynamic(int tid, unsigned char* threadColor);
void mandelbrotThreadMutex(int tid, unsigned char* threadColor);

template <typename Func>
double runExperiment(const std::string& name, Func func, int runs,
    std::ofstream& csv)
{
    double avgTime = 0;
    for (int r = 1; r <= runs; ++r) {
        for (int i = 0; i < nr_threads; i++)
            sum[i] = 0;
        counter = 0;
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
    bool newFile = !std::filesystem::exists("mandelbrot_times_pc.csv");
    std::ofstream csv("mandelbrot_times_pc.csv", std::ios::app);
    if (newFile)
        csv << "method,threads,run,time_seconds\n";

    omp_set_num_threads(nr_threads);

    auto start = omp_get_wtime();

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

#pragma omp parallel for schedule(static, 4)
    for (iY = 0; iY < iYmax; iY++) {
        int tid = omp_get_thread_num();
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
    auto end = omp_get_wtime();
    threadExecTime[tid] = (end - start);

    csv.close();
    return 0;
}

void mandelbrotThreadBlock(int tid, unsigned char* threadColor)
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
    int lowerBound = (iYmax / nr_threads) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    for (iY = tid; iY < iYmax; iY += nr_threads) {
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
    int lowerBound = (iYmax / nr_threads) * tid;
    int upperBound = lowerBound + (iYmax / nr_threads);

    int iX, iY, Iteration;
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

    int myID = 0;

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
