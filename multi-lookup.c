#include "multi-lookup.h"

int main(int argc, char *argv[]){

    //Will hold start and end times of program
    struct timeval startTime;
    struct timeval endTime;

    // gettimeofday function gets time and puts this in struct startTime 
    // startTime.tv_sec gets in seconds, startTime.
    gettimeofday(&startTime, NULL);

    //  ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]
    if ((argc < 6) | (argc > (5+MAX_INPUT_FILES))){
        fprintf(stderr, "ERROR, not enough or too many arguments provided");
        exit(1);
    }

    //command line arguments
    int num_requesters = atoi(argv[1]);
    int num_resolvers = atoi(argv[2]);
    char *requester_log = argv[3];
    char *resolver_log = argv[4];

    // Check # of requesters to make sure valid
    if ((num_requesters <= 0) |  (num_requesters > MAX_REQUESTER_THREADS)){
        fprintf(stderr, "ERROR, invalid input for # requester threads");
        exit(1);
    } 

    // Check # of resolver threads to make sure valid 
    if ((num_resolvers <= 0) |  (num_resolvers > MAX_RESOLVER_THREADS)){
        fprintf(stderr, "ERROR, invalid input for # resolver threads");
        exit(1);
    } 

    // Check access to requester log to make sure valid
    if (access(requester_log, R_OK|W_OK  ) == -1){
        fprintf(stderr, "ERROR, requester log doesn't exist or doesn't have read and write permissions");
        exit(1);
    } 

    //Check access to resolver log to make sure valid
    if (access(resolver_log, R_OK|W_OK  ) == -1){
        fprintf(stderr, "ERROR, resolver log doesn't exist or doesn't have read and write permissions");
        exit(1);
    }

    //Create Shared Buffer
    char **imp_shared_buffer = malloc(ARRAY_SIZE * sizeof(char*));

    for (int i = 0; i < ARRAY_SIZE ; i++){
        imp_shared_buffer[i] = malloc(MAX_NAME_LENGTH * sizeof(char));
    }

    //Create parameter for number of input files 
    int num_files = argc -5;
    if (num_files > MAX_INPUT_FILES){
        fprintf(stderr, "ERROR, too many data files");
        exit(1);
    }

    //Create array to store each file path reference
    char *input_data_files[num_files];
    for (int i = 0; i < num_files; i++){
        input_data_files[i] = argv[5+i];
    }

    //Check access to data files
    for (int i = 0; i< num_files; i++){
        if (access(input_data_files[i], R_OK|W_OK  ) == -1){
            fprintf(stderr, "ERROR, data file doesn't exist or doesn't have read and write permissions");
            exit(1);
        } 
    }

    //allocate memory for shared buffer struct that all requester and worker threads utilize
    Shared_Array *shared_buffer_struct = malloc(sizeof(Shared_Array));
    shared_buffer_struct->buffer_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    shared_buffer_struct->buffer_full = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    shared_buffer_struct->buffer_empty = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    shared_buffer_struct-> requester_file_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    shared_buffer_struct->index=0;
    shared_buffer_struct->requesters_working = num_requesters;
    shared_buffer_struct->num_data_files = num_files;

    //open files to read or write
    FILE *write_resolver_file = fopen(resolver_log, "w");
    FILE *write_requester_file = fopen(requester_log, "w");
    FILE **incoming_data_files = malloc(sizeof(FILE*) * num_files);
    for (int i =0; i < num_files; i++){
        incoming_data_files[i] = fopen(input_data_files[i], "r");
    }
    

    //create structs to pass arguments to requester and resolver threads
    struct RequesterThreadArgs req_args;
    req_args.datafiles = incoming_data_files;
    req_args.requester_log_file = write_requester_file;
    req_args.buffer_arg = shared_buffer_struct;
    req_args.shared_buffer = imp_shared_buffer;

    struct ResolverThreadArgs res_args;
    res_args.datafile = write_resolver_file;
    res_args.buffer_arg = shared_buffer_struct;
    res_args.shared_buffer = imp_shared_buffer;
    res_args.output_file_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    //Create an array to store the thread IDs of the requester and resolver threads
	pthread_t *requester_ids = malloc(sizeof(pthread_t) * num_requesters);
    pthread_t *resolver_ids = malloc(sizeof(pthread_t) * num_resolvers);

    

    //make sure requester threads created properly
    int errno;
    for (int i = 0; i < num_requesters; i ++){
        if(pthread_create(&requester_ids[i], NULL, requester, (void *)&req_args) == 0);
         else {
                fprintf(stderr,"PROBLEM in requester thread: %s\n", strerror( errno ));
            }
    }

    //make sure resolver threads created properly
    for (int i = 0; i < num_resolvers; i ++){
    if(pthread_create(&resolver_ids[i], NULL, resolver, (void *)&res_args) == 0);
    else {
        fprintf(stderr,"PROBLEM in resolver thread: %s\n", strerror( errno ));
        }
    }
    

        //make sure requester threads joined properly
   for (int i = 0; i < num_requesters; i ++){
        if(pthread_join(requester_ids[i], 0) == 0);
        else {
            fprintf(stderr,"COULNT JOIN REQUESTER THREAD: %s\n", strerror( errno ));
        }
   }



    //make sure resolver threads joined properly
    for (int i = 0; i < num_resolvers; i ++){
        if(pthread_join(resolver_ids[i], 0) == 0);
        else {
            fprintf(stderr,"COULDNT JOIN REQUESTER THREAD: %s\n", strerror( errno ));
        }
    }
    

    //close all files
    fclose(write_requester_file);
    fclose(write_resolver_file);
    for (int i =0; i < num_files; i++){
        fclose(incoming_data_files[i]);
    }


    //free all malloc'd items
    free(resolver_ids);
    free(requester_ids);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        free(imp_shared_buffer[i]);
    }
    free(imp_shared_buffer);
    free(shared_buffer_struct);
    free(incoming_data_files);
    

    // gettimeofday function gets time and puts this in struct endTime 
    // startTime.tv_sec gets in seconds, startTime.
    gettimeofday(&endTime, NULL);

    //calculate in seconds the runtime
    double totaltime = (endTime.tv_sec - startTime.tv_sec) + ((endTime.tv_usec - startTime.tv_usec)/1000000.0);

    printf("./multi-lookup: total time is %f seconds\n", totaltime);

    //Check access to performance.txt to make sure valid
    if (access("performance.txt", R_OK|W_OK  ) == -1){
        fprintf(stderr, "ERROR, performance.txt doesn't exist or doesn't have read and write permissions");
        exit(1);
    }

    //open file for writing performance to
    FILE *results = fopen("performance.txt","a");

    //open requester log for reading
    FILE *read_requester_file = fopen(requester_log, "r");

    //call function to print results to performance.txt
    print_results(totaltime, num_requesters, num_resolvers, read_requester_file, results);

    //close files
    fclose(results);
    fclose(read_requester_file); 

    exit(0);

    
}


//Print the results of the program to output_file
//time -> time taken for program
//num_requesters -> number of requester threads
//num_resolvers -> number of resolver threads
//input_file -> requester_log that we will read thread info from
//output_file -> print results of program
void print_results(double time, int num_requesters, int num_resolvers, FILE *input_file, FILE *output_file){

    //create strings
    char output_string[64] = "";
    char num_requesters_str[64] = "";
    char num_resolvers_str[64] = "";
    char time_str[64] = "";

    //number of requesters
    sprintf(num_requesters_str, "Number of requester threads is %d\n", num_requesters);
    fputs(num_requesters_str, output_file);

    //number of resolvers
    sprintf(num_resolvers_str, "Number of resolver threads is %d\n", num_resolvers);
    fputs(num_resolvers_str, output_file);

    //thread info
    while (fgets(output_string, 64, input_file) != NULL){
        fputs(output_string, output_file);
    }

    //time taken to run program
    sprintf(time_str, "./multi-lookup: total time is %f seconds\n", time);
    fputs(time_str, output_file);

}


//function to put domains in buffer
//inputfile -> file with domain names
//buffer_array -> the shared buffer between threads
//buffer_struct -> variables that all threads access
int service_file(FILE *inputfile, char **buffer_array, Shared_Array *buffer_struct){
    //allocate memory for the hostname to be processed
    char req_hostname[MAX_NAME_LENGTH] = "";

    //keeps track of whether thread actually serviced file
    int serviced_file = 0;
    
    while (fgets(req_hostname, MAX_NAME_LENGTH, inputfile) != NULL){

        //make sure hostname is terminated by null charachter
        if ( strlen(req_hostname) <= MAX_NAME_LENGTH - 1 && req_hostname[strlen(req_hostname) - 1] == '\n'){
            req_hostname[strlen(req_hostname) - 1] = '\0';
        }

        //acquire lock
        pthread_mutex_lock(&buffer_struct->buffer_lock);

        //wait if the buffer is full
        while (buffer_struct -> index == ARRAY_SIZE){

            pthread_cond_wait(&buffer_struct->buffer_full, &buffer_struct->buffer_lock);

        }

        //set buffer[index] to hostname
        strcpy(buffer_array[buffer_struct -> index], req_hostname);

        //increase index of buffer
        buffer_struct -> index++;

        //signal resolver that buffer is not empty
        if (buffer_struct -> index != 0) {
        pthread_cond_broadcast(&buffer_struct -> buffer_empty);
        }

        //set serviced_file to 1
        serviced_file = 1;

        //release lock
        pthread_mutex_unlock(&buffer_struct -> buffer_lock);
        usleep(10);
    }

    return serviced_file;

}


//calls service_file function for each data file provided
//input -> pointer to requester arguments struct that holds variables all threads need to access
void *requester(void *input){

    //create struct req_args to take input parameters
    struct RequesterThreadArgs *req_args = (struct RequesterThreadArgs *) input;

    //parameters to pass into service file function
    FILE **input_files = req_args -> datafiles;
    Shared_Array *buffer_struct = req_args -> buffer_arg;
    FILE *output_file = req_args -> requester_log_file;
    char **buffer_array = req_args ->shared_buffer;
    int service_count = 0;

    //while loop that assigns requester threads one by one to each data file provided
    while(buffer_struct -> num_data_files > 0){
    //acquire lock
    pthread_mutex_lock(&buffer_struct->requester_file_lock);

    //checks if there are any data files left to be processed
    if (buffer_struct -> num_data_files > 0){
        if (service_file(input_files[buffer_struct -> num_data_files -1], buffer_array, buffer_struct)){

            //decrement number of data files to be processed
            buffer_struct -> num_data_files = buffer_struct -> num_data_files -1; 

            //increase the amount of data files that have been serviced by thread
            service_count++;
        }
    
    }
    //release lock
    pthread_mutex_unlock(&buffer_struct->requester_file_lock);

    //use this to try and prevent starvation by threads 
	usleep(100);
    }

    //print to requester_log the work that each thread did
    char str[64]="";
    sprintf(str, "Thread %lu serviced %d files\n", pthread_self(), service_count);
    fputs(str, output_file);

    //update requesters working
    buffer_struct -> requesters_working--; 

    return 0;
    
}


//takes domains out of buffer and does dns lookup then puts results into resolver_log.txt
//input -> pointer to requester arguments struct that holds variables all threads need to access
void *resolver(void *input){
    
    //necessary parameters that are shared between threads
    struct ResolverThreadArgs *res_args = (struct ResolverThreadArgs *) input;
    Shared_Array *buffer_struct = res_args -> buffer_arg;
    FILE *outputfile = res_args -> datafile;
    char **buffer_array = res_args ->shared_buffer;
    pthread_mutex_t write_file_lock = res_args -> output_file_lock;
    char res_hostname[MAX_NAME_LENGTH] = "";

    while(1){

        if ((buffer_struct ->requesters_working == 0) && (buffer_struct -> index == 0)){
            break;
        }

        //acquire lock
        pthread_mutex_lock(&buffer_struct->buffer_lock);

        //wait if buffer is empty
        while(buffer_struct -> index == 0){
            pthread_cond_wait(&buffer_struct->buffer_empty, &buffer_struct->buffer_lock);    
        }

        //set hostname to last item in buffer
        strcpy(res_hostname, buffer_array[buffer_struct -> index - 1]);

        //set buffer at index to empty after hostname is read
        memset(buffer_array[buffer_struct-> index - 1],0,MAX_NAME_LENGTH); 

        //decrease index of buffer
        buffer_struct -> index--;

        //signal resolver that buffer is not full
        if (buffer_struct -> index < ARRAY_SIZE) {
         pthread_cond_broadcast(&buffer_struct -> buffer_full);
        }

        //unlock the buffer
        pthread_mutex_unlock(&buffer_struct -> buffer_lock);

        //print to resolver_log hostname and ip
        char ip[MAX_IP_LENGTH]="";
        if ((dnslookup(res_hostname, ip, MAX_IP_LENGTH) == UTIL_SUCCESS)) {
            char output[MAX_IP_LENGTH+MAX_NAME_LENGTH+3]="";
            strcpy(output, res_hostname); 
            strcat(output, ",");
            strcat(output, ip);
            strcat(output, "\n");
            pthread_mutex_lock(&write_file_lock);
            fputs(output, outputfile);
            pthread_mutex_unlock(&write_file_lock);
            memset(ip, 0, MAX_IP_LENGTH);
        }
        //print to resolver_log hostname and print to console bogus hostname
        else {
            fprintf(stderr, "BOGUS HOSTNAME\n");
            char output[MAX_NAME_LENGTH+3]="";
            strcpy(output, res_hostname); 
            strcat(output, ",");
            strcat(output, "\n");
            pthread_mutex_lock(&write_file_lock);
            fputs(output, outputfile);
            pthread_mutex_unlock(&write_file_lock);
            memset(ip, 0, MAX_IP_LENGTH);
        }
        
           
    }
    return 0;

}
