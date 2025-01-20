/*  Управление количеством потоков, используемых в программе.
        * Использует OpenMP для установки количества потоков.
        * Функции set_num_threads и get_num_threads позволяют управлять количеством потоков. */

#include "num_threads.h"
#include <omp.h> // Использование OpenMP для управления потоками

static unsigned thread_num = 1; // Текущее количество потоков

EXTERN_C void set_num_threads(unsigned T)
{
    if (!T || T > (unsigned) omp_get_num_procs())
        T = (unsigned) omp_get_num_procs(); // Установка количества потоков
    thread_num = T;
    omp_set_num_threads((int) T); // Установка количества потоков в OpenMP
}

EXTERN_C unsigned get_num_threads() {
    return thread_num; // Получение текущего количества потоков
}