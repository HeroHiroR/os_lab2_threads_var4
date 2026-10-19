#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "timsort.h"


static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Генерация данных. type: 0=random, 1=half_sorted, 2=reversed */
static void gen_data(int *arr, size_t n, int type) {
    srand(42);
    for (size_t i = 0; i < n; i++) {
        arr[i] = rand() % 1000000;
    }
    if (type == 1) {
        timsort_range(arr, 0, n / 2);
    } else if (type == 2) {
        /* обратно отсортированный */
        for (size_t i = 0; i < n / 2; i++) {
            int tmp = arr[i];
            arr[i] = arr[n - 1 - i];
            arr[n - 1 - i] = tmp;
        }
    }
}

typedef struct {
    int *arr;
    size_t left;
    size_t right;
} ThreadArg;

static void *sort_worker(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    timsort_range(a->arr, a->left, a->right);
    return NULL;
}

static void parallel_timsort(int *arr, size_t n, int num_threads, double *elapsed) {
    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    ThreadArg *args    = malloc((size_t)num_threads * sizeof(ThreadArg));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 256 * 1024);

    size_t chunk = n / (size_t)num_threads;
    size_t rem   = n % (size_t)num_threads;

    double t0 = now_sec();

    size_t offset = 0;
    for (int i = 0; i < num_threads; i++) {
        size_t size = chunk + ((size_t)i < rem ? 1 : 0);
        args[i].arr   = arr;
        args[i].left  = offset;
        args[i].right = offset + size;
        offset += size;

        pthread_create(&threads[i], &attr, sort_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Последовательное слияние отсортированных диапазонов */
    size_t end = 0;
    for (int i = 0; i < num_threads; i++) {
        size_t size = chunk + ((size_t)i < rem ? 1 : 0);
        end += size;
        if (i > 0) {
            merge_ranges(arr, 0, end - size, end);
        }
    }

    double t1 = now_sec();
    *elapsed = t1 - t0;

    pthread_attr_destroy(&attr);
    free(threads);
    free(args);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <array_size> <data_type>\n"
            "  data_type: random | half_sorted | reversed\n", argv[0]);
        return 1;
    }

    size_t n = (size_t)atoll(argv[1]);
    const char *dtype = argv[2];

    int type = 0;
    if (strcmp(dtype, "random") == 0)      type = 0;
    else if (strcmp(dtype, "half_sorted") == 0) type = 1;
    else if (strcmp(dtype, "reversed") == 0)    type = 2;
    else {
        fprintf(stderr, "Unknown data_type: %s\n", dtype);
        return 1;
    }

    /* Для i7-13620H (nproc=16): 2,4,8,12 | 16 | 32,128,1024 */
    int thread_counts[] = {2, 4, 8, 12, 16, 32, 128, 1024};
    int n_counts = (int)(sizeof(thread_counts) / sizeof(thread_counts[0]));

    int *orig = malloc(n * sizeof(int));
    int *work = malloc(n * sizeof(int));
    if (!orig || !work) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    gen_data(orig, n, type);

    printf("=== TimSort: n=%zu, data=%s ===\n", n, dtype);
    printf("%-8s %-14s %-10s %-12s\n", "Threads", "Time(ms)", "Speedup", "Efficiency");

    /* Последовательная версия (эталон) */
    memcpy(work, orig, n * sizeof(int));
    double t0 = now_sec();
    seq_sort(work, n);
    double seq_time = now_sec() - t0;

    /* Проверка корректности последовательной версии */
    for (size_t i = 1; i < n; i++) {
        if (work[i - 1] > work[i]) {
            fprintf(stderr, "SEQ SORT INCORRECT at index %zu\n", i);
            return 1;
        }
    }

    printf("%-8d %-14.3f %-10.2f %-12.2f\n",
           1, seq_time * 1000.0, 1.0, 1.0);

    /* Параллельные версии */
    for (int k = 0; k < n_counts; k++) {
        int nt = thread_counts[k];
        memcpy(work, orig, n * sizeof(int));

        double par_time = 0.0;
        parallel_timsort(work, n, nt, &par_time);

        /* Проверка корректности */
        for (size_t i = 1; i < n; i++) {
            if (work[i - 1] > work[i]) {
                fprintf(stderr, "PAR SORT INCORRECT at index %zu (threads=%d)\n", i, nt);
                return 1;
            }
        }

        double speedup    = seq_time / par_time;
        double efficiency = speedup / (double)nt;

        printf("%-8d %-14.3f %-10.2f %-12.2f\n",
               nt, par_time * 1000.0, speedup, efficiency);
    }

    free(orig);
    free(work);
    return 0;
}