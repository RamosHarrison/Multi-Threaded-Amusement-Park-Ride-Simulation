#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdarg.h>


pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t ride_done = PTHREAD_COND_INITIALIZER;

int total_passengers = 1;
int passengers_boarded = 0;
int car_capacity = 1;
int ride_running = 0;

struct timeval start_time;

void log_time(const char* format, ...) {
    struct timeval now;
    gettimeofday(&now, NULL);
    long seconds = now.tv_sec - start_time.tv_sec;
    printf("[Time: %02ld:%02ld] ", seconds / 60, seconds % 60);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


void board(int id) {
    log_time("Passenger %d boarded the car (%d/%d)\n", id, passengers_boarded, car_capacity);
}

void unboard(int id) {
    log_time("Passenger %d unboarded the car\n", id);
}


void load() {
    log_time("Car invoked load()\n");
}

void run() {
    log_time("Car is running the ride...\n");
    sleep(3);  
}

void unload() {
    log_time("Car invoked unload()\n");
}


void* passenger_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    log_time("Passenger %d is exploring the park...\n", id);
    sleep(2);  
    log_time("Passenger %d finished exploring, heading to ticket booth\n", id);

    pthread_mutex_lock(&ticket_mutex);
    log_time("Passenger %d waiting in ticket queue\n", id);
    sleep(1);  
    log_time("Passenger %d acquired a ticket\n", id);
    pthread_mutex_unlock(&ticket_mutex);

    pthread_mutex_lock(&ride_mutex);
    log_time("Passenger %d joined the ride queue\n", id);

    
    while (passengers_boarded >= car_capacity || ride_running) {
        pthread_cond_wait(&car_ready, &ride_mutex);
    }

    passengers_boarded++;
    board(id);

    
    if (passengers_boarded == car_capacity) {
        pthread_cond_signal(&car_ready);
    }

    
    pthread_cond_wait(&ride_done, &ride_mutex);
    unboard(id);
    pthread_mutex_unlock(&ride_mutex);

    return NULL;
}


void* car_thread(void* arg) {
    pthread_mutex_lock(&ride_mutex);

    load();

    while (passengers_boarded < car_capacity) {
        pthread_cond_broadcast(&car_ready); 
        pthread_cond_wait(&car_ready, &ride_mutex); 
    }

    ride_running = 1;
    log_time("Car departing with %d passenger(s)\n", passengers_boarded);
    pthread_mutex_unlock(&ride_mutex);

    run();

    pthread_mutex_lock(&ride_mutex);
    ride_running = 0;
    unload();
    pthread_cond_broadcast(&ride_done);
    passengers_boarded = 0;
    pthread_mutex_unlock(&ride_mutex);

    return NULL;
}

int main() {
    gettimeofday(&start_time, NULL);
    pthread_t car;

    pthread_t passengers[total_passengers];
    pthread_create(&car, NULL, car_thread, NULL);

    for (int i = 0; i < total_passengers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&passengers[i], NULL, passenger_thread, id);
    }

    for (int i = 0; i < total_passengers; i++) {
        pthread_join(passengers[i], NULL);
    }

    pthread_join(car, NULL);

    log_time("Simulation complete. All %d passengers served.\n", total_passengers);

    return 0;
}

