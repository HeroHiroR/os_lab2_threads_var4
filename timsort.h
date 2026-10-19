#ifndef TIMSORT_H
#define TIMSORT_H

#include <stddef.h>

/* Сортирует arr[left..right) — right exclusive */
void timsort_range(int *arr, size_t left, size_t right);

/* Последовательная сортировка всего массива */
void seq_sort(int *arr, size_t n);

/* Сливает два соседних отсортированных диапазона */
void merge_ranges(int *arr, size_t left, size_t mid, size_t right);

#endif