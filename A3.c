#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utmp.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h> 

#include "print_utils.h"
#include "stats_functions.h"
#define BUF_SIZE 1024

// This function closes file descriptor fd with error checking
void error_checked_close(int fd){
	if(close(fd) == -1){
		perror("close");
		exit(1);
	}
}

// This function writes size bytes from ptr into fd file desriptor, with string as perror msg. 
void error_checked_write(int fd, void *ptr, size_t size, char string[]){
	if(write(fd, ptr, size) == -1){ // If write didn't execute successfully
		if(errno == EINTR){ // If write call was interrupted
			while(1){ // Keep executing write call until it is successful
				write(fd, ptr, size);
				if(errno != EINTR){
					break;
				}
				printf("hello there write\n");
			}
		}else{ // If write call didn't execute successfully but it also wasn't inturrupted. 
			perror(string);
			exit(1);
		}
	}	
}

// This function reads size bytes into ptr from fd file desriptor, with string as perror msg. 
void error_checked_read(int fd, void *ptr, size_t size, char string[]){
	if(read(fd, ptr, size) == -1){ // If read didn't execute successfully
		if(errno == EINTR){ // If read call was interrupted
			while(1){ // Keep executing read call until it is successful
				if(errno != EINTR){
					break;
				}
			}
		}else{ // If read call didn't execute successfully but it also wasn't inturrupted.
			perror(string);
			exit(1);
		}
	}	
}

// This function deletes a pending SIGINT signal
void delete_pending_sigint() {
    sigset_t sigset; // Stores a set of signals
    int sig; // Will store a signal number

    sigemptyset(&sigset); // Sets sigset to an empty set.
    sigaddset(&sigset, SIGINT); // Add the SIGINT signal to the set

    // Blocks SIGINT signal
    if (sigprocmask(SIG_BLOCK, &sigset, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Wait for and remove the pending SIGINT signal
    if (sigwait(&sigset, &sig) != 0) {
        perror("sigwait");
        exit(EXIT_FAILURE);
    }

    // Unblock SIGINT signal
    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
}

// This function handles the ctrl+c signal
void ctrl_c_handler(int sig) {
	signal(SIGINT, SIG_IGN); // Setting the SIGINT to be ignored

	sigset_t pending_signals; // To store the any pending signals

	char response; // To store user input

	printf("\nDo you want to quit the program? (y/n): ");
	fflush(stdout);
	scanf(" %c", &response);

	// Sets all pending signals to pending_signals
	if (sigpending(&pending_signals) == 0) {
		if (sigismember(&pending_signals, SIGINT)) { // Check to see if pending signal is a SIGINT
			//printf("pending\n");
			delete_pending_sigint(); // Function call to delete the pending SIGINT
		} else {
			//printf("not pending\n");
		}
	} else { // Error Checking
		perror("sigpending");
	}

	if (response == 'y') { // If user answers yes to exit program.
		kill(-1 * getpid(), SIGTERM); // Parent process sends a SIGTERM signal to all child processes and itself. 
		printf("\033c");
	} else { // User wants to continue program.
		signal(SIGINT, ctrl_c_handler); // Set the SIGINT signal hanlder back to ctrl_c_handler.
	}
}


int main(int argc, char **argv){
    // Setting parent process new signal handlers
    signal(SIGINT, ctrl_c_handler);
    signal(SIGTSTP, SIG_IGN);

	//Clearing terminal 
	printf("\033c");

	//Declaring and initializing default sample and time (delay). As well as the flags. 
	int samples = 10, time = 1;
	int system_flag = 0, user_flag = 0, sequential_flag = 0, graphics_flag = 0;
	int iteration = 1;
	int pos_arg_entered = 0;

	//if command line arguments present but arguments don't have '-', then samples & time are updated from default 10 samples and 1 sec to user inputs.
	if(argc != 1){ // If there are CLA. 
		for(int i = 1; i < argc; i++){ // loop through all command line arguments checking the type of argument and updating corresponding flag.
			if(argv[i][0] != '-'){ // If argument has no dash it's a positional argument.
				if(!pos_arg_entered){ // Check to see if a positional argument has already been parsed.
					samples = strtol(&argv[i][0], NULL, 10); // If a positional argument hasn't been entered then set new samples.
					pos_arg_entered = 1; // Set flag to one since positional argument has now been entered. 
				}else{
					time = strtol(&argv[i][0], NULL, 10); // If positional argument has been entered we use the next positional argument to set time.
				}
			}else{ //Command line arguments have '-'
				if(strcmp(&argv[i][0], "--system") == 0){
					system_flag = 1;
				}else if(strcmp(&argv[i][0], "--user") == 0){
					user_flag = 1;
				}else if(strcmp(&argv[i][0], "--sequential") == 0){
					sequential_flag = 1;
				}else if(strcmp(&argv[i][0], "--graphics") == 0){
					graphics_flag = 1;
				}else if(argv[i][9] == '=' && strlen(&argv[i][0]) > 10){ // Check for "--samples=n" argument
					samples = strtol(&argv[i][10], NULL, 10); // Updating samples variable with user input
				}else if(argv[i][8] == '=' && strlen(&argv[i][0]) > 9){ // check for "--tdelay=T" argument
					time = strtol(&argv[i][9], NULL, 10); // Updating time (delay) variable with user input.
				}
			}
		}
	}
	// If both the user_flag and system_flag are set then unset them both because otherwise the flags cancel each others outputs resulting in neither user or system output.
	if(user_flag && system_flag){
		user_flag = 0;
		system_flag = 0;
	}


	// Arrays to store all memory samples taken. 
	float *total_ram = (float *) malloc(sizeof(float) * samples);
	float *used_ram = (float *) malloc(sizeof(float) * samples);
	float *total_swap = (float *) malloc(sizeof(float) * samples);
	float *used_swap = (float *) malloc(sizeof(float) * samples);


	//Declaring pointers to get calculate the prev total time & usage values for the cpu usage calculation & to store core_count. 
	long int *user = malloc(sizeof(long int));
	long int *nice = malloc(sizeof(long int));
	long int *system = malloc(sizeof(long int));
	long int *idle = malloc(sizeof(long int)); 
	long int *iowait = malloc(sizeof(long int)); 
	long int *irq = malloc(sizeof(long int)); 
	long int *softirq = malloc(sizeof(long int));
	long int *prev_t_total = malloc(sizeof(long int));
	long int *prev_t_usage = malloc(sizeof(long int));

	// Arrays to store all CPU samples taken.
	int *core_count = (int *) malloc(sizeof(int) * samples);
	float *cpu_usage_percent = (float *) malloc(sizeof(float) * samples);

	//Initializing the times by using set_times function to read through /proc/stat
	set_times(user, nice, system, idle, iowait, irq, softirq);
	*prev_t_total = *user + *nice + *system + *idle + *iowait + *irq + *softirq;
	*prev_t_usage = *prev_t_total - *idle;
	
	// Freeing all the malloced variables. 
	free(user);
	free(nice);
	free(system);
	free(idle);
	free(iowait);
	free(irq);
	free(softirq);
	
	// Time delay for difference between cpu time reads. 
	// Error checking to make sure enough sleep time has been done in case signals intrupt execution.
	int sleep_left = time;
	while(sleep_left != 0){
		sleep_left = sleep(sleep_left);
	}
	sleep_left = time; // Resetting the amount of sleep_left back to time (t-delay).
	
	// Creating 3 pipes for data transfer between 3 children to parent process.
	int mem_pipe[2];
	if(pipe(mem_pipe) == -1){
		perror("pipe");
		exit(1);
	}

	int cpu_pipe[2];
	if(pipe(cpu_pipe) == -1){
		perror("pipe");
		exit(1);
	}

	int user_pipe[2];
	if(pipe(user_pipe) == -1){
		perror("pipe");
		exit(1);
	}	
	
	pid_t pid;
	int child_num;
	fflush(stdout);
	for(child_num = 0; child_num < 3; child_num++){ // Loops 3 times to fork 3 times and create 3 child processes. 
		pid = fork(); // Setting fork return value to pid. 
			
		if(pid == -1){ // Error checking.
			perror("fork");
			exit(1);
		}

		if(pid == 0){ // If child process. 
			if(child_num == 0){ // If the first child process. 
				if(close(mem_pipe[0]) == -1){ // First child process in charge of collecting and writing memory to mem_pipe so close read end.
					perror("close"); // Error checking. 
					exit(1);
				}
			}else if(child_num == 1){ // If the second child process. 
				if(close(cpu_pipe[0] == -1)){ // Second child process in charge of collecting and writing cpu_info to cpu_pipe so close read end.
					perror("close"); // Error checking. 
					exit(1);
				}
			}//else if(child_num == 2){ // Same logic as before but results in bugs in program so commented out, which is fine as long as correct file descriptors are being used at all times.
			// 	printf("Read end3: %d\n", user_pipe[0]);
			// 	if(close(user_pipe[0] == -1)){
			// 		perror("close");
			// 		exit(1);
			// 	}
			// }
			break;
		}else{ // Else It's the parent process.
			if(child_num == 0){ // If on first itereation, that means first child got created.
				if(close(mem_pipe[1]) == -1){ // Close write end of mem_pipe since that's the pipe the first child is going to use. 
					perror("close"); // Error checking. 
					exit(1);
				}
			}//else if(child_num == 1){ // Same logic as before but results in bugs in the program so commented out, which is fine as long as correct file descriptors are being used at all times.
			// 	printf("%d\n", cpu_pipe[1]);
			// 	if(close(cpu_pipe[1] == -1)){
			// 		perror("closee");
			// 		exit(1);
			// 	}
			// }
			// else if(child_num == 2){
			// 	printf("Write end4: %d\n", user_pipe[1]);
			// 	sleep(10);
			// 	if(close(user_pipe[1] == -1)){
			// 		perror("close");
			// 		exit(1);
			// 	}			
			// }
		}

	}
		
	// Setting children processes to ignore ctrl+c and ctrl+z signals.
	if(pid == 0){
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
	}

	//for loop for the samples that will be taken.
	for(int i = 0; i < samples; i++){
		if(pid != 0){
			printf("Nbr of samples: %d -- every %d secs\n", samples, time);
		}

		//if in sequential mode the loop will print out the interation number. 
		if(sequential_flag == 1){
			if(pid != 0){ // If parent process. 
				printf(">>> iteration %d\n", iteration);
				iteration++;
			}
		}
		
		//If --user argument was not present, then memory info will be printed.
		if(user_flag != 1){
			if(pid == 0 && child_num == 0){ // If first child process. 
				//Calling mem_info to store memory sample into arrays.
				mem_info(i, total_ram, used_ram, total_swap, used_swap);

				// Writing memory info to the mem_pipe. 
				error_checked_write(mem_pipe[1], &total_ram[i], sizeof(float), "Total Ram Write");
				error_checked_write(mem_pipe[1], &used_ram[i], sizeof(float), "Used Ram Write");
				error_checked_write(mem_pipe[1], &total_swap[i], sizeof(float), "Total Swap Write");
				error_checked_write(mem_pipe[1], &used_swap[i], sizeof(float), "Used Swap Write");		

			}else if(pid != 0){ // Else it's the parent process. 
				// Reading memory info from the mem_pipe. 
				error_checked_read(mem_pipe[0], &total_ram[i], sizeof(float), "Total Ram Read");
				error_checked_read(mem_pipe[0], &used_ram[i], sizeof(float), "Used Ram Read");
				error_checked_read(mem_pipe[0], &total_swap[i], sizeof(float), "Total Swap Read");
				error_checked_read(mem_pipe[0], &used_swap[i], sizeof(float), "Used Swap Read");

				//Printing memory samples by sending the arrays and loop data as arguments to get desired printing outputs
				print_mem_info(samples, i, total_ram, used_ram, total_swap, used_swap, graphics_flag);	
			}
		}

		//If --system argument was not entered then print user information
		if(system_flag != 1){
			if(pid == 0 && child_num == 2){ // If third child process. 
				get_users(user_pipe[1]); // Call get_users sending the write end of pipe fd so user_info can be written into user_pipe.
			}else if(pid != 0){ // Else it's parent process. 
				print_users(user_pipe[0]); // Call print_users sending the read end of pipe fd so user_info can be read out of user_pipe and printed.
			}
		}

		//If --user argument was not entered then print cpu information.
		if(user_flag != 1){
			if(pid == 0 && child_num == 1){ // If second child process. 
				//Calling cpu_info to obtain core_count and cpu_usage_percent stats. 
				cpu_info(i, prev_t_total, prev_t_usage, core_count, cpu_usage_percent);

				// Writing CPU info to cpu_pipe.
				error_checked_write(cpu_pipe[1], &core_count[i], sizeof(int), "Core Count Write");
				error_checked_write(cpu_pipe[1], &cpu_usage_percent[i], sizeof(float), "CPU Usage Write");

			}else if(pid != 0){ // Else it's the parent process. 
				// Reading CPU info from pipe.
				error_checked_read(cpu_pipe[0], &core_count[i], sizeof(int), "Core Count Read");
				error_checked_read(cpu_pipe[0], &cpu_usage_percent[i], sizeof(float), "CPU Usage Read");

				//Printing cpu info by sending core count and cpu usage info. 
				print_cpu_info(i, samples, core_count, cpu_usage_percent, graphics_flag);
			}
		}
		

		//The delay between samples with if statement to skip the delay after last sample has been printed.
		if(i != samples - 1){
			// Error checking to make sure enough sleep time has been done in case signals intrupt execution.
			while(sleep_left != 0){
				sleep_left = sleep(sleep_left);
			}
			sleep_left = time; // Resetting the amount of sleep left to time (t-delay).
		}

		//If --sequential argument was not entered then erase screen for new outputs. 
		if(sequential_flag != 1){
			if(pid != 0){ // If Parent process. 
				if(i != samples - 1){ // Only if it's not the last output. 
					printf("\033c");
				}
			}
		}
	}

	// Freeing all the malloced variables to avoid memory leaks. 
	free(prev_t_total);
	free(prev_t_usage);
	free(core_count);
	free(cpu_usage_percent);
	free(total_ram);
	free(used_ram);
	free(total_swap);
	free(used_swap);

	// End of program pipe closing logic. 
	if(pid == 0 && child_num == 0){
		error_checked_close(mem_pipe[1]);
	}else{
		if(pid != 0){
			error_checked_close(mem_pipe[0]);
		}
	}
	// End of program pipe closing logic.
	if(pid == 0 && child_num == 1){
		error_checked_close(cpu_pipe[1]);
	}else{
		if(pid != 0){
			error_checked_close(cpu_pipe[0]);
		}
	}
	// End of program pipe closing logic.
	if(pid == 0 && child_num == 2){
		error_checked_close(user_pipe[1]);
	}else{
		if(pid != 0){
			error_checked_close(user_pipe[0]);
		}
	}	

	// Function call to print system information with if statement allows only the parent to execute. 
	if(pid != 0){
		print_system_information();

		for(int x = 0; x < 3; x++){ // Running the wait() function 3 times with error checking to avoid zombie processes.
			int check = wait(NULL);
			if(check == -1){
				perror("wait");
				exit(1);
			}
		}
	}
	return 0;
}