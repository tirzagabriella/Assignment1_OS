#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define BUFFER_SIZE 100
#define NUMBERS_TO_PRODUCE 10000

int buffer[BUFFER_SIZE]; // the buffer for production
int count = 0; // buffer count
int numbers_produced = 0; // counter to determine amount of produced numbers
int even_numbers = 0; // counter to know even numbers amount
int odd_numbers = 0; // counter to know odd numbers amount
int production_done = 0; // flag for production completion

// initialize mutex and the conditions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;

void* producer(void* arg);
void* consumer(void* arg);
void write_number_to_file(const char* filename, int number, long is_odd);

int main() {
    struct timeval start, end; 

    // get start time for overall duration count later
    gettimeofday(&start, NULL);
 
    pthread_t producer_thread, consumer_even_thread, consumer_odd_thread;

    srand(time(NULL));

    // Create threads
    if (pthread_create(&producer_thread, NULL, producer, NULL) ||
        pthread_create(&consumer_even_thread, NULL, consumer, (void*)0) || // 0 for even
        pthread_create(&consumer_odd_thread, NULL, consumer, (void*)1)) { // 1 for odd
        fprintf(stderr, "Error creating threads\n");
        return 1;
    }

    // Wait for all threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_even_thread, NULL);
    pthread_join(consumer_odd_thread, NULL);

    // get end time
    gettimeofday(&end, NULL);

    // calculate elapsed time
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    printf("Time taken by program is : %ld seconds and %ld microseconds\n", seconds, micros);
    printf("Odd numbers : %d\n", odd_numbers);
    printf("Even numbers : %d\n", even_numbers);

    return 0;
}

void* producer(void* arg) {
    FILE* all_file = fopen("all.txt", "w");
    if (all_file == NULL) {
        perror("Failed to open all.txt");
        return NULL;
    }

    for (int i = 0; i < NUMBERS_TO_PRODUCE; i++) {
        int item = rand() % 10000 + 1;

        pthread_mutex_lock(&mutex);

        // when buffer is full
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &mutex);
        }

        // produce numbers
        buffer[count++] = item;
        numbers_produced++;

        fprintf(all_file, "%d\n", item);

        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }

    fclose(all_file);

    pthread_mutex_lock(&mutex);
    production_done = 1;
    pthread_cond_broadcast(&can_consume); // wake up any sleeping consumers
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void* consumer(void* arg) {
    long is_odd = (long)arg; // 0 for even, 1 for odd
    const char* filename = is_odd ? "odd.txt" : "even.txt";

    while (1) {
        pthread_mutex_lock(&mutex);

        // when buffer is empty while the production isn't over
        while (count == 0 && !production_done) {
            pthread_cond_wait(&can_consume, &mutex);
        }

        // when all numbers produced and consumed
        if (count == 0 && production_done) {
            pthread_mutex_unlock(&mutex);
            break; 
        }

        // consume and write
        if ((buffer[count - 1] % 2) == is_odd) {
            int item = buffer[--count];
            pthread_mutex_unlock(&mutex);
            write_number_to_file(filename, item, is_odd);
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&can_produce);
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void write_number_to_file(const char* filename, int number, long is_odd) {
    FILE* file = fopen(filename, "a"); 
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }
    fprintf(file, "%d\n", number);
    is_odd ? odd_numbers++ : even_numbers++; // count amount of odd and even numbers consumed
    fclose(file);
}