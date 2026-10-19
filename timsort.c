#include "timsort.h"
#include <stdlib.h>
#include <string.h>

static void binary_insertion_sort(int *arr, size_t left, size_t right) {
    for (size_t i = left + 1; i < right; i++) {
        int key = arr[i];
        size_t lo = left, hi = i;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (arr[mid] <= key) lo = mid + 1;
            else                 hi = mid;
        }
        if (lo != i) {
            memmove(&arr[lo + 1], &arr[lo], (i - lo) * sizeof(int));
            arr[lo] = key;
        }
    }
}

#define MINRUN 32

static size_t min_run(size_t n) {
    size_t r = 0;
    while (n >= MINRUN) {
        r |= (n & 1);
        n >>= 1;
    }
    return n + r;
}

void merge_ranges(int *arr, size_t left, size_t mid, size_t right) {
    size_t n1 = mid - left;
    size_t n2 = right - mid;
    if (n1 == 0 || n2 == 0) return;

    int *tmp = malloc((n1 + n2) * sizeof(int));
    if (!tmp) return; /* при нехватке памяти тихо выходим (для лабы ок) */

    memcpy(tmp, &arr[left], n1 * sizeof(int));

    size_t i = 0;
    size_t j = mid;
    size_t k = left;

    while (i < n1 && j < right) {
        if (tmp[i] <= arr[j]) arr[k++] = tmp[i++];
        else                  arr[k++] = arr[j++];
    }
    while (i < n1)   arr[k++] = tmp[i++];

    free(tmp);
}

void timsort_range(int *arr, size_t left, size_t right) {
    size_t n = right - left;
    if (n < 2) return;

    size_t run = min_run(n);

    /* Шаг 1+2: сортируем каждый ран бинарными вставками */
    for (size_t start = left; start < right; start += run) {
        size_t end = start + run;
        if (end > right) end = right;
        binary_insertion_sort(arr, start, end);
    }

    /* Шаг 3: сливаем раны попарно, удваивая ширину на каждом проходе.
       Это классическое bottom-up merge, эквивалентное попарному
       слиянию ранов в TimSort (без стека). */
    size_t width = run;
    while (width < n) {
        for (size_t start = left; start < right; start += 2 * width) {
            size_t mid = start + width;
            if (mid >= right) break;
            size_t end = start + 2 * width;
            if (end > right) end = right;
            merge_ranges(arr, start, mid, end);
        }
        width *= 2;
    }
}

void seq_sort(int *arr, size_t n) {
    timsort_range(arr, 0, n);
}