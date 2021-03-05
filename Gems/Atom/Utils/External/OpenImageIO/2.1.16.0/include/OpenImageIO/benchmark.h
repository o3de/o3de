// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

#pragma once

#include <iostream>
#include <vector>

#include <OpenImageIO/function_view.h>
#include <OpenImageIO/string_view.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/timer.h>


OIIO_NAMESPACE_BEGIN

/// DoNotOptimize(val) is a helper function for timing benchmarks that fools
/// the compiler into thinking the the location 'val' is used and will not
/// optimize it away.  For benchmarks only, do not use in production code!
/// May not work on all platforms. References:
/// * Chandler Carruth's CppCon 2015 talk
/// * Folly https://github.com/facebook/folly/blob/master/folly/Benchmark.h
/// * Google Benchmark https://github.com/google/benchmark/blob/master/include/benchmark/benchmark_api.h

template <class T>
OIIO_FORCEINLINE T const& DoNotOptimize (T const &val);


/// clobber_all_memory() is a helper function for timing benchmarks that
/// fools the compiler into thinking that potentially any part of memory
/// has been modified, and thus serves as a barrier where the optimizer
/// won't assume anything about the state of memory preceding it.

OIIO_FORCEINLINE void clobber_all_memory();



/// A call to clobber(p) fools the compiler into thinking that p (or *p, for
/// the pointer version) might potentially have its memory altered. The
/// implementation actually does nothing, but it's in another module, so the
/// compiler won't know this and will be conservative about any assupmtions
/// of what's in p. This is helpful for benchmarking, to help erase any
/// preconceptions the optimizer has about what might be in a variable.

void OIIO_API clobber (void* p);
OIIO_FORCEINLINE void clobber (const void* p) { clobber ((void*)p); }

template<typename T>
OIIO_FORCEINLINE T& clobber (T& p) { clobber(&p); return p; }

// Multi-argument clobber, added in OIIO 2.2.2
template<typename T, typename ...Ts>
OIIO_FORCEINLINE void clobber (T& p, Ts&... ps)
{
    clobber(&p);
    if (sizeof...(Ts) > 0)
        clobber(ps...);
}




/// Benchmarker is a class to assist with "micro-benchmarking".
/// The goal is to discern how long it takes to run a snippet of code
/// (function, lambda, etc). The code will be run in some number of trials,
/// each consisting of many iterations, yielding statistics about the run
/// time of the code.
///
/// Tne number of trials is user-selectable, with a reasonable default of 10
/// trials. The number of iterations per trial may be set explicitly, but
/// the default is to automatically compute a reasonable number of
/// iterations based on their timing. For most use cases, it's fire and
/// forget.
///
/// Generally, the most and least expesive trials will be discarded (all
/// sorts of things can happen to give you a few spurious results) and then
/// the remainder of trials will be used to compute the average, standard
/// deviation, range, and median value, in ns per iteration as well as
/// millions of executions per second. The default behavior it just to echo
/// the relevant statistics to the console.
///
/// The basic use illustrated by this example in which we try to assess
/// the difference in speed between acos() and fast_acos():
///
///     Benchmarker bench;
///     float val = 0.5f;
///     clobber (val);  // Scrub compiler's knowledge of the value
///     bench ("acos", [&](){ DoNotOptimize(std::acos(val)); });
///     bench ("fast_acos", [&](){  // alternate indentation style
///         DoNotOptimize(OIIO::fast_acos(val));
///     });
///
/// Which produces output like this:
///   acos      :    4.3 ns,  230.5 M/s (10x2097152, sdev=0.4ns rng=31.2%, med=4.6)
///   fast_acos :    3.4 ns,  291.2 M/s (10x2097152, sdev=0.4ns rng=33.0%, med=3.4)
///
/// Some important details:
///
/// After declaring the Benchmarker, a number of options can be set: number
/// of trials to run, iterations per trial (0 means automatic detection),
/// verbosity, whether (or how many) outliers to exclude. You can chain them
/// together if you want:
///     bench.iterations(10000).trials(10);
///
/// It can be VERY hard to get valid benchmarks without the compiler messing
/// up your results. Some tips:
///
/// * Code that is too fast will not be reliable. Anything that appears
///   to take less than 1 ns actually prints "unreliable" instead of full
///   stats, figuring that it is likely that it has been inadvertently
///   optimized away.
///
/// * Use the DoNotOptimize() call on any final results computed by your
///   benchmarked code, or else the compiler is likely to remove the code
///   that leads to any values it thinks will never be used.
///
/// * Beware of the compiler constant folding operations in your code --
///   do not pass constants unless you want to benchmark its performance on
///   known constants, and it is probably smart to ensure that all variables
///   acccessed by your code should be passed to clobber() before running
///   the benchmark, to confuse the compiler into not assuming its value.

class OIIO_API Benchmarker {
public:
    Benchmarker() {}

    // Calling Benchmarker like a function (operator()) executes the
    // benchmark. This process runs func(args...), several trials, each
    // trial with many iterations. The value returned is the best estimate
    // of the average time per iteration that it takes to run func.
    template<typename FUNC, typename... ARGS>
    double operator()(string_view name, FUNC func, ARGS&&... args)
    {
        m_name = name;
        run(func, args...);
        if (verbose())
            std::cout << (*this) << std::endl;
        return avg();
    }

    // Return the average, sample standard deviation, median, and range
    // of per-iteration time.
    double avg() const { return m_avg; }
    double stddev() const { return m_stddev; }
    double range() const { return m_range; }
    double median() const { return m_median; }

    // Control the number of iterations per trial. The special value 0 means
    // to determine automatically a reasonable number of iterations. That is
    // also the default behavior.
    Benchmarker& iterations(size_t val)
    {
        m_user_iterations = val;
        return *this;
    }
    size_t iterations() const { return m_iterations; }

    // Control the number of trials to perform.
    Benchmarker& trials(size_t val)
    {
        m_trials = val;
        return *this;
    }
    size_t trials() const { return m_trials; }

    // Control the number of values of work that each iteration represents.
    // Usually you will leave this at the default of 1, but for some cases,
    // it may be helpful. An example of where you might use this is if you
    // are benchmarking SIMD operations. A scalar sqrt and an SIMD sqrt may
    // run in the same amount of time, but the SIMD version is operating on
    // 4 (or 8, etc.) times as many values. You can use the 'work' size to
    // make the calls report Mvals/s, showing more accurately than the SIMD
    // call is faster than the scalar call.
    Benchmarker& work(size_t val)
    {
        m_work = val;
        return *this;
    }
    size_t work() const { return m_work; }

    // Control the exclusion of outliers. This number (default 1) of fastest
    // and slowest trials will be excluded from the statistics, to remove
    // the effects of spurious things happening on the system. Setting
    // outliers to 0 will compute statistics on all trials, without any
    // outlier exclusion.
    Benchmarker& exclude_outliers(int e)
    {
        m_exclude_outliers = e;
        return *this;
    }
    int exclude_outliers() const { return m_exclude_outliers; }

    // Control the verbosity of the printing for each benchmark. The default
    // is 1, which prints basic statistics. Verbosity 0 is silent and leaves
    // it up to the app to retrieve results.
    Benchmarker& verbose(int v)
    {
        m_verbose = v;
        return *this;
    }
    int verbose() const { return m_verbose; }

    // Control indentation in the printout -- this number of spaces will
    // be printed before the statistics.
    Benchmarker& indent(int spaces)
    {
        m_indent = spaces;
        return *this;
    }
    int indent() const { return m_indent; }

    // Choices of unit to report results.
    enum class Unit : int { autounit, ns, us, ms, s };

    // Control the units for reporting results. By default, an appropriate
    // unit will be chosen for nice printing of each benchmark individually.
    // But the user may also wish to request specific units like ns or ux in
    // order to ensure that all benchmark resutls are using the same units.
    Benchmarker& units(Unit s)
    {
        m_units = s;
        return *this;
    }
    Unit units() const { return m_units; }

    const std::string& name() const { return m_name; }

private:
    size_t m_iterations      = 0;
    size_t m_user_iterations = 0;
    size_t m_trials          = 10;
    size_t m_work            = 1;
    std::string m_name;
    std::vector<double> m_times;  // times for each trial
    double m_avg;                 // average time per iteration
    double m_stddev;              // standard deviation per iteration
    double m_range;               // range per iteration
    double m_median;              // median per-iteration time
    int m_exclude_outliers = 1;
    int m_verbose          = 1;
    int m_indent           = 0;
    Unit m_units           = Unit::autounit;

    template<typename FUNC, typename... ARGS>
    double run(FUNC func, ARGS&&... args)
    {
        if (m_user_iterations)
            m_iterations = m_user_iterations;
        else
            m_iterations = determine_iterations(func, args...);
        m_times.resize(m_trials);

        double overhead = iteration_overhead() * iterations();
        for (auto& t : m_times)
            t = std::max(0.0, do_trial(m_iterations, func, args...) - overhead);
        compute_stats();
        return avg();
    }

    template<typename FUNC, typename... ARGS>
    size_t determine_iterations(FUNC func, ARGS&&... args)
    {
        // We're shooting for a trial around 1/100s
        const double target_time = 0.01;
        size_t i = 1;
        while (1) {
            double t = do_trial (i, func, args...);
            // std::cout << "Trying " << i << " iters = " << t << "\n";
            if (t > target_time * 1.5 && i > 2)
                return i / 2;
            if (t > target_time * 0.75 || i > (size_t(1) << 30))
                return i;
            if (t < target_time / 16)
                i *= 8;
            else
                i *= 2;
        }
    }

    template<typename FUNC, typename... ARGS>
    double do_trial(size_t iterations, FUNC func, ARGS&&... args)
    {
        Timer timer;
        while (iterations--) {
            clobber_all_memory();
            func(args...);
        }
        return timer();
    }

    void compute_stats() { compute_stats(m_times, m_iterations); }
    void compute_stats(std::vector<double>& times, size_t iterations);
    double iteration_overhead();

    friend OIIO_API std::ostream& operator<<(std::ostream& out,
                                             const Benchmarker& bench);
};



/// Helper template that runs a function (or functor) n times, using a
/// Timer to benchmark the results, and returning the fastest trial.  If
/// 'range' is non-NULL, the range (max-min) of the various time trials
/// will be stored there.
///
/// DEPRECATED(1.8): This may be considered obsolete, probably the
/// Benchmarker class is a better solution.
template<typename FUNC>
double
time_trial(FUNC func, int ntrials = 1, int nrepeats = 1, double* range = NULL)
{
    double mintime = 1.0e30, maxtime = 0.0;
    while (ntrials-- > 0) {
        Timer timer;
        for (int i = 0; i < nrepeats; ++i) {
            // Be sure that the repeated calls to func aren't optimzed away:
            clobber_all_memory();
            func();
        }
        double t = timer();
        if (t < mintime)
            mintime = t;
        if (t > maxtime)
            maxtime = t;
    }
    if (range)
        *range = maxtime - mintime;
    return mintime;
}

/// Version without repeats.
template<typename FUNC>
double
time_trial(FUNC func, int ntrials, double* range)
{
    return time_trial(func, ntrials, 1, range);
}



// Benchmarking helper function: Time a function with various thread counts.
// Inputs:
//     task(int iterations) : The function to run (which understands an
//                            iteration count or work load).
//     pretask() : Code to run before the task threads start.
//     posttask() : Code to run after the task threads complete.
//     out : Stream to print results (or NULL to not print anything).
//     maxthreads : Don't do any trials greater than this thread count,
//                      even if it's in the threadcounts[].
//     total_iterations : Total amount of work to do. The func() will be
//                      called with total_iterations/nthreads, so that the
//                      total work for all threads stays close to constant.
//     ntrials : The number of runs for each thread count (more will take
//                      longer, but be more accurate timing). The best case
//                      run is the one that will be reported.
//     threadcounts[] : An span<int> giving the set of thread counts
//                      to try.
// Return value:
//     A vector<double> containing the best time (of the trials) for each
//     thread count. This can be discarded.
OIIO_API std::vector<double>
timed_thread_wedge (function_view<void(int)> task,
                    function_view<void()> pretask,
                    function_view<void()> posttask,
                    std::ostream *out,
                    int maxthreads,
                    int total_iterations, int ntrials,
                    cspan<int> threadcounts = {1,2,4,8,12,16,24,32,48,64,128});

// Simplified timed_thread_wedge without pre- and post-tasks, using
// std::out for output, with a default set of thread counts, and not needing
// to return the vector of times.
OIIO_API void
timed_thread_wedge (function_view<void(int)> task,
                    int maxthreads, int total_iterations, int ntrials,
                    cspan<int> threadcounts = {1,2,4,8,12,16,24,32,48,64,128});




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Implementation details...
//


namespace pvt {
void OIIO_API use_char_ptr (char const volatile *);
}


#if ((OIIO_GNUC_VERSION && NDEBUG) || OIIO_CLANG_VERSION >= 30500 || OIIO_APPLE_CLANG_VERSION >= 70000 || defined(__INTEL_COMPILER)) && (defined(__x86_64__) || defined(__i386__))

// Major non-MS compilers on x86/x86_64: use asm trick to indicate that
// the value is needed.
template <class T>
OIIO_FORCEINLINE T const&
DoNotOptimize (T const &val) {
#if defined(__clang__)
    // asm volatile("" : "+rm" (const_cast<T&>(val)));
    // Clang doesn't like the 'X' constraint on `val` and certain GCC versions
    // don't like the 'g' constraint. Attempt to placate them both.
    asm volatile("" : : "g"(val) : "memory");
#else
    asm volatile("" : : "i,r,m"(val) : "memory");
#endif
    return val;
}

#elif _MSC_VER

// Microsoft of course has its own way of turning off optimizations.
#pragma optimize("", off)
template <class T>
OIIO_FORCEINLINE T const & DoNotOptimize (T const &val) {
    pvt::use_char_ptr(&reinterpret_cast<char const volatile&>(val));
    _ReadWriteBarrier ();
    return val;
}
#pragma optimize("", on)

#elif __has_attribute(__optnone__)

// If __optnone__ attribute is available: make a null function with no
// optimization, that's all we need.
template <class T>
inline T const & __attribute__((__optnone__))
DoNotOptimize (T const &val) {
    return val;
}

#else

// Otherwise, it won't work, just make a stub.
template <class T>
OIIO_FORCEINLINE T const & DoNotOptimize (T const &val) {
    pvt::use_char_ptr(&reinterpret_cast<char const volatile&>(val));
    return val;
}

#endif



#if ((OIIO_GNUC_VERSION && NDEBUG) || OIIO_CLANG_VERSION >= 30500 || OIIO_APPLE_CLANG_VERSION >= 70000 || defined(__INTEL_COMPILER)) && (defined(__x86_64__) || defined(__i386__))

// Special trick for x86/x86_64 and gcc-like compilers
OIIO_FORCEINLINE void clobber_all_memory() {
    asm volatile ("" : : : "memory");
}

#elif _MSC_VER

OIIO_FORCEINLINE void clobber_all_memory() {
    _ReadWriteBarrier ();
}

#else

// No fallback for other CPUs or compilers. Suggestions?
OIIO_FORCEINLINE void clobber_all_memory() { }

#endif



OIIO_NAMESPACE_END
