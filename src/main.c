#include <stdio.h>
#include <pthread.h>

void *thread_function(void *arg) {
    long value = (long) arg;
    printf("Thread value: %ld\n", value);
    return NULL;
}

int main() {
    pthread_t thread1;
    pthread_t thread2;

    long value1 = 1;
    long value2 = 5;

 
    pthread_create(&thread1, NULL, thread_function, (void *) value1);
    pthread_create(&thread2, NULL, thread_function, (void *) value2);

    pthread_join(thread1, NULL); // Wait for thread1 to finish
    pthread_join(thread2, NULL); // Wait for thread2 to finish
    return 0;
}
