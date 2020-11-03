#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include "util.h"

#define ARRAY_SIZE 20
#define MAX_INPUT_FILES 100
#define MAX_RESOLVER_THREADS 10 
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH 46 //INET6 ADDRSTRLEN


typedef struct Shared_Array{
    pthread_mutex_t buffer_lock;
    pthread_cond_t buffer_full;
    pthread_cond_t buffer_empty;
    pthread_mutex_t requester_file_lock;
    int index;
    int requesters_working;
    int num_data_files;
} Shared_Array;

struct RequesterThreadArgs {
    FILE *requester_log_file;
    Shared_Array *buffer_arg;
    char **shared_buffer;
    FILE **datafiles;
};

struct ResolverThreadArgs {
    FILE *datafile;
    Shared_Array *buffer_arg;
    char **shared_buffer;
    pthread_mutex_t output_file_lock;
};


int main(int argc, char *argv[]);
void *requester(void* input);
void *resolver(void* input);
int service_file(FILE *inputfile, char **buffer_array, Shared_Array *buffer_struct);
void print_results(double time, int num_requesters, int num_resolvers, FILE *input_file, FILE *output_file);




#endif //MULTI_LOOKUP_H


