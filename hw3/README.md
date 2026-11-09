# Urban Water Supply System Simulator
A console application for simulating an urban water supply system operating over an unreliable data transmission network. 
This version is implemented as a **multi-process application** using **POSIX shared memory** and **POSIX semaphores** for inter-process communication (IPC) and synchronization.  
# Build
## Requirements:  
- GCC compiler (C99 support)
- Linux OS
- POSIX threads and Realtime extensions support (`-pthread`, `-lrt`)  
- Make (optional)  
# Compilation
## Compilation via Makefile
```
make
```
## Manual Compilation
```
gcc -Wall -Wextra -std=c99 -o water_supply main.c simulation.c signal_handler.c logger.c config_reader.c strategies.c
```
# Run
```
./water_supply <config_file> [delay_ms] [max_time] [seed]
```
## Parameters
- `config_file` - Path to the configuration file (required).
- `delay_ms` - Delay between simulation ticks in milliseconds (optional, default is 500 ms).
- `max_time` - Maximum simulation time in ticks (optional, default is 100).
- `seed` - Seed for the random number generator to ensure reproducible network behavior (optional, default is 1).
## Run examples
```
# Basic run with a 500 ms delay
./water_supply config.txt

# Fast simulation (no delay) with custom max time and seed
./water_supply config.txt 0 200 42

# Slow demonstration (1 second per tick)
./water_supply config.txt 1000
```
## Program Termination
- By time: Automatically stops after reaching max_time.
- By emergency: Stops immediately if a reservoir completely dries up (volume <= 0) or overflows (volume >= max_volume), or if power limits are exceeded.
- By user interruption: Press Ctrl+C for graceful early termination. The signal handler triggers statistics output, waits for all child processes to exit, and safely cleans up IPC resources.
## IPC Cleanup
The program automatically cleans up shared memory (shm_unlink) and semaphores (sem_unlink / sem_destroy) upon normal exit or interruption.
In case of a fatal crash (e.g., SIGKILL), you can manually clean up resources:
```
# Remove shared memory
rm -f /dev/shm/wss_shm
# Remove named semaphores
rm -f /dev/shm/sem.wss_sem_*
```
# Example Configuration File (config.txt)
```
# ============================================================
# Urban Water Supply System Configuration
# ============================================================

# Reservoirs
# Format: reservoir <id> <min_volume> <max_volume> <initial_volume>
reservoirs 2
reservoir 0 20.0 100.0 60.0
reservoir 1 30.0 150.0 90.0

# Pumps
# Format: pump <id> <capacity> <power>
# capacity — flow rate (L/s), power — power consumption (W)
pumps 2
pump 0 10.0 50.0
pump 1 15.0 80.0

# Districts with consumption schedules
# Format: district <id> <schedule_size> <time1> <consumption1> <time2> <consumption2> ...
districts 2
district 0 3 0 8.0 10 12.0 20 6.0
district 1 2 0 12.0 15 8.0

# Pipelines
# Format: pipe <id> <source_type> <source_id> <dest_type> <dest_id> <capacity>
# source_type/dest_type: pump | reservoir | district
pipes 4
pipe 0 pump 0 reservoir 0 20.0
pipe 1 reservoir 0 district 0 15.0
pipe 2 pump 1 reservoir 1 25.0
pipe 3 reservoir 1 district 1 20.0

# Sensors
# Format: sensor <id> <reservoir_id> <measurement_frequency>
sensors 2
sensor 0 0 3
sensor 1 1 3

# Network Parameters
# Format: network <loss_probability> <max_delay> <duplicate_probability>
# Probabilities are in percentages (0-100)
network 15.0 2 10.0

# Dispatcher
# Format: dispatcher <max_power> <strategy_type>
# strategy_type: greedy | sequential
dispatcher 130 greedy
```
