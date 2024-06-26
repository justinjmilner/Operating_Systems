/*

 * logging.c
 *
 * For: UVic CSC 360, Spring 2024
 *
 * Copyright: Mike Zastre 2024
 *
 * DO NOT MODIFY THIS SOURCE-CODE FILE WITHOUT EXPRESS WRITTEN
 * PERMISSION FROM A MEMBER OF THE TEACHING TEAM.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logging.h"

char kosmos_log[LOG_MAX_LINES][LOG_MAX_LINE_LEN];
unsigned char kosmos_log_numlines;


/* No code outside of this .c file should be able to directly
 * access this mutex.
 */
static pthread_mutex_t kosmos_log_mutex;


void kosmos_log_init() {
    int row;

    for (row = 0; row < LOG_MAX_LINES; row++) {
        kosmos_log[row][0] = '\0';
    }

    kosmos_log_numlines = 0;

    if (pthread_mutex_init(&kosmos_log_mutex, NULL) != 0) {
        fprintf(stderr, "logging: problems initializing kosmos_log_mutex\n");
        exit(1);
    }

}


/*
 * Can be used at any time -- outputs the current contents of the log.
 */
void kosmos_log_dump() {
    pthread_mutex_lock(&kosmos_log_mutex);

    printf("Lines in log: %d\n", kosmos_log_numlines);

    for (int r = 0; r < kosmos_log_numlines; r++) {
        printf("%s\n", kosmos_log[r]);
    } 
    /* Do stuff. */

    pthread_mutex_unlock(&kosmos_log_mutex);
}


/*
 * Meant for specific ethynyl-radical creation reports.
 */
void kosmos_log_add_entry(int molecule_num, int c_one, int o_one, int h_one,
    int h_two, int h_three, char *instigator)
{
    char line[LOG_MAX_LINE_LEN];

    if (kosmos_log_numlines == LOG_MAX_LINES) {
        fprintf(stderr, "logging: The log is full with %d entries already\n",
            kosmos_log_numlines);
        exit(2);
    }

    sprintf(line, "%03d: c%03d o%03d h%03d h%03d h%03d (%s)",
        molecule_num, c_one, o_one, h_one, h_two, h_three, instigator);

    pthread_mutex_lock(&kosmos_log_mutex);
    strncpy(kosmos_log[kosmos_log_numlines], line, LOG_MAX_LINE_LEN);
    kosmos_log_numlines++;
    pthread_mutex_unlock(&kosmos_log_mutex);
}


/*
 * Meant for more general log messages, if that will help during development.
 */
void kosmos_log_add_line(char *line) {
    if (kosmos_log_numlines == LOG_MAX_LINES) {
        fprintf(stderr, "logging: The log is full with %d entries already\n",
            kosmos_log_numlines);
        exit(2);
    }

    pthread_mutex_lock(&kosmos_log_mutex);
    strncpy(kosmos_log[kosmos_log_numlines], line, LOG_MAX_LINE_LEN);
    kosmos_log_numlines++;
    pthread_mutex_unlock(&kosmos_log_mutex);
}
