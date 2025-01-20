/* Реализация функции vector_mod, которая выполняет вычисления над вектором чисел с использованием многопоточности.
	* Функция vector_mod использует барьеры (std::barrier) для синхронизации потоков.
	* Выполняет операции сложения и умножения по модулю.
	* Использует оптимизированные функции add_mod и mul_mod из mod_ops.cpp. */

#include "vector_mod.h"
#include "mod_ops.h"
#include "num_threads.h"
#include <thread>
#include <vector>
#include <barrier>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

// Функция для возведения в степень по модулю
IntegerWord pow_mod(IntegerWord base, IntegerWord power, IntegerWord mod) {
	IntegerWord result = 1;
	while (power > 0) {
		if (power % 2 != 0) {
			result = mul_mod(result, base, mod);
		}
		power >>= 1;
		base = mul_mod(base, base, mod);
	}
	return result;
}

// Функция для вычисления (-mod)^power % mod
IntegerWord word_pow_mod(size_t power, IntegerWord mod) {
	return pow_mod((-mod) % mod, power, mod);
}

// Структура для хранения диапазона работы потока
struct thread_range {
	std::size_t b, e;
};

// Функция для вычисления диапазона работы потока
thread_range vector_thread_range(size_t n, unsigned T, unsigned t) {
	auto b = n % T;
	auto s = n / T;
	if (t < b) b = ++s * t;
	else b += s * t;
	size_t e = b + s;
	return thread_range{ b, e };
}

// Структура для хранения частичного результата
struct partial_result_t {
	alignas(hardware_destructive_interference_size) IntegerWord value;
};

// Основная функция для вычисления vector_mod
IntegerWord vector_mod(const IntegerWord* V, std::size_t N, IntegerWord mod) {
	size_t T = get_num_threads(); // Получение количества потоков
	std::vector<std::thread> threads(T - 1); // Создание потоков
	std::vector<partial_result_t> partial_results(T); // Частичные результаты
	std::barrier<> bar(T); // Барьер для синхронизации потоков

	// Лямбда-функция для работы потока
	auto thread_lambda = [V, N, T, mod, &partial_results, &bar](unsigned t) {
		auto [b, e] = vector_thread_range(N, T, t); // Диапазон работы потока

		IntegerWord sum = 0;
		// Вычисление частичной суммы (Схема Хорнера)
		for (std::size_t i = e; b < i;) {
			sum = add_mod(times_word(sum, mod), V[--i], mod); // то же самое, но без переполнения
		}
		partial_results[t].value = sum; // Сохранение результата

		// Синхронизация и объединение результатов
		for (size_t i = 1, ii = 2; i < T; i = ii, ii += ii) {
			bar.arrive_and_wait(); // Синхронизация
			if (t % ii == 0 && t + i < T) {
				auto neighbor = vector_thread_range(N, T, t + i);
				partial_results[t].value = add_mod(partial_results[t].value, mul_mod(partial_results[t + i].value, word_pow_mod(neighbor.b - b, mod), mod), mod);
			}
		}
	};

	// Запуск потоков
	for (std::size_t i = 1; i < T; ++i) {
		threads[i - 1] = std::thread(thread_lambda, i);
	}
	thread_lambda(0); // Работа основного потока

	// Ожидание завершения потоков
	for (auto& i : threads) {
		i.join();
	}
	return partial_results[0].value; // Возврат результата
}