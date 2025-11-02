#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

int nr_threads = 1;
const int N = 2400;

double A[N][N], B[N][N], C[N][N];

void func(int tid) {
  int i, j, k;

  int lb = (N / nr_threads) * tid;
  int ub = lb + (N / nr_threads) - 1;
  for (i = lb; i <= ub; ++i) {
    for (j = 0; j < N; ++j) {
      C[i][j] = 0;
      for (k = 0; k < N; ++k) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

int main(int argv, char **argc) {

  nr_threads = std::atoi(argc[1]);
  const auto start{std::chrono::steady_clock::now()};

  std::vector<std::thread> threads;

  for (int i = 0; i < nr_threads; ++i) {
    threads.push_back(std::thread(func, i));
  }

  for (auto &t : threads) {
    t.join();
  }
  const auto finish{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> elapsed_seconds{finish - start};
  std::cout << elapsed_seconds.count() << '\n';

  std::ofstream myFile("TabliceStatyczne.csv", std::ios_base::app);

  myFile << nr_threads << ", " << N << ", " << elapsed_seconds.count()
         << std::endl;

  myFile.close();
  return 0;
}
