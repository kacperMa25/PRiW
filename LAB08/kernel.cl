__kernel void matrix_multiply_2d(
        __global const float *A, // macierz A
        __global const float *B, // macierz B
        __global float *C, // macierz wynikowa C
        const unsigned int N) //
{
    // Indeks wątku w 2D
    unsigned int row = get_global_id(1); // Wiersz w macierzy C
    unsigned int col = get_global_id(0); // Kolumna w macierzy C
    // Pamięć lokalna
    __local float As[16][16]; // Fragment macierzy A w pamięci lokalnej
    __local float Bs[16][16]; // Fragment macierzy B w pamięci lokalnej
    float result = 0.0f;
    // Mnożenie macierzy C = A * B, podzielone na fragmenty
    for (unsigned int i = 0; i < (N / 16); ++i) {
        // Załaduj fragmenty A i B do pamięci lokalnej
        As[get_local_id(1)][get_local_id(0)] = A[row * N + (i * 16 + get_local_id(0))];
        Bs[get_local_id(1)][get_local_id(0)] = B[(i * 16 + get_local_id(1)) * N + col];
        // Czekaj na załadowanie danych
        barrier(CLK_LOCAL_MEM_FENCE);
        // Wykonaj mnożenie
        for (unsigned int j = 0; j < 16; ++j) {
            result += As[get_local_id(1)][j] * Bs[j][get_local_id(0)];
        }
        // Czekaj na zakończenie obliczeń przed załadowaniem nowych fragmentów
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    // Zapisz wynik do macierzy C
    C[row * N + col] = result;
}