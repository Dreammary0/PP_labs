/* Запуск тестов производительности для функции vector_mod с разным количеством потоков.
		* Генерирует случайные данные для тестирования.
		* Запускает функцию vector_mod с разным количеством потоков и измеряет время выполнения.*/

#include "performance.h"
#include <memory>
#include <thread>
#include "num_threads.h"
#include "randomize.h"
#include "vector_mod.h"

std::vector<measurement> run_experiments()
{
	// Размер данных для тестирования
	constexpr std::size_t word_count = (std::size_t(1) << 30) / sizeof(IntegerWord);
	constexpr IntegerWord divisor = INTWORD_MAX;

	const size_t thread_count = std::thread::hardware_concurrency(); // Количество потоков
	auto data = std::make_unique<IntegerWord[]>(word_count); // Выделение памяти для данных
	std::vector<measurement> results;
	randomize(data.get(), word_count * sizeof(IntegerWord)); // Заполнение данных случайными числами
	results.reserve(thread_count);

	// Запуск тестов с разным количеством потоков
	for (unsigned T = 1; T <= thread_count; ++T)
	{
		set_num_threads(T); // Установка количества потоков
		using namespace std::chrono;
		auto tm0 = steady_clock::now();
		auto result = vector_mod(data.get(), word_count, divisor); // Выполнение vector_mod
		auto time = duration_cast<milliseconds>(steady_clock::now() - tm0); // Измерение времени
		results.emplace_back(measurement{result, time}); // Сохранение результата
	}
	return results;
}