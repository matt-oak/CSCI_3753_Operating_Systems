/*
 * File: multi-lookup.c
 * Author: Matthew Oakley
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 02/25/2015 
 */

/* Include Statements */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "util.c"
#include "queue.c"

/* Define Global Variables */
#define USAGE "<inputFilePath> <outputFilePath>"
#define MAX_NAME_LENGTH 1025
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_IP_LENGTH INET6_ADDRSTRLEN
#define MAX_INPUT_FILE_LENGTH 15
queue request_queue;
FILE* outputFilePointer;

/* Mutex Locks */
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t outputFileLock = PTHREAD_MUTEX_INITIALIZER;

/* Function for Requester Threads */
void* requesterFunction(void* inputFileString){
	
	/* Local Variables */
    FILE* inputFileName = NULL;
	char* URL;
	int URLsRequested = 0;
	
	/* Open input file and check for errors */
	inputFileName = fopen((char *)inputFileString, "r");
	if(!inputFileName) {
		fprintf(stderr, "Error Opening Input File: %s\n", (char*)inputFileName);
	}

	/* Malloc for the first URL string */
	URL = (char*)malloc((MAX_NAME_LENGTH)*sizeof(char));
	
	/* Read individual file and push URL strings to queue */
	while(fscanf(inputFileName, INPUTFS, URL) > 0){
		
		/* Provide mutual exclusion to queue */
		pthread_mutex_lock(&queueLock);
		
		/* If the queue is full, unlock to allow other threads to process information,
		 * sleep for a random amount of time between 1 and 100 microseconds, 
		 * and relock the queue                                                         
		 */
		if(queue_is_full(&request_queue)) {
			pthread_mutex_unlock(&queueLock);
			usleep(rand()%100);
			pthread_mutex_lock(&queueLock);
		}
		
		/* Otherwise, if the queue is not full, push URL string to the queue */
		queue_push(&request_queue, URL);
		URLsRequested += 1;
		
		/* Unlock queue to allow other threads to input to queue */
		pthread_mutex_unlock(&queueLock);
		
		/* Re-Malloc for the next URL string */
		URL = (char*)malloc((MAX_NAME_LENGTH)*sizeof(char));
	 }
	
	/* Close the input file, free the last malloc'd URL space, and exit */
	fclose(inputFileName);
	free(URL);
    printf("Requester thread added %d hostnames to the queue.\n", URLsRequested);
    return NULL;
}

/* Function for Resolver Threads */
void* resolverFunction(void* outputFileString) {
	
	/* Local Variables */
	char *URL;
	char IPString[INET_ADDRSTRLEN];
	int stopper = 1;
	int URLsResolved = 0;
	
	/* While there are more URLs on the queue... */
	while(stopper){
		
		/* Provide mutual exclusion to the queue so only 1 resolver thread can pop something off*/
		pthread_mutex_lock(&queueLock);
		URL = (char*)queue_pop(&request_queue);
		
		/* If the URL is NULL, this means we've reached the end of the queue and should unlock */
		if (URL == NULL){
			pthread_mutex_unlock(&queueLock);
		}
		
		/* Otherwise, if we pop off an actual URL, we need to go look up the corresponding IP address */
		else{
			pthread_mutex_unlock(&queueLock);
			
			/* If we receive an error after trying to look up a corresponding IP address,
			 * print to the error log and print an empty string for the IP address     
			 */
			if(dnslookup(URL, IPString, sizeof(IPString)) == UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", URL);
				pthread_mutex_lock(&outputFileLock);
				fprintf(outputFileString, "%s,%s\n", URL, "");
				pthread_mutex_unlock(&outputFileLock);
				URLsResolved += 1;
			}
			
			/* Otherwise, if we succesfully look up the corresponding IP address, provide
			 * mutual exclusion and write the "URL,IPAdress" to the output file         
			 */
			else{
				pthread_mutex_lock(&outputFileLock);
				fprintf(outputFileString, "%s,%s\n", URL, IPString);
				pthread_mutex_unlock(&outputFileLock);
				URLsResolved += 1;
			}
			
			/* Check and see if the queue is empty. If it is, break out of the loop.
			 * Otherwise, loop again because there are more URLs to be resolved    
			 */
			free(URL);
			pthread_mutex_lock(&queueLock);
			if(queue_is_empty(&request_queue)){
				stopper = 0;
			}
			pthread_mutex_unlock(&queueLock);
		}
	}
	printf("Resolver thread added %d hostnames to the queue.\n", URLsResolved);
	return NULL;
}

int main(int argc, char* argv[]){
	
	/* Local Variables */
    outputFilePointer = NULL;
    int numInputFiles = argc - 2;
    int numCores = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t requesterThreads[numInputFiles];
    pthread_t resolverThreads[numCores];
	
	/* Open and Check Output File Path for Errors */
	outputFilePointer = fopen(argv[argc - 1], "w");
	if(!outputFilePointer){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }
    
    /* Initialize Request Queue w/ a Size of 1024 */
    queue_init(&request_queue, 1024);
    
    /* Thread + Counter Variables */
    int rc;
    long i;
    
    /* Spawn Requester Threads */
    for(i = 0; i < numInputFiles; i++){
		char* inputFile = argv[i + 1];
		rc = pthread_create(&(requesterThreads[i]), NULL, requesterFunction, inputFile);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
    }
    
    /* Spawn Resolver Threads */
    for(i = 0; i < numCores; i++){
		rc = pthread_create(&(resolverThreads[i]), NULL, resolverFunction, outputFilePointer);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}
    
    /* Wait for Requester Threads to Finish */
    for(i = 0; i < numInputFiles; i++){
		pthread_join(requesterThreads[i], NULL);
    }
    
    /* Wait for Resolver Threads to Finish */
    for(i = 0; i < numCores; i++){
		pthread_join(resolverThreads[i], NULL);
    }
    
    /* Cleanup */
    fclose(outputFilePointer);
    queue_cleanup(&request_queue);
    pthread_mutex_destroy(&queueLock);
    pthread_mutex_destroy(&outputFileLock);
    
	return 0;
}
