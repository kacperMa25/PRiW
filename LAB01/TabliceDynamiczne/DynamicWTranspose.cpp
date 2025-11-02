#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <thread>
#include <vector>

// This function will be called from a thread
int N = 8 * 128;
int nr_threads = 2; // Założenie: N dzieli się przez nr_threads

double **A, **B, **C, **BT;
double *_a, *_b, *_c, *_bt;

void func(int tid) {
  int i, j, k;

  int lb = (N / nr_threads) * tid;
  int ub = lb + (N / nr_threads) - 1;
  for (i = lb; i <= ub; ++i) {
    for (j = 0; j < N; ++j) {
      C[i][j] = 0;
      for (k = 0; k < N; ++k) {
        C[i][j] += A[i][k] * BT[j][k];
      }
    }
  }
}

void allocMatix(double **&m, double *&b) {
  m = new double *[N];
  b = new double[N * N];
  for (int i = 0; i < N; ++i) {
    m[i] = b + i * N;
  }
}

void deleteMatrix(double **&m, double *&b) {
  delete[] m;
  delete[] b;
}
int main(int argc, char **argv) {

  nr_threads = atoi(argv[1]);
  N = atoi(argv[2]);

  allocMatix(A, _a);

  allocMatix(B, _b);

  allocMatix(C, _c);

  allocMatix(BT, _bt);

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      BT[i][j] = B[j][i];
    }
  }
  const auto start{std::chrono::steady_clock::now()};
  std::vector<std::thread> th;

  // Launch a group of threads
  for (int i = 0; i < nr_threads; ++i) {
    th.push_back(std::thread(func, i));
  }

  // Join the threads with the main thread
  for (auto &t : th) {
    t.join();
  }
  const auto finish{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> elapsed_seconds{finish - start};
  std::cout << elapsed_seconds.count()
            << '\n'; // C++20's chrono::duration operator<<

  deleteMatrix(A, _a);
  deleteMatrix(B, _b);
  deleteMatrix(C, _c);

  std::ofstream myFile("TabliceDynamiczneT.csv", std::ios_base::app);

  myFile << nr_threads << ", " << N << ", " << elapsed_seconds.count()
         << std::endl;

  myFile.close();
  return 0;
}
