#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>
#include <signal.h>

#include "stats_functions.h"
#include "A3.h"
#define BUF_SIZE 1024

//set_times takes in pointers for each category of cpu time usage shown in /proc/stat. Then reads the file and assigns the corresponding values to pointers. 
void set_times(long int *user, long int *nice, long int *system, long int *idle, long int *iowait, long int *irq, long int *softirq){
	FILE *file = fopen("/proc/stat", "r");
	char *line = malloc(sizeof(char) * 400);
	char *next;
	fgets(line, 400, file);
	
	*user = strtol(line+4, &next, 10);
	*nice = strtol(next, &next, 10);
	*system = strtol(next, &next, 10);
	*idle = strtol(next, &next, 10);
	*iowait = strtol(next, &next, 10);
	*irq = strtol(next, &next, 10);
	*softirq = strtol(next, &next, 10);

	free(line);
	fclose(file);
}


void mem_info(int i, float *total_ram, float *used_ram, float *total_swap, float *used_swap){
    //struct sysinfo will store all memory data within its fields
	struct sysinfo *meminfo = malloc(sizeof(struct sysinfo));

    // initializing memory data into the fields of meminfo and error checking. 
    if(sysinfo(meminfo) == -1){
        perror("sysinfo");
    }
    
    //Storing the memory samples into arrays for printing
    total_ram[i] = (float)(meminfo->totalram * meminfo->mem_unit) * 9.31 * pow(10, -10);
    used_ram[i] = total_ram[i] - (float)(meminfo->freeram * meminfo->mem_unit) * 9.31 * pow(10, -10);
    total_swap[i] = (float)((meminfo->totalram + meminfo->totalswap) * meminfo->mem_unit) * 9.31 * pow(10, -10);
    used_swap[i] = total_swap[i] - (float)((meminfo->freeswap + meminfo->freeram)* meminfo->mem_unit) * 9.31 * pow(10, -10);

    free(meminfo);

    return;
}


//cpu_info calulates the cpu usage. Function takes in pointers to previous total cpu time and total cpu usage time. 
void cpu_info(int i, long int *prev_t_total, long int *prev_t_usage, int *core_count, float *cpu_usage_percent){
	FILE *file = fopen("/proc/cpuinfo", "r");
	char *line = malloc(sizeof(char) * 400);

	//Reading the first 12 lines of /proc/cpuinfo to get to cores per cpu line. 
	for(int i = 0; i < 12; i++){
		fgets(line, 400, file);
	}

	free(line);
	line = malloc(sizeof(char) * 400);
	fgets(line, 400, file);
	//printf("The line: %s\n", line);
	core_count[i] = (int)strtol(line+11, NULL, 10);
	//printf("The count is %d\n", *core_count);

	fclose(file);

	//printing cores per cpu. 
	//printf("Number of %s", line);


	//The current total cpu time and usage time are needed for cpu usage calculation. So we initialize pointers for the cpu times to be stored.
	long int *user = malloc(sizeof(long int));
	long int *nice = malloc(sizeof(long int));
	long int *system = malloc(sizeof(long int));
	long int *idle = malloc(sizeof(long int)); 
	long int *iowait = malloc(sizeof(long int)); 
	long int *irq = malloc(sizeof(long int)); 
	long int *softirq = malloc(sizeof(long int));
	long int t_total, t_idle, t_usage;
	//float cpu_usage_percent;

	//Reading current cpu times through set_times function call.
	set_times(user, nice, system, idle, iowait, irq, softirq);

	//Calculating the percent of cpu use
	t_total = *user + *nice + *system + *idle + *iowait + *irq + *softirq;
	t_idle = *idle;
	t_usage = t_total - t_idle - *prev_t_usage;
	t_total -= *prev_t_total;
	cpu_usage_percent[i] = ((float)t_usage/t_total) * 100;

	//printing cpu usage.
	//printf("CPU Usage: %.2f\n", cpu_usage_percent);

	//Setting prev total and usage time to current values to prepare for the next cpu usage sample calculation
	*prev_t_total = t_total + *prev_t_total;
	*prev_t_usage = t_usage + *prev_t_usage;

	free(user);
	free(nice);
	free(system);
	free(idle);
	free(iowait);
	free(irq);
	free(softirq);
	free(line);
}


void get_users(int user_pipe){
	int num;
	char line[BUF_SIZE];
	struct utmp *info; // struct utmp stores info about user sessions
	setutent(); //opens utmp file for reading

	info = getutent(); //reads from utmp file
	while (info != NULL) {
		if (info->ut_type == USER_PROCESS) { //check if info is type USER_PROCESS 
			sprintf(line,"%s       %s %s", info->ut_user, info->ut_line, info->ut_host);
			
			// Getting the number of bytes that is going to be sent to the pipe for the user string (line).
			num = strlen(line) + 1;
			// Writing the number of bytes to the pipe. 
			error_checked_write(user_pipe, &num, sizeof(int), "Writing num");
			// Writing the user string to the pipe.
			error_checked_write(user_pipe, line, num, "Writing User");
		}
		info = getutent(); // reads from utmp file
	}
	endutent(); // closes utmp file;

	num = strlen("done") + 1; // Calculating the number of bytes that will be sent to the pipe to send "done"
	// Sending the num of bytes of the string to the pipe.
	error_checked_write(user_pipe, &num, sizeof(int), "Writing num");
	// Then sending the actual string "done" to mark the end of users for this sample.
	error_checked_write(user_pipe, "done", num, "Writing done");
}