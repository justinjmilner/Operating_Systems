/* getstats.c 
 *
 * CSC 360, Spring 2024
 *
 * - If run without an argument, prints information about 
 *   the computer to stdout.
 *
 * - If run with a process number created by the current user, 
 *   prints information about that process to stdout.
 *
 * Please change the following before submission:
 *
 * Author: Din Grogu
 * Login:  babyyoda@uvic.ca 
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Note: You are permitted, and even encouraged, to add other
 * support functions in order to reduce duplication of code, or
 * to increase the clarity of your solution, or both.
 */

int check_file_opened(FILE* fp, const int* pid) {
    if (fp == NULL) {
        if (pid != NULL) {
            // If pid is provided, print the error message with pid
            printf("Failed to open /proc/%d/status", *pid);
        } else {
            // If pid is not provided, print a generic error message
            printf("Failed to open file\n");
        }
        return -1;
    } 
    return 0;
}

void print_process_info(char * process_num) {
    // Initialize data reading variables
    FILE *fp;
    char buffer[1024];
    char path[256];
    int pid = atoi(process_num);
    
    // Try to open the virtual file containing the process name
    sprintf(path, "/proc/%d/comm", pid);
    fp = fopen(path, "r");
    if (check_file_opened(fp, &pid) < 0) {
        return;
    } else if (fgets(buffer, sizeof(buffer), fp)) { 
        printf("Process number: %d\n", pid); // Print the number and the name of the process
        printf("Name:   %s", buffer);
    }
    fclose(fp);

    // Try to open the file containing the command that started the process
    sprintf(path, "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (check_file_opened(fp, &pid) < 0) {
        return;
    } else if (fgets(buffer, sizeof(buffer), fp)) {
        printf("Filename (if any): %s\n", buffer); // Print the console command that started the process (if any) 
    }
    fclose(fp);

    // Try to open the file containing the number of threads and context switches running in the process
    int vol_switches = 0, nonvol_switches = 0, threads;
    sprintf(path, "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (check_file_opened(fp, &pid) < 0) {
        return;
    }
    
    // Look for number of threads and vol/nonvol context switches
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "Threads:", 8) == 0) {
            sscanf(buffer, "Threads: %d", &threads);
        } else if (strncmp(buffer, "voluntary_ctxt_switches:", 24) == 0) {
            sscanf(buffer, "voluntary_ctxt_switches: %d", &vol_switches);
        } else if (strncmp(buffer, "nonvoluntary_ctxt_switches:", 24) == 0) {
            sscanf(buffer, "nonvoluntary_ctxt_switches: %d", &nonvol_switches);
        }
    }
    int total_switches = vol_switches + nonvol_switches;
    printf("Threads:        %d\nTotal context switches: %d\n", threads, total_switches);
    fclose(fp);
} 


void print_full_info() {
    // Initialize data reading variables
    FILE *fp;
    char buffer[1024];

    // Try opening the virtual file containing cpuinfo
    fp = fopen("/proc/cpuinfo", "r");
    if (check_file_opened(fp, NULL) < 0) {
        return;
    }
    // Search for the model name of CPU, then grab the model name after the colon
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "model name", 10) == 0) {
            char* modelName = strchr(buffer, ':') + 2;
            printf("model name      : %s", modelName);
            break;
        }
    }

    // Find each CPU processor instance and count them all
    int cores = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strncmp(buffer, "processor", 9) == 0) {
            cores++;
        }
    }
    printf("cpu cores       : %d\n", cores);
    fclose(fp);

    // Try opening the virtual file containing the kernel version info
    fp = fopen("/proc/version", "r");
    if (check_file_opened(fp, NULL) < 0) {
        return;
    }
    // Print the version of Linux running in the environment
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Linux version %s", buffer);
    } else {
        printf("Failed to read kernel version\n");
    }
    fclose(fp);

    // Try opening the virtual file containing the total memory
    long totalMemory;
    fp = fopen("/proc/meminfo", "r");
    if (check_file_opened(fp, NULL) < 0) {
        return;
    }
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        sscanf(buffer, "MemTotal: %ld kB", &totalMemory);
        printf("MemTotal:       %ld kB\n", totalMemory);
    } else {
        printf("Failed to read total memory\n");
    }
    fclose(fp);

    // Try opening the virtual file containing up time
    fp = fopen("/proc/uptime", "r");
    if (check_file_opened(fp, NULL) < 0) {
        return;
    }
    double uptimeSeconds;
    if (fscanf(fp, "%lf", &uptimeSeconds) == 1) {
        int days = (int) uptimeSeconds / 86400;
        int hours = ((int) uptimeSeconds % 86400) / 3600;
        int minutes = ((int) uptimeSeconds % 3600) / 60;
        int seconds = (int) uptimeSeconds % 60;
        printf("Uptime: %d days, %d hours, %d minutes, %d seconds\n", days, hours, minutes, seconds);
    }
    fclose(fp);
}


int main(int argc, char ** argv) {  
    if (argc == 1) {
        print_full_info();
    } else {
        print_process_info(argv[1]);
    }
}
