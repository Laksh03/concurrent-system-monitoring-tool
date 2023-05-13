#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H

void print_mem_info(int samples, int i, float total_ram[], float used_ram[], float total_swap[], float used_swap[], int graphics_flag);

void print_system_information();

void print_users(int user_pipe);

void print_cpu_info(int i, int samples, int *core_count, float *cpu_usage_percent, int graphics_flag);

#endif