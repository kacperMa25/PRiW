#define CL_TARGET_OPENCL_VERSION 300

#include <CL/cl.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#define MAX_SIZE 19200

char* readKernelSource(const char* filename, size_t* length)
{
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Nie moĹźna otworzyÄ pliku: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    rewind(fp);

    char* source = (char*)malloc(*length + 1); // +1 na \0
    fread(source, 1, *length, fp);
    source[*length] = '\0'; // zakoĹcz jako string

    fclose(fp);
    return source;
}

int main()
{
    std::string fileName = "./openCLDouble.csv";
    bool newFile = !std::filesystem::exists(fileName);

    std::ofstream csv(fileName, std::ios::app);
    if (newFile) {
        csv << "size, time\n";
    }

    // Inicjalizacja OpenCL
    cl_int err;
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    cl_queue_properties properties[] = { 0 }; // Brak specjalnych wĹaĹciwoĹci
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, properties, &err);

    // ZaĹadowanie kodu kernelu
    size_t sourceSize;
    const char* kernelSource = readKernelSource("kernel.cl", &sourceSize);
    cl_program program = clCreateProgramWithSource(context, 1, &kernelSource, &sourceSize, &err);
    clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // ObsĹuga bĹÄdu
        size_t logSize;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> buildLog(logSize);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, buildLog.data(), nullptr);
        std::cerr << "Build Error:" << std::endl;
        std::cerr << buildLog.data() << std::endl;
    }

    cl_kernel kernel = clCreateKernel(program, "matrix_multiply_2d", &err);

    for (int i = 1200; i < MAX_SIZE; i += 1200) {
        auto start = std::chrono::high_resolution_clock::now();
        const unsigned int N = i;
        std::vector<double> A(N * N, 1.0f); // Macierz A
        std::vector<double> B(N * N, 1.0f); // Macierz B
        std::vector<double> C(N * N, 0.0f); // Macierz wynikowa C

        // Alokacja pamiÄci na dane
        cl_mem bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(double) * N * N, A.data(), &err);
        cl_mem bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(double) * N * N, B.data(), &err);
        cl_mem bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * N * N, nullptr, &err);

        // Przekazanie danych do kernela
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufferA);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufferB);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferC);
        clSetKernelArg(kernel, 3, sizeof(unsigned int), &N);

        // Ustawienie rozmiaru grupy roboczej i globalnego rozmiaru
        size_t globalSize[] = { N, N };
        size_t localSize[] = { 16, 16 }; // Wymiary grupy roboczej
        // Uruchomienie kernela
        err = clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr);
        clFinish(queue);
        // Pobranie wynikĂłw
        err = clEnqueueReadBuffer(queue, bufferC, CL_TRUE, 0, sizeof(double) * N * N, C.data(), 0, nullptr, nullptr);
        // Czyszczenie zasobĂłw
        clReleaseMemObject(bufferA);
        clReleaseMemObject(bufferB);
        clReleaseMemObject(bufferC);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time = end - start;
        std::cout << "Size: " << N << ", executed in " << time.count() << std::endl;
        csv << N << ", " << time.count() << std::endl;
    }

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return 0;
}
