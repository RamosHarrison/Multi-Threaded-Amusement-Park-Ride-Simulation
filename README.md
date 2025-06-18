# Multi-Threaded-Amusement-Park-Ride-Simulation
This is a multi-threaded simulation of an amusement park ride system, modeled using Pthreads. The simulation demonstrates how passengers enter the park, get tickets, line up for a roller coaster, ride it, and repeat. The ride car waits until it’s full before taking off, ensuring passengers get off in the correct order. This project focuses on thread synchronization, proper communication, and simulation of a real-world system using concurrent threads.

Features
Part 1: Basic version with a single passenger simulating the park flow (no concurrency).

Part 2: Full simulation with multiple passengers, a ticket booth, ride queue, and synchronization using semaphores and mutexes.

Part 3: Real-time monitoring system that tracks the state of the park, ride statuses, and queue sizes.

Synchronization: Uses mutexes and semaphores to ensure proper coordination between threads and prevent race conditions.

Edge Case Handling: Accounts for various edge cases like fewer passengers than the car's capacity, multiple cars, and possible deadlocks.

Installation
Prerequisites
Linux or Unix-like environment (macOS, WSL, etc.)

GCC compiler for compiling C code

Pthreads library (usually comes pre-installed on Linux systems)

Steps to Compile and Run
Clone the repository:
git clone https://github.com/RamosHarrison/Multi-Threaded-Amusement-Park-Ride-Simulation.git
cd duck-park

Compile:
make

Run:
./park -n <num_passengers> -c <car_capacity> -p <num_parks> -w <wait_time> -r <ride_time>

Where:

-n : Number of passengers in the park.

-c : Number of passengers needed to fill a car.

-p : Number of parks (rides).

-w : Time spent waiting for the ticket.

-r : Time taken for the ride.

Usage
Part 1: Basic Version
In this version, only one passenger is simulated. The passenger:

Enters the park.

Gets a ticket.

Rides the coaster.

Repeats the process.

No concurrency is used in this part; it's just to test the basic logic of the system.

Part 2: Full Simulation
In this version, multiple passengers enter the park:

They sleep for a random amount of time (1–10 seconds).

They get a ticket at the ticket booth (only one person can get a ticket at a time).

They wait in line for the ride, which only operates when it’s full or when the timeout occurs.

Semaphores are used to control the boarding and unboarding of the ride, while mutexes protect shared resources.

Part 3: Real-Time Monitoring
A separate monitoring thread is implemented to track the state of the system:

The monitor prints the status of the system every few seconds, including:

The number of people in each queue.

The status of each car (loading or running).

The total number of passengers in the park.

The monitor uses mutexes to protect the shared state and prevent race conditions.

Performance Results and Discussion
Compilation & Execution: All parts of the project compiled and ran successfully with no issues.

Memory Leaks: Valgrind was used to verify that there were no memory leaks in any part of the project.

Edge Cases: The system handled various edge cases:

Fewer passengers than car capacity—cars still departed after the wait timer.

Multiple passengers and multiple cars were handled without deadlocks.

All rides were completed with correct loading/unloading order.

Monitor Issues: During Part 3, the monitor occasionally "missed" the final car boarding or unloading due to its sleep cycles. However, this did not affect the correctness of the simulation since all passengers were served correctly.
