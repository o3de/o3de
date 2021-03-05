// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef TIMER_H
#define TIMER_H

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

typedef struct
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER startCount;
    LARGE_INTEGER endCount;
#else
    struct timeval startCount;
    struct timeval endCount;
#endif
} Timer_t;

void InitTimer(Timer_t* psTimer);
void ResetTimer(Timer_t* psTimer);
double ReadTimer(Timer_t* psTimer);

#endif
