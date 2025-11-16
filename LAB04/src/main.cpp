#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <math.h>
#include <omp.h>
#include <stdio.h>

const int iXmax = 10000;
const int iYmax = 10000;
const double CxMin = -2.5;
const double CxMax = 1.5;
const double CyMin = -2.0;
const double CyMax = 2.0;
const int MaxColorComponentValue = 255;
const int IterationMax = 500;
const double EscapeRadius = 2;
const int nr_threads = 8;

unsigned char color[iYmax][iXmax][3];

long int sum[nr_threads] = { 0 };
double threadExecTime[nr_threads] = { 0 };
int counter = 0;

void mandelbrotThreadGuided();
void mandelbrotThreadStatic();
void mandelbrotThreadDynamic();

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

    for (int i = 0; i < runs; ++i) {
        auto start = omp_get_wtime();
        func();
        auto end = omp_get_wtime();

        avgTime += end - start;
    }

    avgTime /= runs;

    // csv << name << "," << nr_threads << "," << iXmax << "," << avgTime << "\n";
    std::cout << name << ": " << avgTime << " s\n";
    for (int tid = 0; tid < nr_threads; ++tid) {
        std::cout << "Thread " << tid << ": " << threadExecTime[tid] << " s" << ", iterations executed: " << sum[tid]
                  << std::endl;
    }
    std::cout << std::endl;

    /**
    FILE* fp;
    char* comment = "# ";
    fp = fopen((name + std::to_string(nr_threads) + ".ppm").c_str(), "wb");
    fprintf(fp, "P6\n %s\n %d\n %d\n %d\n", comment, iXmax, iYmax,
        MaxColorComponentValue);

    for (int y = 0; y < iYmax; ++y) {
        for (int x = 0; x < iXmax; ++x) {
            fwrite(color[y][x], 1, 3, fp);
        }
    }
    fclose(fp);
    **/

    return 0;
}

int main()
{
    bool newFile = !std::filesystem::exists("../mandelbrot_times_pc.csv");
    std::ofstream csv("../mandelbrot_times_pc.csv", std::ios::app);
    if (newFile)
        csv << "method,threads,size,time_seconds\n";

    omp_set_num_threads(nr_threads);

    runExperiment("Guided", mandelbrotThreadGuided, 1, csv);
    runExperiment("Static", mandelbrotThreadStatic, 1, csv);
    runExperiment("Dynamic", mandelbrotThreadGuided, 1, csv);

    csv.close();
    return 0;
}

void mandelbrotThreadGuided()
{
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

#pragma omp parallel private(Cx, Cy, Zx, Zy, Zx2, Zy2)
    {
        int tid = omp_get_thread_num();
        auto start = omp_get_wtime();

        long int localSum = 0;

        unsigned char threadColor[3];
        threadColor[0] = (255 / nr_threads) * tid;
        threadColor[1] = 255 - threadColor[0];
        threadColor[2] = 0;

#pragma omp for schedule(guided)
        for (int iY = 0; iY < iYmax; ++iY) {

            Cy = CyMin + iY * PixelHeight;
            if (fabs(Cy) < PixelHeight / 2)
                Cy = 0.0;

            for (int iX = 0; iX < iXmax; iX++) {

                Cx = CxMin + iX * PixelWidth;

                Zx = Zy = 0.0;
                Zx2 = Zy2 = 0.0;

                int Iteration;
                for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                    Iteration++) {
                    Zy = 2 * Zx * Zy + Cy;
                    Zx = Zx2 - Zy2 + Cx;
                    Zx2 = Zx * Zx;
                    Zy2 = Zy * Zy;
                }

                localSum += Iteration;

                if (Iteration == IterationMax) {
                    color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
                } else {
                    color[iY][iX][0] = threadColor[0];
                    color[iY][iX][1] = threadColor[1];
                    color[iY][iX][2] = threadColor[2];
                }
            }
        }

        sum[tid] = localSum;

        auto end = omp_get_wtime();
        threadExecTime[tid] = end - start;
    }
}

void mandelbrotThreadStatic()
{
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

#pragma omp parallel private(Cx, Cy, Zx, Zy, Zx2, Zy2)
    {
        int tid = omp_get_thread_num();
        auto start = omp_get_wtime();

        long int localSum = 0;

        unsigned char threadColor[3];
        threadColor[0] = (255 / nr_threads) * tid;
        threadColor[1] = 255 - threadColor[0];
        threadColor[2] = 0;

#pragma omp for schedule(static)
        for (int iY = 0; iY < iYmax; ++iY) {

            Cy = CyMin + iY * PixelHeight;
            if (fabs(Cy) < PixelHeight / 2)
                Cy = 0.0;

            for (int iX = 0; iX < iXmax; iX++) {

                Cx = CxMin + iX * PixelWidth;

                Zx = Zy = 0.0;
                Zx2 = Zy2 = 0.0;

                int Iteration;
                for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                    Iteration++) {
                    Zy = 2 * Zx * Zy + Cy;
                    Zx = Zx2 - Zy2 + Cx;
                    Zx2 = Zx * Zx;
                    Zy2 = Zy * Zy;
                }

                localSum += Iteration;

                if (Iteration == IterationMax) {
                    color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
                } else {
                    color[iY][iX][0] = threadColor[0];
                    color[iY][iX][1] = threadColor[1];
                    color[iY][iX][2] = threadColor[2];
                }
            }
        }

        sum[tid] = localSum;

        auto end = omp_get_wtime();
        threadExecTime[tid] = end - start;
    }
}

void mandelbrotThreadDynamic()
{
    double Cx, Cy;
    double PixelWidth = (CxMax - CxMin) / iXmax;
    double PixelHeight = (CyMax - CyMin) / iYmax;
    double Zx, Zy, Zx2, Zy2;
    double ER2 = EscapeRadius * EscapeRadius;

#pragma omp parallel private(Cx, Cy, Zx, Zy, Zx2, Zy2)
    {
        int tid = omp_get_thread_num();
        auto start = omp_get_wtime();

        long int localSum = 0;

        unsigned char threadColor[3];
        threadColor[0] = (255 / nr_threads) * tid;
        threadColor[1] = 255 - threadColor[0];
        threadColor[2] = 0;

#pragma omp for schedule(dynamic)
        for (int iY = 0; iY < iYmax; ++iY) {

            Cy = CyMin + iY * PixelHeight;
            if (fabs(Cy) < PixelHeight / 2)
                Cy = 0.0;

            for (int iX = 0; iX < iXmax; iX++) {

                Cx = CxMin + iX * PixelWidth;

                Zx = Zy = 0.0;
                Zx2 = Zy2 = 0.0;

                int Iteration;
                for (Iteration = 0; Iteration < IterationMax && (Zx2 + Zy2) < ER2;
                    Iteration++) {
                    Zy = 2 * Zx * Zy + Cy;
                    Zx = Zx2 - Zy2 + Cx;
                    Zx2 = Zx * Zx;
                    Zy2 = Zy * Zy;
                }

                localSum += Iteration;

                if (Iteration == IterationMax) {
                    color[iY][iX][0] = color[iY][iX][1] = color[iY][iX][2] = 0;
                } else {
                    color[iY][iX][0] = threadColor[0];
                    color[iY][iX][1] = threadColor[1];
                    color[iY][iX][2] = threadColor[2];
                }
            }
        }

        sum[tid] = localSum;

        auto end = omp_get_wtime();
        threadExecTime[tid] = end - start;
    }
}
