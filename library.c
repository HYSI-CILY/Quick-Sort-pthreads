#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <math.h>

//#define MAX_ITEMS 4800
//#define MAX_THREADS 8
//#define THREADS_LEVEL 3
//#define SET_SIZE (MAX_ITEMS/MAX_THREADS)
#define swap(v, a, b) {unsigned tmp; tmp=v[a]; v[a]=v[b]; v[b]=tmp;}
int SET_SIZE, MAX_ITEMS, THREADS_LEVEL, MAX_THREADS;
static int *v;
pthread_t *tid;
//pthread_t tid[MAX_THREADS];
typedef struct {
    int end;
    int start;
} Args;
typedef struct {
    unsigned leftStart;
    unsigned rightStart;
    unsigned setSize;
    unsigned tlevel;
} MergeArgs;

static void print_array(void) {
    int i;
    for (i = 0; i < MAX_ITEMS; i++) {
        printf("%d ", v[i]);
    }
    printf("\n");
    printf("___________________________________________\n");
}

static void init_array(void) {
    int i;
    v = (int *) malloc(MAX_ITEMS * sizeof(int));
    for (i = 0; i < MAX_ITEMS; i++) {
        v[i] = rand();
    }
}

static unsigned partition(int *v, unsigned low, unsigned high, unsigned pivot_index) {
    if (pivot_index != low) swap(v, low, pivot_index);
    pivot_index = low;
    low++;
    while (low <= high) {
        if (v[low] <= v[pivot_index])
            low++;
        else if (v[high] > v[pivot_index])
            high--;

        else swap(v, low, high);

    }
    if (high != pivot_index) swap(v, pivot_index, high);
    return high;
}

/*
static int partition(int *lst, int lo, int hi) {
    int b = lo;
    int r = (int)(lo + (hi - lo) * (1.0 * rand() / RAND_MAX));
    double pivot = lst[r];
    swap(lst, r, hi);
    for (int i = lo; i < hi; i++) {
        if (lst[i] < pivot) {
            swap(lst, i, b);
            b++;
        }
    }
    swap(lst, hi, b);
    return b;
}
*/
static void quick_sort(int *v, unsigned low, unsigned high) {
    unsigned pivot_index;
    if (low >= high)
        return;
    pivot_index = (low + high) / 2;
    pivot_index = partition(v, low, high, pivot_index);
    if (low < pivot_index)
        quick_sort(v, low, pivot_index - 1);
    if (pivot_index < high)
        quick_sort(v, pivot_index + 1, high);
}

/*
void quick_sort(int *lst, int lo, int hi) {
    if (lo >= hi)
        return;
    int b = partition(lst, lo, hi);
    quick_sort(lst, lo, b - 1);
    quick_sort(lst, b + 1, hi);
}
*/
void *sortWorker(void *arg) {
    Args args = *((Args *) arg);
    quick_sort(v, args.start, args.end - 1);
    free(arg);
    pthread_exit(0);
}

void *merge(void *arg) {
    MergeArgs args = *((MergeArgs *) arg);
    int *left = v + args.leftStart;
    int *right = v + args.rightStart;

    int *temp = malloc((args.setSize * 2) * sizeof(int));

    int size = args.setSize;

    int i, j, k;

    i = j = k = 0;

    while (i < size && j < size) {
        if (left[i] <= right[j]) {
            temp[k++] = left[i++];
        } else {
            temp[k++] = right[j++];
        }
    }
    while (i < size)
        temp[k++] = left[i++];
    while (j < size)
        temp[k++] = right[j++];
    i = 0;
    int s = args.leftStart;
    for (; i < args.setSize * 2; i++) {
        v[s++] = temp[i];
    }
    //free(arg);
    free(temp);
    //print_array();
    pthread_exit(NULL);
}

void *mergeWorkerHelper(void *arg) {
    int rc;
    void *status;
    MergeArgs args = *((MergeArgs *) arg);

    int level = args.tlevel;
    int size = args.setSize;
    if (level <= 0 || size <= SET_SIZE) {
        //the level should never be 0
        //printf("level: %d\n",level);

        merge(&args);
        pthread_exit(NULL);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    MergeArgs args_fils_gauche;
    args_fils_gauche.leftStart = args.leftStart;
    args_fils_gauche.rightStart = args.leftStart + args.setSize / 2;
    args_fils_gauche.setSize = args.setSize / 2;
    args_fils_gauche.tlevel = args.tlevel - 1;

    MergeArgs args_fils_droite;
    args_fils_droite.leftStart = args.rightStart;
    args_fils_droite.rightStart = args.rightStart + ((args.setSize) / 2);
    args_fils_droite.setSize = args.setSize / 2;
    args_fils_droite.tlevel = args.tlevel - 1;

    pthread_t thread_gauche;
    pthread_t thread_droite;
    rc = pthread_create(&thread_gauche, &attr, mergeWorkerHelper, (void *) &args_fils_gauche);
    if (rc) {
        printf("ERROR: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    rc = pthread_create(&thread_droite, &attr, mergeWorkerHelper, (void *) &args_fils_droite);
    if (rc) {
        printf("ERROR: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    pthread_attr_destroy(&attr);
    rc = pthread_join(thread_gauche, &status);
    if (rc) {
        printf("ERROR: return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
    rc = pthread_join(thread_droite, &status);
    if (rc) {
        printf("ERROR: return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
    merge(&args);
    free(arg);
    pthread_exit(NULL);
}

void *mergeWorker() {
    int rc;
    void *status;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    MergeArgs *args = malloc(sizeof(*args));
    args->leftStart = 0;
    args->rightStart = MAX_ITEMS / 2;
    args->setSize = MAX_ITEMS / 2;
    args->tlevel = THREADS_LEVEL;

    pthread_t theThreadMaster;
    rc = pthread_create(&theThreadMaster, &attr, mergeWorkerHelper, (void *) args);
    if (rc) {
        printf("ERROR: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    pthread_attr_destroy(&attr);
    rc = pthread_join(theThreadMaster, &status);
    if (rc) {
        printf("ERROR: return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
}

int isSorted(int *lst, int size) {
    for (int i = 1; i < size; i++) {
        if (lst[i] < lst[i - 1]) {
            printf("at loc %d, %d < %d \n", i, lst[i], lst[i - 1]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    double start, end, times_merges, times;
    if (argc == 2) // user specified list size.
    {
        MAX_ITEMS = atoi(argv[1]);
    } else if (argc == 3) // user specified list size and thread level.
    {
        MAX_ITEMS = atoi(argv[1]);
        THREADS_LEVEL = atoi(argv[2]);
    } else {
        exit(EXIT_FAILURE);
    }
    times = times_merges = 0.0;
    for (int j = 0; j < 5; j++) {
        init_array();

        MAX_THREADS = pow(2, THREADS_LEVEL);
        SET_SIZE = (MAX_ITEMS / MAX_THREADS);
        tid = malloc(MAX_THREADS * sizeof(pthread_t));

        start = omp_get_wtime();

        int i = 0;
        for (; i < MAX_THREADS; i++) {
            Args *args = malloc(sizeof(*args));
            args->start = SET_SIZE * (i + 1) - SET_SIZE;
            args->end = SET_SIZE * (i + 1);
            pthread_create(&tid[i], 0, sortWorker, args);
        }
        i = 0;
        for (; i < MAX_THREADS; i++) {
            pthread_join(tid[i], NULL);
        }
        //merge the sous-lists
        double end1 = omp_get_wtime();
        //printf("time for sort: %lf\n",end1-start);
        mergeWorker();
        end = omp_get_wtime();
        //test is_Sorted()
        isSorted(v, MAX_ITEMS);

        times_merges += (end1 - start);
        times += (end - start);
    }
    //printf("time for dim: %d and threads: %d is %lf\n",MAX_ITEMS,MAX_THREADS,end-start);
    printf("%d %d %lf %lf\n", MAX_THREADS, MAX_ITEMS, times_merges / 5, times / 5);
    free(v);
}