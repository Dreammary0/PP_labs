 /* Сложение матриц - основные моменты:
Размеры матриц:
    Матрицы имеют размеры 32768 x 32768 (2^15 x 2^15).

    Скалярное сложение (addMatrix):
        Проходит по всем элементам матриц и складывает их поэлементно.
        Используется для сравнения производительности с векторным сложением.

    Векторное сложение (addMatrix256):
        Использует AVX-инструкции для сложения 4 элементов за раз.
            __m256d — это тип данных для работы с 4 числами double.
            _mm256_loadu_pd — загрузка 4 double из памяти.
            _mm256_add_pd — сложение 4 double.
            _mm256_storeu_pd — сохранение результата в память.

    Время выполнения измеряется с помощью std::chrono::steady_clock.

    Инициализация матриц:
        Матрица B заполнена единицами, матрица C — минус двойками.
        Матрица A используется для хранения результата.

    Очистка матриц:
        Перед векторным сложением матрицы A, B и C очищаются и заполняются новыми значениями. */


#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <immintrin.h>
#include <fstream>
#include <cstring> // Для memset (очистки кэша)

const int cols = 1 << 15; // Количество столбцов (32768)
const int rows = 1 << 15; // Количество строк (32768)
const size_t batch = 4;   // Размер пакета для AVX (4 double)
const int num_tests = 10; // Количество тестов

// Функция для очистки кэша
void clear_cache() {
    const size_t size = 100 * 1024 * 1024; // 100 МБ (достаточно для очистки кэша)
    char* memory = new char[size];
    std::memset(memory, 0, size); // Заполняем память нулями
    delete[] memory; // Освобождаем память
}

// Скалярное сложение матриц
void addMatrix(double* A, const double* B, const double* C, size_t colsc, size_t rowsc)
{
    for (size_t i = 0; i < colsc * rowsc; i++)
    {
        A[i] = B[i] + C[i];
    }
}

// Векторное сложение матриц с использованием AVX
void addMatrix256(double* A, const double* B, const double* C, size_t colsc, size_t rowsc)
{
    for (size_t i = 0; i < rowsc * colsc / batch; i++)
    {
        __m256d b = _mm256_loadu_pd(&(B[i * batch]));
        __m256d c = _mm256_loadu_pd(&(C[i * batch]));
        __m256d a = _mm256_add_pd(b, c);
        _mm256_storeu_pd(&(A[i * batch]), a);
    }
}

int main()
{
    std::ofstream output("../output.csv");

    if (!output.is_open())
    {
        std::cout << "Couldn't open file!\n";
        return -1;
    }

    // Заголовок для CSV-файла
    output << "test,scalar,vector,avg_scalar,avg_vector\n";

    std::vector<double> B(cols * rows, 1), C(cols * rows, -2), A(cols * rows);

    double total_scalar_time = 0;
    double total_vector_time = 0;

    // Векторы для хранения времени каждого теста
    std::vector<double> scalar_times(num_tests);
    std::vector<double> vector_times(num_tests);

    for (int test = 0; test < num_tests; ++test)
    {
        // Очистка кэша перед каждым тестом
        clear_cache();

        // Перезаполнение матриц
        std::fill(B.begin(), B.end(), 1);
        std::fill(C.begin(), C.end(), -2);
        std::fill(A.begin(), A.end(), 0);

        // Скалярное сложение
        auto t1 = std::chrono::steady_clock::now();
        addMatrix(A.data(), B.data(), C.data(), cols, rows);
        auto t2 = std::chrono::steady_clock::now();
        double scalar_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        scalar_times[test] = scalar_time;
        total_scalar_time += scalar_time;

        // Очистка кэша перед векторным сложением
        clear_cache();

        // Перезаполнение матриц
        std::fill(B.begin(), B.end(), -2);
        std::fill(C.begin(), C.end(), 1);
        std::fill(A.begin(), A.end(), 0);

        // Векторное сложение
        t1 = std::chrono::steady_clock::now();
        addMatrix256(A.data(), B.data(), C.data(), cols, rows);
        t2 = std::chrono::steady_clock::now();
        double vector_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        vector_times[test] = vector_time;
        total_vector_time += vector_time;
    }

    // Вычисление средних значений
    double avg_scalar_time = total_scalar_time / num_tests;
    double avg_vector_time = total_vector_time / num_tests;

    // Запись результатов каждого теста и средних значений
    for (int test = 0; test < num_tests; ++test)
    {
        output << test << ","
               << scalar_times[test] << ","
               << vector_times[test] << ","
               << avg_scalar_time << ","
               << avg_vector_time << "\n";
    }

    // Вывод среднего времени выполнения
    std::cout << "Average Scalar Time: " << avg_scalar_time << " ms.\n";
    std::cout << "Average Vector Time: " << avg_vector_time << " ms.\n";

    output.close();
    return 0;
}