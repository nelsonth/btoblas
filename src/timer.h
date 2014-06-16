// Three timer options.. select 1 only
#ifndef TIMERUS_H
#define TIMERUS_H


#include <sys/time.h>
#include <sys/types.h>
// gettimeofday 4us at best accuracy
double cal_sec(struct timeval *start, struct timeval *end) {
    double sec;
	double s_usec = start->tv_usec;
	double e_usec = end->tv_usec;
	double s_sec = start->tv_sec;
	double e_sec = end->tv_sec;
   
    if (s_usec > e_usec) {
            e_usec += 1000000;
            e_sec -= 1;
    }
    sec = e_sec - s_sec;
    sec += (double)(e_usec - s_usec)/1000000.0;

    return sec;
}
#define START_TIMER gettimeofday(&start,NULL)
#define GET_TIMER gettimeofday(&end,NULL)
#define INIT_TIMER struct timeval start, end
#define DIFF_TIMER cal_sec(&start, &end)


/*
// THIS IS NOT WORKING?? //
// time stamp counter (rdtsc) < 4us accuracy
// do not use without verifying constant_tsc flag is set in /proc/cpuinfo
// do not use without verifying frequency of machine
double getts() {
	long long val;
	__asm__ __volatile__("rdtsc":"=A"(val));
	return (double) val;
}

#define FREQ  2666000000.0
//#define FREQ  2400000000.0
#define START_TIMER start = getts()
#define GET_TIMER end = getts()
#define INIT_TIMER long long start, end
#define DIFF_TIMER (double)(end - start) / FREQ
*/

/*
// boost timer .01s at best accuracy
#include <boost/timer.hpp>
#define START_TIMER t.restart()
#define GET_TIMER ;
#define INIT_TIMER boost::timer t
#define DIFF_TIMER t.elapsed() 
*/

#endif
