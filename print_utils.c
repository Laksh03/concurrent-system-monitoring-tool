#define _POSIX_C_SOURCE 199309L
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
#include <errno.h>

#include "print_utils.h"
#include "stats_functions.h"
#include "A3.h"
#define BUF_SIZE 1024

//print_mem_info prints the memory data stored in the array arguments
void print_mem_info(int samples, int i, float total_ram[], float used_ram[], float total_swap[], float used_swap[], int graphics_flag){
	printf("---------------------------------------\n");
	printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
	
	//This for loop prints the memory samples stored in array arguments
	if(graphics_flag){ // If graphics flag is true. 
		for(int x = 0; x < i + 1; x++){ // Loop through all taken samples. 
			if(x == 0){ // If first sample print the default graphic. 
				printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB   |o 0.000 (%.2f)\n", used_ram[x], total_ram[x], used_swap[x], total_swap[x], used_swap[x]);			
			}else{ // Else print the cacluated graphic. 
				float f_difference = used_swap[x] - used_swap[x-1]; // Cacluating float difference between previous virtual ram and current virtual ram usage. 
				//printf("Here is the f_diff:%f\n", f_difference);

				int difference = (int) ((f_difference) * 100); // Calculating how much of a differnce was there in terms of hundreths of a GB of virtual ram. 

				//printf("Here is the reg_diff:%d\n", difference);
				char graphic[abs(difference) + 3]; // Declaring array to store the graphic, which will be abs(difference) big, plus 3 for null character and extra space. 
				
				graphic[0] = '|'; // Adding the bar for ram graphics.
				
				if(difference >= 0){ // If there is a difference (change) of at least plus 0.01 in ram. 
					for(int i = 1; i < difference + 1; i++){
						graphic[i] = '#'; // Add difference number of #'s to the graphic.
					}
					graphic[difference + 1] = '*'; // Add the * to graphic;
					graphic[difference + 2] = '\0'; // Add the null character to graphic. 
				}else{ // Else there is negative change / difference in ram. 
					difference = difference * -1; // Just turn the differnce into a positive number for the for loop. 
					for(int i = 1; i < difference + 1; i++){
						graphic[i] = ':'; // Add difference number of :'s to the graphic. 
					}
					graphic[difference + 1] = '@'; // Add the @ symbol to graphic when the ram has decreased.
					graphic[difference + 2] = '\0'; // Add the null character to the graphic string.
				}
				
				// Printing the ram sample with graphics. 
				printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB   %s %0.3f (%.2f)\n", used_ram[x], total_ram[x], used_swap[x], total_swap[x], graphic, f_difference, used_swap[x]);
			}
		}
	}else{ // Else graphic flag was not selected and we just print all the ram samples. 
		for(int x = 0; x < i + 1; x++){
			printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB\n", used_ram[x], total_ram[x], used_swap[x], total_swap[x]);
		}
	}

	//This for loop prints new lines for the specific amount of memory samples yet to be taken using int samples and int i.
	for(int x = 0; x < samples - i - 1; x++){
		printf("\n");
	}
	
	printf("---------------------------------------\n");
}


//print_system_information prints system information using utsname module.
void print_system_information(){
	//struct from utsname module stores system information
	struct utsname *system_information;
	system_information = malloc(sizeof(struct utsname));
	
	//uname function initializes the fields of struct utsname system_information
	int num = uname(system_information);

	//Checking for errors
	if(num == -1){
		perror("uname");
	}

	//Printing all system information from system_information fields.
	printf("---------------------------------------\n");
	printf("### System Information ###\n");
	printf(" System Name = %s\n", system_information->sysname);
	printf(" Machine Name = %s\n", system_information->nodename);
	printf(" Version = %s\n", system_information->version);
	printf(" Release = %s\n", system_information->release);
	printf(" Architecture = %s\n", system_information->machine);
	printf("---------------------------------------\n");	

	free(system_information);
}


//print_users function prints current users using utmp module
void print_users(int user_pipe){
	int length; 
	char line[BUF_SIZE];
	int num = 7;

	printf("### Sessions/users ###\n");

	while (num != 0) { // While we haven't reached the end of the file (pipe).
		error_checked_read(user_pipe, &length, sizeof(int), "Length Read"); // Reading the length of the string that will be read through the pipe next.

		num = read(user_pipe, line, length); // Read the user string from pipe.
		if(num == -1){ // If read call was an error.
			if(errno == EINTR){ // Check if it got inturrupted.
				num = read(user_pipe, line, length); // If inturrupted then redo. 
			}else{ // Actual error. 
				perror("User Read");
				exit(1);
			}
		}

		// If bytes of data was read from pipe enter if statement.
		if(num != 0){
			if(strncmp(line, "done", strlen("done")) == 0){ // If the data read from pipe was the line "done" break to stop reading from pipe.
				break;
			}		
			printf("%s\n", line); // Printing a user. 
		}
	}

	printf("---------------------------------------\n");
}


//print_cpu_info prints the cpu usage. Function takes in pointers to core count and cpu usage percent. 
void print_cpu_info(int i, int samples, int *core_count, float *cpu_usage_percent, int graphics_flag){

	//printing cores per cpu. 
	printf("Number of cpu cores: %d\n", core_count[i]);

	//printing cpu usage.
	printf("CPU Usage: %.2f%%\n", cpu_usage_percent[i]);

	if(graphics_flag){ // If graphics flag was selected.
		int num; 
		for(int x = 0; x < i + 1; x++){ // For each cpu usage percent sample.
			num = (int) cpu_usage_percent[x] + 1; // Add 1 to the cpu usage percent just to have minimum 1 bars of graphics display then flooring down to int. 
			char str[num + 1]; // Declaring graphic string.
			for(int n = 0; n < num; n++){
				str[n] = '|'; // Initializing the correct number of bars. 
			}
			str[num] = '\0'; // Terminating the string.
			printf("\t %s %.2f\n", str, cpu_usage_percent[x]); // Printing CPU graphical output. 
		}
	}
}