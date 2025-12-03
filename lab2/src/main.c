#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

sem_t thread_limit;

typedef struct {
    int* arr;
    int left;
    int right;
} thread_data;

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void merge(int* arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int* L = malloc(n1 * sizeof(int));
    int* R = malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void merge_sort(int* arr, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    merge_sort(arr, left, mid);
    merge_sort(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

void* thread_merge_sort(void* arg) {
    thread_data* data = (thread_data*)arg;
    merge_sort(data->arr, data->left, data->right);
    sem_post(&thread_limit);
    free(data);
    return NULL;
}

void parallel_merge_sort(int* arr, int left, int right) {
    if (left >= right) return;

    int mid = left + (right - left) / 2;

    pthread_t t1, t2;
    int created1 = 0, created2 = 0;

    if (sem_trywait(&thread_limit) == 0) {
        thread_data* d = malloc(sizeof(thread_data));
        d->arr = arr;
        d->left = left;
        d->right = mid;
        if (pthread_create(&t1, NULL, thread_merge_sort, d) == 0) {
            created1 = 1;
        } else {
            perror("pthread_create");
            free(d);
            sem_post(&thread_limit);
        }
    }

    if (sem_trywait(&thread_limit) == 0) {
        thread_data* d = malloc(sizeof(thread_data));
        d->arr = arr;
        d->left = mid + 1;
        d->right = right;
        if (pthread_create(&t2, NULL, thread_merge_sort, d) == 0) {
            created2 = 1;
        } else {
            perror("pthread_create");
            free(d);
            sem_post(&thread_limit);
        }
    }

    if (!created1) merge_sort(arr, left, mid);
    if (!created2) merge_sort(arr, mid + 1, right);

    // Ждём созданные потоки
    if (created1) pthread_join(t1, NULL);
    if (created2) pthread_join(t2, NULL);

    merge(arr, left, mid, right);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <max_threads>\n", argv[0]);
        return 1;
    }

    int max_threads = atoi(argv[1]);
    sem_init(&thread_limit, 0, max_threads);

    FILE* f = fopen("input.txt", "r");
    if (!f) {
        perror("Failed to open file");
        return 1;
    }

    int n;
    fscanf(f, "%d", &n);

    int* arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) fscanf(f, "%d", &arr[i]);
    fclose(f);

    printf("Array size: %d\n", n);
    printf("Max threads: %d\n", max_threads);

    double start_time = get_time_ms();

    if (max_threads > 1)
        parallel_merge_sort(arr, 0, n - 1);
    else
        merge_sort(arr, 0, n - 1);

    double end_time = get_time_ms();
    printf("Sorting time: %.2f ms\n", end_time - start_time);

    // for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    // printf("\n");

    free(arr);
    sem_destroy(&thread_limit);

    return 0;
}
