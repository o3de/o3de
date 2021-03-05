// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#include "timer.h"

void InitTimer(Timer_t* psTimer)
{
#if defined(_WIN32)
    QueryPerformanceFrequency(&psTimer->frequency);
#endif
}

void ResetTimer(Timer_t* psTimer)
{
#if defined(_WIN32)
    QueryPerformanceCounter(&psTimer->startCount);
#else
    gettimeofday(&psTimer->startCount, 0);
#endif
}

/* Returns time in micro seconds */
double ReadTimer(Timer_t* psTimer)
{
    double startTimeInMicroSec, endTimeInMicroSec;

#if defined(_WIN32)
    const double freq = (1000000.0 / psTimer->frequency.QuadPart);
    QueryPerformanceCounter(&psTimer->endCount);
    startTimeInMicroSec = psTimer->startCount.QuadPart * freq;
    endTimeInMicroSec = psTimer->endCount.QuadPart * freq;
#else
    gettimeofday(&psTimer->endCount, 0);
    startTimeInMicroSec = (psTimer->startCount.tv_sec * 1000000.0) + psTimer->startCount.tv_usec;
    endTimeInMicroSec = (psTimer->endCount.tv_sec * 1000000.0) + psTimer->endCount.tv_usec;
#endif

    return endTimeInMicroSec - startTimeInMicroSec;
}

