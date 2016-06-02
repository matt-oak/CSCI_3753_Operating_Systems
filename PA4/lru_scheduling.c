/*
 * File: pager-lru.c
 * Author:       Matthew Oakley
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2016/04/03
 * Description:
 * 	This file contains an lru pageit
 *      implementation.
 */

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for an LRU pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];

    /* Local vars */
    int proctmp;
    int pagetmp;
    int proc;
    int page;

    /* initialize static vars on first run */
    if(!initialized){
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
		timestamps[proctmp][pagetmp] = 0; 
	    }
	}
	initialized = 1;
	}
    
    /* TODO: Implement LRU Paging */
    for (proc = 0; proc < MAXPROCESSES; proc++){
		
		/* Check and see if the process is active */
		if (q[proc].active) {
			
			/* Get the page that we are to work with */
			page = q[proc].pc/PAGESIZE;
			
			/* If the page isn't in memory, continue */
			if (!q[proc].pages[page]) {
				
				/* Try and page the page in */
				if (!pagein(proc, page)) {
					
					/* If it doesn't get paged in when we need to find something to replace */
					int minTick = -1;
					int i;
					int x = 0;
					
					/* Find the page with the lowest timestamp */
					for (i = 0; i < q[proc].npages; i++) {
						int threshold = tick - timestamps[proc][i];
						if ((minTick < threshold) && q[proc].pages[i]){
							minTick = threshold;
							x = i;
						}
					}
					
					/* Evict that page and update the timestamps matrix as well as the tick */
					pageout(proc, x);
					timestamps[proc][page] = tick;
					tick++;
				}
			}
		}
	}
} 
