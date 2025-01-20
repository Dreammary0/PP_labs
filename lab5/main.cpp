#include <complex>  // Для работы с комплексными числами
#include <bit>      // Для работы с битовыми операциями
#include <bitset>   // Для работы с битовыми наборами
#include <iostream> // Для вывода в консоль
#include <vector>   // Для использования std::vector
#include <thread>   // Для работы с потоками
#include <barrier>  // Для синхронизации потоков
#include <fstream>  // Для работы с файлами

// Переставляет элементы входного массива в соответствии с битовой перестановкой
void bit_shuffle(const std::complex<double>* in, std::complex<double>* out, std::size_t n) {
    std::size_t index_len = sizeof(n) * 8 - std::countl_zero(n) - 1;  // Вычисление длины индекса
    for (std::size_t i = 0; i < n; i++) {
        std::size_t index = i;
        std::size_t newIndex = 0;

        // Перестановка битов индекса
        for (int j = 0; j < index_len; j++) {
            newIndex <<= 1;
            newIndex += (index & 1);
            index >>= 1;
        }

        out[newIndex] = in[i];  // Запись элемента в новую позицию
    }
}

// Рекурсивная реализация быстрого преобразования Фурье (FFT)
void fft(const std::complex<double>* in, std::complex<double>* out, std::size_t n) {
    if (n == 1) {
        out[0] = in[0];  // Базовый случай рекурсии
        return;
    }

    // Рекурсивный вызов для первой и второй половины массива
    fft(in, out, n / 2);
    fft(in + n / 2, out + n / 2, n / 2);

    // Объединение результатов
    for (std::size_t i = 0; i < n / 2; i++) {
        auto w = std::polar(1.0, -2.0 * i * std::numbers::pi_v<double> / n);  // Вычисление поворотного коэффициента
        auto r1 = out[i];
        auto r2 = out[i + n / 2];
        out[i] = r1 + w * r2;          // Обновление значения
        out[i + n / 2] = r1 - w * r2;  // Обновление значения
    }
}

// Параллельная реализация быстрого преобразования Фурье (FFT)
void parallel_fft(const std::complex<double>* in, std::complex<double>* out, std::size_t N, std::size_t T) {
    std::vector<std::thread> threads(T - 1);  // Вектор для хранения потоков
    std::barrier<> bar(T);  // Барьер для синхронизации потоков

    // Лямбда-функция для работы каждого потока
    auto thread_lambda = [&in, &out, N, T, &bar](unsigned threadNumber) {
        // Инициализация данных для текущего потока
        for (size_t i = threadNumber; i < N; i += T) {
            out[i] = in[i];
        }

        // Основной цикл для выполнения FFT
        for (size_t n = 2; n <= N; n += n) {
            bar.arrive_and_wait();  // Синхронизация потоков
            for (size_t start = threadNumber * n; start + n <= N; start += T * n) {
                for (std::size_t i = 0; i < n / 2; i++) {
                    auto w = std::polar(1.0, -2.0 * i * std::numbers::pi_v<double> / n);  // Поворотный коэффициент
                    auto r1 = out[start + i];
                    auto r2 = out[start + i + n / 2];
                    out[start + i] = r1 + w * r2;          // Обновление значения
                    out[start + i + n / 2] = r1 - w * r2;  // Обновление значения
                }
            }
        }
    };

    // Запуск потоков
    for (std::size_t i = 1; i < T; ++i) {
        threads[i - 1] = std::thread(thread_lambda, i);
    }
    thread_lambda(0);  // Работа основного потока

    // Ожидание завершения всех потоков
    for (auto& i : threads) {
        i.join();
    }
}

// Реализация обратного быстрого преобразования Фурье (IFFT)
void ifft(const std::complex<double>* in, std::complex<double>* out, std::size_t n) {
    if (n == 1) {
        out[0] = in[0];  // Базовый случай рекурсии
        return;
    }

    // Рекурсивный вызов для первой и второй половины массива
    ifft(in, out, n / 2);
    ifft(in + n / 2, out + n / 2, n / 2);

    // Объединение результатов
    for (std::size_t i = 0; i < n / 2; i++) {
        auto w = std::polar(1.0, 2.0 * i * std::numbers::pi_v<double> / n);  // Вычисление поворотного коэффициента
        auto r1 = out[i];
        auto r2 = out[i + n / 2];
        out[i] = r1 + w * r2;          // Обновление значения
        out[i + n / 2] = r1 - w * r2;  // Обновление значения
    }
}

// Точка входа в программу
int main() {
    std::ofstream output("../output.csv");  // Открытие файла для записи результатов
    if (!output.is_open()) {
        std::cout << "Error. Could not open file!\n";
        return -1;
    }

    const std::size_t n = 1 << 20;  // Размер входного массива (2^20)
    std::vector<std::complex<double>> in(n), shuffled_in(n);  // Входные данные
    std::vector<std::complex<double>> out(n), shuffled_out(n), iout(n);  // Выходные данные

    // Инициализация входных данных
    for (int i = 0; i < n; i++) {
        in[i] = i;
    }

    // Битовая перестановка входных данных
    bit_shuffle(in.data(), shuffled_in.data(), n);

    size_t trials = 5;  // Количество тестов для усреднения времени
    size_t thread_count = std::thread::hardware_concurrency();  // Количество потоков
    size_t result[thread_count + 1];  // Массив для хранения результатов

    // Измерение времени выполнения рекурсивного FFT
    size_t recursive_time = 0;
    for (int i = 0; i < trials; i++) {
        auto tm0 = std::chrono::steady_clock::now();
        fft(shuffled_in.data(), out.data(), n);
        auto time = duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tm0);
        recursive_time += time.count();
    }
    result[0] = recursive_time / trials;  // Среднее время выполнения

    // Измерение времени выполнения параллельного FFT для разного количества потоков
    for (size_t i = 1; i <= thread_count; i++) {
        size_t parallel_time = 0;
        for (int j = 0; j < trials; j++) {
            auto tm0 = std::chrono::steady_clock::now();
            parallel_fft(shuffled_in.data(), out.data(), n, i);
            auto time = duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tm0);
            parallel_time += time.count();
        }
        result[i] = parallel_time / trials;  // Среднее время выполнения
    }

    // Вывод результатов в консоль и запись в файл
    std::cout << "T\t| Duration\t| Acceleration\n";
    output << "T,Duration\n";
    for (size_t i = 0; i <= thread_count; i++) {
        std::cout << i << "\t| " << result[i] << "\t| " << std::fixed << result[0] / (double)result[i] << std::endl;
        output << i << "," << result[i] << "\n";
    }

    return 0;
}