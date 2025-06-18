#ifndef PARK_H
#define PARK_H
#include <pthread.h>

#define MAX_PASSENGERS 100
#define MAX_CARS 10


typedef struct {
    int id;
    int capacity;
    int passengers_boarded;
    int passengers_unboarded;
    int unloading_now;
    int active;
    pthread_mutex_t lock;
    pthread_cond_t full;
    pthread_cond_t unload_done;
} Car;


extern Car cars[MAX_CARS];
extern int total_passengers;
extern int car_capacity;
extern int num_cars;
extern int wait_timeout;
extern int ride_duration;

extern int passengers_served;
extern int next_car_to_unload;

extern pthread_mutex_t ticket_mutex;
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t boarding_mutex;
extern pthread_cond_t can_board;

extern struct timeval start_time;

#endif 

