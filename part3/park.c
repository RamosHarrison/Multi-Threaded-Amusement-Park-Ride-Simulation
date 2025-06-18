#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include "park.h"
#include "monitor.h"

Car cars[MAX_CARS];
int total_passengers = 10;
int car_capacity = 5;
int num_cars = 2;
int wait_timeout = 5;
int ride_duration = 3;
int passengers_served = 0;
int next_car_to_unload = -1;
int cars_finished = 0;


int total_rides = 0;
double total_ticket_wait_time = 0;
double total_ride_wait_time = 0;
int total_ticket_wait_count = 0;
int total_ride_wait_count = 0;
int total_passengers_boarded = 0;

pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t boarding_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_board = PTHREAD_COND_INITIALIZER;

struct timeval start_time;

void log_time(const char* format, ...) {
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsed = now.tv_sec - start_time.tv_sec;
	printf("[Time: %02ld:%02ld:%02ld] ", elapsed / 3600, (elapsed / 60) % 60, elapsed % 60);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void random_sleep(int min, int max) {
	sleep((rand() % (max - min + 1)) + min);
}

void board(int id, int car_id, int count) {
	log_time("[Passenger %2d] Boarded Car %d (%d/%d)\n", id, car_id, count, car_capacity);
}

void unboard(int id, int car_id) {
	log_time("[Passenger %2d] Unboarded from Car %d\n", id, car_id);
}

void* passenger_thread(void* arg) {
	int id = *(int*)arg;
	free(arg);

	log_time("[Passenger %2d] Exploring the park...\n", id);
	random_sleep(1, 3);

	
	struct timeval ticket_start, ticket_end;
	gettimeofday(&ticket_start, NULL);
	

	pthread_mutex_lock(&ticket_mutex);
	log_time("[Passenger %2d] Got a ticket\n", id);
	pthread_mutex_unlock(&ticket_mutex);

	
	gettimeofday(&ticket_end, NULL);
	double ticket_wait = (ticket_end.tv_sec - ticket_start.tv_sec) +
                     	(ticket_end.tv_usec - ticket_start.tv_usec) / 1e6;

	pthread_mutex_lock(&queue_mutex);
	total_ticket_wait_time += ticket_wait;
	total_ticket_wait_count++;
	pthread_mutex_unlock(&queue_mutex);
	

	Car* assigned_car = NULL;

	
	struct timeval ride_start, ride_end;
	gettimeofday(&ride_start, NULL);
	

	while (1) {
    	pthread_mutex_lock(&queue_mutex);
    	if (passengers_served >= total_passengers) {
        	pthread_mutex_unlock(&queue_mutex);
        	return NULL;
    	}
    	pthread_mutex_unlock(&queue_mutex);

    	pthread_mutex_lock(&boarding_mutex);
    	for (int i = 0; i < num_cars; i++) {
        	pthread_mutex_lock(&cars[i].lock);
        	if (cars[i].active && cars[i].passengers_boarded < cars[i].capacity) {
            	assigned_car = &cars[i];
            	assigned_car->passengers_boarded++;
            	board(id, assigned_car->id, assigned_car->passengers_boarded);

            	
            	gettimeofday(&ride_end, NULL);
            	double ride_wait = (ride_end.tv_sec - ride_start.tv_sec) +
                               	(ride_end.tv_usec - ride_start.tv_usec) / 1e6;
            	pthread_mutex_lock(&queue_mutex);
            	total_ride_wait_time += ride_wait;
            	total_ride_wait_count++;
            	pthread_mutex_unlock(&queue_mutex);
            	

            	if (assigned_car->passengers_boarded == assigned_car->capacity) {
                	pthread_cond_signal(&assigned_car->full);
            	}
            	pthread_mutex_unlock(&cars[i].lock);
            	pthread_mutex_unlock(&boarding_mutex);
            	goto wait_for_unload;
        	}
        	pthread_mutex_unlock(&cars[i].lock);
    	}
    	pthread_cond_wait(&can_board, &boarding_mutex);
    	pthread_mutex_unlock(&boarding_mutex);
	}

wait_for_unload:
	pthread_mutex_lock(&assigned_car->lock);
	while (!assigned_car->unloading_now)
    	pthread_cond_wait(&assigned_car->unload_done, &assigned_car->lock);

	unboard(id, assigned_car->id);
	assigned_car->passengers_unboarded++;
	if (assigned_car->passengers_unboarded == assigned_car->passengers_boarded)
    	pthread_cond_signal(&assigned_car->unload_done);
	pthread_mutex_unlock(&assigned_car->lock);

	pthread_mutex_lock(&queue_mutex);
	passengers_served++;
	log_time("[Passenger %2d] Finished ride. Served: %d/%d\n", id, passengers_served, total_passengers);
	pthread_mutex_unlock(&queue_mutex);

	return NULL;
}

void* car_thread(void* arg) {
	Car* car = (Car*)arg;
	int ride_num = 1;

	while (1) {
    	pthread_mutex_lock(&queue_mutex);
    	if (passengers_served >= total_passengers) {
        	pthread_mutex_unlock(&queue_mutex);
        	break;
    	}
    	if (car->passengers_boarded > 0 && car->passengers_unboarded < car->passengers_boarded) {
        	log_time("[Car %d] Waiting for previous passengers to unboard...\n", car->id);
        	while (car->passengers_unboarded < car->passengers_boarded)
            	pthread_cond_wait(&car->unload_done, &car->lock);
    	}
    	pthread_mutex_unlock(&queue_mutex);

    	pthread_mutex_lock(&boarding_mutex);
    	pthread_mutex_lock(&car->lock);
    	car->passengers_boarded = 0;
    	car->passengers_unboarded = 0;
    	car->unloading_now = 0;
    	car->active = 1;
    	log_time("[Car %d] Invoked load() for ride #%d\n", car->id, ride_num);
    	pthread_cond_broadcast(&can_board);
    	pthread_mutex_unlock(&boarding_mutex);

    	struct timespec timeout;
    	clock_gettime(CLOCK_REALTIME, &timeout);
    	timeout.tv_sec += wait_timeout;

    	while (car->passengers_boarded < car->capacity) {
        	int rc = pthread_cond_timedwait(&car->full, &car->lock, &timeout);
        	if (rc == ETIMEDOUT) {
            	if (car->passengers_boarded > 0) {
                	log_time("[Car %d] Timeout. Departing with %d/%d\n", car->id, car->passengers_boarded, car->capacity);
                	break;
            	} else {
                	car->active = 0;
                	pthread_mutex_unlock(&car->lock);
                	ride_num++;
                	goto continue_loop;
            	}
        	}
    	}

    	car->active = 0;
    	if (car->passengers_boarded == 0) {
        	pthread_mutex_unlock(&car->lock);
        	ride_num++;
        	continue;
    	}

    	
    	pthread_mutex_lock(&queue_mutex);
    	total_rides++;
    	total_passengers_boarded += car->passengers_boarded;
    	pthread_mutex_unlock(&queue_mutex);
    	

    	pthread_mutex_unlock(&car->lock);
    	log_time("[Car %d] Departing for ride #%d with %d passenger(s)\n", car->id, ride_num, car->passengers_boarded);
    	sleep(ride_duration);

    	pthread_mutex_lock(&car->lock);
    	if (next_car_to_unload == -1 && car->passengers_boarded > 0) {
        	next_car_to_unload = car->id;
        	pthread_cond_broadcast(&car->unload_done);
    	}

    	while (car->id != next_car_to_unload && car->passengers_boarded > 0)
        	pthread_cond_wait(&car->unload_done, &car->lock);

    	if (car->passengers_boarded > 0) {
        	car->unloading_now = 1;
        	log_time("[Car %d] Invoked unload() for ride #%d\n", car->id, ride_num);
        	pthread_cond_broadcast(&car->unload_done);

        	while (car->passengers_unboarded < car->passengers_boarded)
            	pthread_cond_wait(&car->unload_done, &car->lock);

        	int found = 0;
        	for (int i = 1; i <= num_cars; i++) {
            	int next = (car->id + i) % num_cars;
            	if (cars[next].passengers_boarded > 0 && cars[next].unloading_now == 0) {
                	next_car_to_unload = next;
                	found = 1;
                	break;
            	}
        	}
        	if (!found) next_car_to_unload = -1;
        	pthread_cond_broadcast(&car->unload_done);
    	}

    	pthread_mutex_unlock(&car->lock);
    	ride_num++;

	continue_loop:;
	}

	pthread_mutex_lock(&queue_mutex);
	cars_finished++;
	pthread_mutex_unlock(&queue_mutex);

	log_time("[Car %d] Exiting (all passengers served).\n", car->id);
	return NULL;
}

void parse_args(int argc, char* argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "n:c:p:w:r:h")) != -1) {
    	switch (opt) {
        	case 'n': total_passengers = atoi(optarg); break;
        	case 'c': num_cars = atoi(optarg); break;
        	case 'p': car_capacity = atoi(optarg); break;
        	case 'w': wait_timeout = atoi(optarg); break;
        	case 'r': ride_duration = atoi(optarg); break;
        	case 'h':
        	default:
            	printf("Usage: %s -n passengers -c cars -p capacity -w wait_time -r ride_duration\n", argv[0]);
            	exit(EXIT_SUCCESS);
    	}
	}
	if (num_cars > MAX_CARS || total_passengers > MAX_PASSENGERS) {
    	fprintf(stderr, "Too many cars or passengers. Adjust MAX constants.\n");
    	exit(EXIT_FAILURE);
	}
}

int main(int argc, char* argv[]) {
	gettimeofday(&start_time, NULL);
	srand(time(NULL));
	parse_args(argc, argv);

	printf("===== DUCK PARK SIMULATION =====\n");
	log_time("Simulation starting with:\n");
	printf("	- Passengers: %d\n", total_passengers);
	printf("	- Cars: %d\n", num_cars);
	printf("	- Capacity per Car: %d\n", car_capacity);
	printf("	- Wait Timeout: %d sec\n", wait_timeout);
	printf("	- Ride Duration: %d sec\n\n", ride_duration);

	pthread_t car_threads[MAX_CARS];
	pthread_t passenger_threads[MAX_PASSENGERS];
	pthread_t monitor;

	for (int i = 0; i < num_cars; i++) {
    	cars[i].id = i;
    	cars[i].capacity = car_capacity;
    	cars[i].passengers_boarded = 0;
    	cars[i].passengers_unboarded = 0;
    	cars[i].unloading_now = 0;
    	cars[i].active = 0;
    	pthread_mutex_init(&cars[i].lock, NULL);
    	pthread_cond_init(&cars[i].full, NULL);
    	pthread_cond_init(&cars[i].unload_done, NULL);
    	pthread_create(&car_threads[i], NULL, car_thread, &cars[i]);
	}

	for (int i = 0; i < total_passengers; i++) {
    	int* id = malloc(sizeof(int));
    	*id = i + 1;
    	pthread_create(&passenger_threads[i], NULL, passenger_thread, id);
	}

	pthread_create(&monitor, NULL, monitor_thread, NULL);

	for (int i = 0; i < total_passengers; i++)
    	pthread_join(passenger_threads[i], NULL);

	for (int i = 0; i < num_cars; i++)
    	pthread_join(car_threads[i], NULL);

	pthread_join(monitor, NULL);
	log_time("Simulation complete. All %d passengers served.\n", total_passengers);
	return 0;
}





