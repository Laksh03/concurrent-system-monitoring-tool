#ifndef STATS_FUNCTIONS_H
#define STATS_FUNCTIONS_H

void set_times(long int *user, long int *nice, long int *system, long int *idle, long int *iowait, long int *irq, long int *softirq);

void mem_info(int i, float *total_ram, float *used_ram, float *total_swap, float *used_swap);

void cpu_info(int i, long int *prev_t_total, long int *prev_t_usage, int *core_count, float *cpu_usage_percent);

void get_users(int user_pipe);

#endif