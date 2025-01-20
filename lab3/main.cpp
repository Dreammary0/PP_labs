 /* Умножение матриц - основные моменты:
    Матричное умножение:
        * Скалярное умножение (mulMatrix) использует тройной вложенный цикл.
        * Векторизованное умножение (mulMatrix256) использует AVX-инструкции для ускорения вычислений.

    Генерация матриц:
        * getIdentityMatrix создает единичную матрицу.
        * getPermutationMatrix создает случайную перестановочную матрицу.

    Измерение времени:
        * Время выполнения измеряется с помощью std::chrono.

    Сравнение результатов:
        * Результаты скалярного и векторного умножения сравниваются с помощью memcmp.
*/

#include <assert.h>
#include <immintrin.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <cstring>
#include "fstream"
using namespace std;

const size_t matrixSize = 64 * (1 << 4); // Размер матрицы: 64 * 16 = 1024
const int num_tests = 10; // Количество тестов для усреднения

// Функция для классического матричного умножения (скалярное)
void mulMatrix(
        double* A, // Результирующая матрица
        size_t cA, // Количество столбцов в A
        size_t rA, // Количество строк в A
        const double* B, // Матрица B
        size_t cB, // Количество столбцов в B
        size_t rB, // Количество строк в B
        const double* C, // Матрица C
        size_t cC, // Количество столбцов в C
        size_t rC // Количество строк в C
        )
{
    assert(cB == rC && cA == cC && rA == rB);
    assert((cA & 0x3f) == 0); // Проверка, что размер кратен 64

    for (size_t i = 0; i < cA; i++)
    {
        for (size_t j = 0; j < rA; j++)
        {
            A[i * rA + j] = 0; // Инициализация элемента A[i][j]
            for (size_t k = 0; k < cB; k++)
            {
                A[i * rA + j] += B[k * rB + j] * C[i * rC + k]; // Умножение и сложение
            }
        }
    }
}

// Функция для матричного умножения с использованием AVX (векторизация)
void mulMatrix256(
        double* A, // Результирующая матрица
        const double* B, // Матрица B
        const double* C, // Матрица C
        size_t cA, // Количество столбцов в A
        size_t rA, // Количество строк в A
        size_t cB, // Количество столбцов в B
        size_t rB, // Количество строк в B
        size_t cC, // Количество столбцов в C
        size_t rC // Количество строк в C
        )
{
    assert(cB == rC && cA == cC && rA == rB);
    assert((cA & 0x3f) == 0); // Проверка, что размер кратен 64

    const size_t values_per_operation = 4; // Количество double, обрабатываемых за одну операцию AVX

    for (size_t i = 0; i < rB / values_per_operation; i++)
    {
        for (size_t j = 0; j < cC; j++)
        {
            __m256d sum = _mm256_setzero_pd(); // Инициализация суммы нулями
            for (size_t k = 0; k < rC; k++)
            {
                __m256d bCol = _mm256_loadu_pd(B + rB * k + i * values_per_operation); // Загрузка 4 элементов из B
                __m256d broadcasted = _mm256_set1_pd(C[j * rC + k]); // Броадкастинг элемента из C
                sum = _mm256_fmadd_pd(bCol, broadcasted, sum); // Умножение и сложение
            }

            _mm256_storeu_pd(A + j * rA + i * values_per_operation, sum); // Сохранение результата в A
        }
    }
}

// Функция для создания случайной перестановочной матрицы
vector<double> getPermutationMatrix(size_t n)
{
    vector<double> matrix(n * n); // Матрица n x n
    vector<size_t> permut(n); // Вектор для хранения перестановок

    // Генерация случайной перестановки
    for (size_t i = 0; i < n; i++)
    {
        permut[i] = i;
        swap(permut[i], permut[rand() % (i + 1)]); // Случайная перестановка
    }

    // Заполнение матрицы на основе перестановки
    for (size_t c = 0; c < n; c++)
    {
        matrix[c * n + permut[c]] = 1; // Единицы на позициях перестановки
    }

    return matrix;
}

// Функция для создания единичной матрицы
vector<double> getIdentityMatrix(size_t n)
{
    vector<double> matrix(n * n); // Матрица n x n

    // Заполнение единичной матрицы
    for (size_t c = 0; c < n; c++)
    {
        matrix[c * n + c] = 1; // Единицы на главной диагонали
    }

    return matrix;
}

// Основная функция
int main(int argc, char** argv)
{
    srand(time(NULL)); // Инициализация генератора случайных чисел

    std::ofstream output("../output.csv"); // Открытие файла для записи результатов

    if (!output.is_open())
    {
        std::cout << "Couldn't open file!\n";
        return -1;
    }

    // Лямбда-функция для вывода матрицы (закомментирована, так как матрицы большие)
    auto show_matrix = [](const double* A, size_t colsc, size_t rowsc)
    {
        return;

        for (size_t r = 0; r < rowsc; ++r)
        {
            cout << "[" << A[r + 0 * rowsc];
            for (size_t c = 1; c < colsc; ++c)
            {
                cout << ", " << A[r + c * rowsc];
            }
            cout << "]\n";
        }
        cout << "\n";
    };

    // Инициализация матриц
    vector<double> A(matrixSize * matrixSize), D(matrixSize * matrixSize);

    // Создание единичной и перестановочной матриц
    auto identity = getIdentityMatrix(matrixSize);
    auto permutation = getPermutationMatrix(matrixSize);

    vector<double> B = identity;
    vector<double> C = permutation;

    // Векторы для хранения времени выполнения
    vector<double> scalar_times(num_tests);
    vector<double> vector_times(num_tests);

    // Цикл для 10 запусков
    for (int test = 0; test < num_tests; ++test)
    {
        // Скалярное умножение матриц
        auto t1 = chrono::steady_clock::now();
        mulMatrix(A.data(), matrixSize, matrixSize,
                  B.data(), matrixSize, matrixSize,
                  C.data(), matrixSize, matrixSize);
        auto t2 = chrono::steady_clock::now();
        scalar_times[test] = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        // Векторизованное умножение матриц
        t1 = chrono::steady_clock::now();
        mulMatrix256(D.data(), B.data(), C.data(), matrixSize, matrixSize, matrixSize, matrixSize, matrixSize,
                     matrixSize);
        t2 = chrono::steady_clock::now();
        vector_times[test] = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        // Сравнение результатов скалярного и векторного умножения
        if (!std::memcmp(static_cast<void*>(A.data()),
                         static_cast<void*>(D.data()),
                         matrixSize * matrixSize * sizeof(double)))
        {
            cout << "Test " << test << " :" << "The results of matrix multiplication are the same! \n" << "Scalar_times: " <<
                    scalar_times[test] << "\n" << "Vector_times: " << vector_times[test] << "\n";
        }
    }

    // Вычисление среднего времени выполнения
    double avg_scalar_time = 0;
    double avg_vector_time = 0;

    for (int test = 0; test < num_tests; ++test)
    {
        avg_scalar_time += scalar_times[test];
        avg_vector_time += vector_times[test];
    }

    avg_scalar_time /= num_tests;
    avg_vector_time /= num_tests;

    // Запись результатов в файл
    output << "test,scalar,vector,avg_scalar,avg_vector\n";
    for (int test = 0; test < num_tests; ++test)
    {
        output << test << "," << scalar_times[test] << "," << vector_times[test] << "," << avg_scalar_time << "," << avg_vector_time << "\n";
    }

    // Вывод среднего времени выполнения
    cout << "Average Scalar Time: " << avg_scalar_time << " ms\n";
    cout << "Average Vector Time: " << avg_vector_time << " ms\n";

    output.close();
    return 0;
}