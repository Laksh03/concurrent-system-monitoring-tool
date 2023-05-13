# concurrent-system-monitoring-tool
This is a modifyed version of the linux-system-monitoring-tool program that utilizes forks, pipes, and signals to make more efficient use of hardware through concurrency. 

How to run the program: 

C program that will report different metrics of the utilization of a linux system as described below.

The program should accept several command line arguments:

--system
        
	to indicate that only the system usage should be generated


--user

        to indicate that only the users usage should be generated


--graphics  (+2 bonus points)

        to include graphical output in the cases where a graphical outcome is possible as indicated below.


--sequential

        to indicate that the information will be output sequentially without needing to "refresh" the screen (useful if you would like to redirect the output into a file)

 

--samples=N

        if used the value N will indicate how many times the statistics are going to be collected and results will be average and reported based on the N number of repetitions.
If not value is indicated the default value will be 10.


--tdelay=T

        to indicate how frequently to sample in seconds.
If not value is indicated the default value will be 1 sec.


How did I solve the problem of concurrency?

What I did step by step: 

(Set-up)
First I made 3 pipes for the data transfer RAM INFO, CPU INFO, and USER INFO. Then I forked 3 times in a for loop. One fork per loop and the children immediately break out of the loop so it is just the parent that forks. Since this was done in a loop, I used the loop counter as the select/differentiator between the 3 children, so from here on it was very easy for me to program and select whether child 1,2,3 or parent will execute line x of code by just putting the code in a if statement that checks whether or not the pid returned by fork was zero or not and if it was zero what the loop counter is set too (again to differentiate between the 3 children). Another thing that happens in this loop is that after forking the parent and child will close the ends of the pipes they will not be using, so read end for the children and write end for the parent. 

(Samples)
Now set-up is done and I have a parent process and 3 children process going into my sample collecting and printing loop (the main loop of the program). From here it's very easy using the selection if statement idea from the first paragraph. I basically took my 3 functions from A1 that collected and printed the data (user, mem, cpu) when called and split them into 6 functions, 3 of which that collect data and 3 functions that print data. From here I get my children processes to collect the data using the 3 collection functions in stat_functions.c And this is done at the same time as each child is set to call a single data collection function per loop. After calling the data collection functions the children write the data to the pipe. Now, what the parent does in this sample loop is that it reads from each of the pipes (of course depending on the CLA arguments), in the order of printing required. So for example, it goes read ram pipe then, calls print ram data function. 

(So, "full marks will be given to implementations employing pipes to communicate the results to the main process." is exactly what happens in the loop).

(End of program)
Then, before the end of the program, I have the parent program use the wait() function 3 times to ensure the child processes have finished running and to avoid any zombie or orphan processes. 

(Error Checking)
So, how I handled error checking was to collect the return values of all the various functions and if they indicated and error I would use perror(), to report it and exit the program. For the read and write calls, during my testing I noticed sometimes these calls would get interrupted by the signals I sent through stdout. So for error handling with these functions when -1 was returned I checked what errno was set to and if it indicated that the function call had been interrupted I would loop until it the call fully executed. 
Then, I noticed that the sleep() call would not sleep the full amount when signals interrupted it. So I added error checking for it as well by just checking it's return value which is the amount of time it actually slept minus the amount it was supposed to sleep (so the amount of sleep it didn't do). So, for error checking I just put the sleep call in a while loop until it returned 0 indicating it slept the full amount.

(--graphics flag)
Was pretty simple. All I had to do was send the graphics flag to my print functions for cpu_info and mem_info and if graphics flag was set I just needed to do the extra calculations and display the graphics as well. The graphics for ram is that a symbol for about every 0.01 change in virtual used ram will be printed. And graphics for cpu is that starting at 1 bar for every 1% of cpu usage an extra bar is printed. 

(Signal handling)
For signal handling I just used signal(SIGTSTP, SIG_IGN) to ignore the ctrl+z and for ctrl+c I just made a simple handler that prompted the user the question and if y is entered I used kill(-1 * getpid(), SIGKILL) to kill all the children and parent processes. How I avoided the children from responding to these signals was to after forking get each child to call signal(SIGTSTP, SIG_IGN), and signal(SIGINT, SIG_IGN). How I made sure the parent process didn't accept more ctrl+c signals while executing the ctrl_c_handler was to just use signal(SIGINT, SIG_IGN) in the handler and set it back to ctrl_c_handler before finishing the function. There is some extra implementation in the handler and an extra function called delete_pending_sigint to detect and remove a SIGINT signal from the queue because during testing I noticed for some reason the line signal(SIGINT, SIG_IGN) in the ctrl_c_handler did not ignore the SIGINT signal the first time the handler is called. 

(CPU usage calculation)
Has been updated to the calculation provided in the assignment page.

(How to use the program?)
Same way as in A1, explained at the bottom. Except this time you can use --graphics and positional arguments work properly this time.

(Functions)
Some of the old functions are explained below from A1 as they aren't really that much different.

void error_checked_close(int fd);
void error_checked_write(int fd, void *ptr, size_t size, char string[]);
void error_checked_read(int fd, void *ptr, size_t size, char string[]);

These 3 functions do exactly as they are named, they take in the arguments required of the close(), write() and read() calls and char string[] for perror argument. And they perform the functions with error checking. So making sure to check errno and return values of the functions and making sure the functions get executed fully. 

void delete_pending_sigint();
The function deletes any SIGINT signals that are queued up. Using the sigprogmask() and sigwait() functions. 

void ctrl_c_handler(int sig);
Handles any SIGINT signal. This handler prompts the user if they wish to terminate the program. If user says (y) the program (parent & children) processes get killed. If user says no handler checks if there's any other pending SIGINT signal and dequeues it with a delete_pending_sigint() call, then returns allowing the parent process to continue executing main()

(Modules)

print_utils.c contains all the printing functions for cpu_info, mem_info and user_info
stat_functions.c contains all the sample data collection functions for cpu_info, mem_info, and user_info.

Print functions can be explained a little bit in A1 doc below. 

<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ReadMe from A1 down below >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

How did I solve the problem?

For the problem of acquiring and reading all the data I needed I looked over the listed modules, documentation, and files on the assignment page. I started with the system information data. At first I started looking through a bunch of files such as /proc/version, /proc/sys/kernel/, etc. But then I realized the listed modules did all the work for me. I used <sys/utsname.h> for the system information, <utmp.h> for the user data, and <sys/sysinfo.h> for the memory data. These modules did the work of acquiring the data for me. For the cpu info however, I read the file /proc/stat & /proc/cpuinfo and used strtol() get the required data. 

For the problem of calculating cpu usage I saw the website linked on piazza 127: https://www.kgoettler.com/post/proc-stat/ From where I got the formula for calculating cpu usage. This formula makes sense to me. CPU usage is basically the percent of time in your sample where the cpu is actually running and doing calculations and stuff, while the rest of the time is where the cpu stays idle or waiting. 

For the problem of refreshing the screen and getting the output to look like the demo video, I researched ESCape codes, and found a code which allowed me to clear the terminal. After this I just stored my memory samples in arrays and messed around with loops to solve the display and refresh problem. I had my arrays that stored samples then I basically had two loops, one that printed samples and one that printed new lines. So as the program ran I had variables that made the new line loop run fewer loops and the sample loop run more loops. 

For the problem of command line arguments: I just had a variable flags for --system, --user, --sequential. So when I checked for the arguments using if statements I would update the corresponding variables and have the program call certain functions based on the flags. Then for samples and delays I just made clever use of which position the '=' sign was to differentiate. 

Funtions:

void print_mem_info(int samples, int i, float total_ram[], float used_ram[], float total_swap[], float used_swap[])

This function takes in float arrays total_ram[], used_ram[], total_swap[], and used_swap[]. These arrays contain the memory sample data which is used for printing. The int samples and int i are to determine how many samples have already been taken by just doing samples - i, since i was an iterator from the loop that the function gets called from. This allows print_mem_info to print the correct number of blank lines and memory samples to help give the refresh effect.

void print_system_information()

This function makes use of the utsname module and prints the required system information. 

void print_users()

This function makes use of the utmp module and prints the user data.

void set_times(long int *user, long int *nice, long int *system, long int *idle, long int *iowait, long int *irq, long int *softirq, long int *steal)

This function takes in pointers for all those cpu time measurements found in /proc/stat. The function reads the first line from /proc/stat and uses strtol() to store the times in the addresses given by the pointers.

void print_cpu_info(long int *prev_t_total, long int *prev_t_usage)

This function takes in long int *prev_t_total, and long int *prev_t_usage. The values stored in these addresses are needed for the function to calculate cpu usage and print it. The calculation used for cpu usage is found at https://www.kgoettler.com/post/proc-stat/ Once that is done the function takes the new t_total and new t_usage it calculated, and assigns it to *prev_t_total and *prev_t_usage. So that when the function gets called again with these pointers it's then able to make another cpu usage calculation.

