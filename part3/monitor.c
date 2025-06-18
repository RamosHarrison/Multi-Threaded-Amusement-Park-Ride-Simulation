#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>
#include "monitor.h"


extern int total_rides;
extern double total_ticket_wait_time;
extern double total_ride_wait_time;
extern int total_ticket_wait_count;
extern int total_ride_wait_count;
extern int total_passengers_boarded;
extern int total_passengers;
extern int passengers_served;
extern int num_cars;
extern Car cars[MAX_CARS];
extern pthread_mutex_t queue_mutex;
extern struct timeval start_time;


void print_final_report();

void log_time_monitor(const char* format, ...) {
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsed = now.tv_sec - start_time.tv_sec;
	printf("[Monitor] [Time: %02ld:%02ld:%02ld] ", elapsed / 3600, (elapsed / 60) % 60, elapsed % 60);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void* monitor_thread(void* arg) {
	while (1) {
		pthread_mutex_lock(&queue_mutex);
		int done = (passengers_served >= total_passengers);
		pthread_mutex_unlock(&queue_mutex);

		if (done) break;

		log_time_monitor("========= SYSTEM SNAPSHOT =========\n");
		for (int i = 0; i < num_cars; i++) {
			pthread_mutex_lock(&cars[i].lock);
			printf("  [Car %d] Status: %s | Onboard: %d/%d | Unboarded: %d | Unloading: %s\n",
				cars[i].id,
				cars[i].active ? "LOADING" : "WAITING",
				cars[i].passengers_boarded,
				cars[i].capacity,
				cars[i].passengers_unboarded,
				cars[i].unloading_now ? "YES" : "NO");
			pthread_mutex_unlock(&cars[i].lock);
		}
		pthread_mutex_lock(&queue_mutex);
		printf("  Passengers served: %d/%d\n", passengers_served, total_passengers);
		pthread_mutex_unlock(&queue_mutex);
		printf("====================================\n\n");

		sleep(2);
	}

	log_time_monitor("All passengers served. Monitor exiting.\n");
	print_final_report();
	return NULL;
}

void print_final_report() {
	struct timeval now;
	gettimeofday(&now, NULL);
	long elapsed = now.tv_sec - start_time.tv_sec;

	int hours = elapsed / 3600;
	int minutes = (elapsed / 60) % 60;
	int seconds = elapsed % 60;

	double avg_ticket_wait = total_ticket_wait_count ? total_ticket_wait_time / total_ticket_wait_count : 0;
	double avg_ride_wait = total_ride_wait_count ? total_ride_wait_time / total_ride_wait_count : 0;
	double avg_utilization = total_rides ? ((double)total_passengers_boarded / (total_rides * cars[0].capacity)) : 0;

	printf("[Monitor] FINAL STATISTICS:\n");
	printf("Total simulation time: %02d:%02d:%02d\n", hours, minutes, seconds);
	printf("Total passengers served: %d\n", passengers_served);
	printf("Total rides completed: %d\n", total_rides);
	printf("Average wait time in ticket queue: %.1f seconds\n", avg_ticket_wait);
	printf("Average wait time in ride queue: %.1f seconds\n", avg_ride_wait);
	printf("Average car utilization: %.0f%% (%.1f/%d passengers per ride)\n",
		avg_utilization * 100,
		(double)total_passengers_boarded / total_rides,
		cars[0].capacity); 
}

