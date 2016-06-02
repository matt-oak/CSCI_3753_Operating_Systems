/*
 * File: pager-lru.c
 * Author:       Matthew Oakley
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2016/04/03
 * Description:
 * 	This file contains a predictive pageit
 *      implementation.
 */
 
//Collaborators: Loic Guegan, Chris Rhoda

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES]) { 

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int counts[MAXPROCESSES][MAXPROCPAGES];
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
			counts[proctmp][pagetmp] = 0;
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
				
				counts[proc][page] += 1;
				
				/* Check and see if the page we're trying to work with */
				/* is currently at the end with respect to the program counter */
				/* If it is, it'd be useless to page it in so page in */
				/* the next subsequent page */
				if ((q[proc].pc % PAGESIZE) == 0){
					pagein(proc, page + 1);
				}
				
				/* Try and page the page we found in */
				if (!pagein(proc, page)) {
				
					/* Vars */
					int maxVal = -1;
					int minTick = -1;
					int i;
					int j;
					int row;
					int col;
					int pageToRemove = 0;
					
					/* Find the page that's been paged in the most */
					for (i = 0; i < MAXPROCESSES; i++) {
						for (j = 0; j < MAXPROCPAGES; j++) {
							int val = counts[i][j];
							if (val > maxVal) {
								maxVal = val;
								row = i;
								col = j;
							}
						}
					}
					
					/* Find the page with the lowest timestamp */
					for (i = 0; i < q[proc].npages; i++) {
						int threshold = tick - timestamps[proc][i];
						if ((minTick < threshold) && q[proc].pages[i]){
							minTick = threshold;
							pageToRemove = i;
						}
					}
					
					/* Evict that page and update the timestamps matrix as well as the tick */
					pageout(proc, pageToRemove);
					
					/* If the page with the max counts value isn't in memory */
					/* that is the page we want to page in */
					if(!q[row].pages[col]){
						pagein(row, col);
					}
					
					/* Update the timestamps matrix and the tick */
					timestamps[proc][page] = tick;
					tick++;
				}
			}
		}
		
		/* Otherwise, if the process is not active, */
		/* take all of its pages out of memory */
		/* and set the counts column for that process to 0s */
		else {
			int i;
			for (i = 0; i < q[proc].npages; i++) {
				pageout(proc, i);
				counts[proc][i] = 0;
			}
			q[proc].npages = 0;
		}
	}
} 
