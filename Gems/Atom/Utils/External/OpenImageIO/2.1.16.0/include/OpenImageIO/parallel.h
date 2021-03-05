// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

#pragma once

#include <algorithm>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <OpenImageIO/strutil.h>
#include <OpenImageIO/thread.h>


OIIO_NAMESPACE_BEGIN

/// Split strategies
enum SplitDir { Split_X, Split_Y, Split_Z, Split_Biggest, Split_Tile };


/// Encapsulation of options that control parallel_image().
class parallel_options {
public:
    parallel_options(int maxthreads = 0, SplitDir splitdir = Split_Y,
                     size_t minitems = 16384)
        : maxthreads(maxthreads)
        , splitdir(splitdir)
        , minitems(minitems)
    {
    }
    parallel_options(string_view name, int maxthreads = 0,
                     SplitDir splitdir = Split_Y, size_t minitems = 16384)
        : maxthreads(maxthreads)
        , splitdir(splitdir)
        , minitems(minitems)
        , name(name)
    {
    }

    // Fix up all the TBD parameters:
    // * If no pool was specified, use the default pool.
    // * If no max thread count was specified, use the pool size.
    // * If the calling thread is itself in the pool and the recursive flag
    //   was not turned on, just use one thread.
    void resolve()
    {
        if (pool == nullptr)
            pool = default_thread_pool();
        if (maxthreads <= 0)
            maxthreads = pool->size() + 1;  // pool size + caller
        if (!recursive && pool->is_worker())
            maxthreads = 1;
    }

    bool singlethread() const { return maxthreads == 1; }

    int maxthreads    = 0;        // Max threads (0 = use all)
    SplitDir splitdir = Split_Y;  // Primary split direction
    bool recursive    = false;    // Allow thread pool recursion
    size_t minitems   = 16384;    // Min items per task
    thread_pool* pool = nullptr;  // If non-NULL, custom thread pool
    string_view name;             // For debugging
};



/// Parallel "for" loop, chunked: for a task that takes an int thread ID
/// followed by an int64_t [begin,end) range, break it into non-overlapping
/// sections that run in parallel using the default thread pool:
///
///    task (threadid, start, start+chunksize);
///    task (threadid, start+chunksize, start+2*chunksize);
///    ...
///    task (threadid, start+n*chunksize, end);
///
/// and wait for them all to complete.
///
/// If chunksize is 0, a chunksize will be chosen to divide the range into
/// a number of chunks equal to the twice number of threads in the queue.
/// (We do this to offer better load balancing than if we used exactly the
/// thread count.)
///
/// Note that the thread_id may be -1, indicating that it's being executed
/// by the calling thread itself, or perhaps some other helpful thread that
/// is stealing work from the pool.
OIIO_API void
parallel_for_chunked(int64_t start, int64_t end, int64_t chunksize,
                     std::function<void(int id, int64_t b, int64_t e)>&& task,
                     parallel_options opt = parallel_options(0, Split_Y, 1));
// Implementation is in thread.cpp



/// Parallel "for" loop, chunked: for a task that takes a [begin,end) range
/// (but not a thread ID).
inline void
parallel_for_chunked(int64_t start, int64_t end, int64_t chunksize,
                     std::function<void(int64_t, int64_t)>&& task,
                     parallel_options opt = parallel_options(0, Split_Y, 1))
{
    auto wrapper = [&](int id, int64_t b, int64_t e) { task(b, e); };
    parallel_for_chunked(start, end, chunksize, wrapper, opt);
}



/// Parallel "for" loop, for a task that takes a single int64_t index, run
/// it on all indices on the range [begin,end):
///
///    task (begin);
///    task (begin+1);
///    ...
///    task (end-1);
///
/// Using the default thread pool, spawn parallel jobs. Conceptually, it
/// behaves as if each index gets called separately, but actually each
/// thread will iterate over some chunk of adjacent indices (to aid data
/// coherence and minimuize the amount of thread queue diddling). The chunk
/// size is chosen automatically.
inline void
parallel_for (int64_t start, int64_t end,
              std::function<void(int64_t index)>&& task,
              parallel_options opt=parallel_options(0,Split_Y,1))
{
    parallel_for_chunked (start, end, 0, [&task](int id, int64_t i, int64_t e) {
        for ( ; i < e; ++i)
            task (i);
    }, opt);
}


/// parallel_for, for a task that takes an int threadid and an int64_t
/// index, running all of:
///    task (id, begin);
///    task (id, begin+1);
///    ...
///    task (id, end-1);
inline void
parallel_for (int64_t start, int64_t end,
              std::function<void(int id, int64_t index)>&& task,
              parallel_options opt=parallel_options(0,Split_Y,1))
{
    parallel_for_chunked (start, end, 0, [&task](int id, int64_t i, int64_t e) {
        for ( ; i < e; ++i)
            task (id, i);
    }, opt);
}



/// parallel_for_each, semantically is like std::for_each(), but each
/// iteration is a separate job for the default thread pool.
template<class InputIt, class UnaryFunction>
UnaryFunction
parallel_for_each (InputIt first, InputIt last, UnaryFunction f,
                   parallel_options opt=parallel_options(0,Split_Y,1))
{
    opt.resolve ();
    task_set ts (opt.pool);
    for ( ; first != last; ++first) {
        if (opt.singlethread() || opt.pool->very_busy()) {
            // If we are using just one thread, or if the pool is already
            // oversubscribed, do it ourselves and avoid messing with the
            // queue or handing off between threads.
            f (*first);
        } else {
            ts.push (opt.pool->push ([&](int id){ f(*first); }));
        }
    }
    return std::move(f);
}



/// Parallel "for" loop in 2D, chunked: for a task that takes an int thread
/// ID followed by begin, end, chunksize for each of x and y, subdivide that
/// run in parallel using the default thread pool.
///
///    task (threadid, xstart, xstart+xchunksize, );
///    task (threadid, start+chunksize, start+2*chunksize);
///    ...
///    task (threadid, start+n*chunksize, end);
///
/// and wait for them all to complete.
///
/// If chunksize is 0, a chunksize will be chosen to divide the range into
/// a number of chunks equal to the twice number of threads in the queue.
/// (We do this to offer better load balancing than if we used exactly the
/// thread count.)
OIIO_API void
parallel_for_chunked_2D (int64_t xstart, int64_t xend, int64_t xchunksize,
                         int64_t ystart, int64_t yend, int64_t ychunksize,
                         std::function<void(int id, int64_t, int64_t,
                                            int64_t, int64_t)>&& task,
                         parallel_options opt=0);
// Implementation is in thread.cpp



/// Parallel "for" loop, chunked: for a task that takes a 2D [begin,end)
/// range and chunk sizes.
inline void
parallel_for_chunked_2D (int64_t xstart, int64_t xend, int64_t xchunksize,
                         int64_t ystart, int64_t yend, int64_t ychunksize,
                         std::function<void(int64_t, int64_t,
                                            int64_t, int64_t)>&& task,
                         parallel_options opt=0)
{
    auto wrapper = [&](int id, int64_t xb, int64_t xe,
                       int64_t yb, int64_t ye) { task(xb,xe,yb,ye); };
    parallel_for_chunked_2D (xstart, xend, xchunksize,
                             ystart, yend, ychunksize, wrapper, opt);
}



/// parallel_for, for a task that takes an int threadid and int64_t x & y
/// indices, running all of:
///    task (id, xstart, ystart);
///    ...
///    task (id, xend-1, ystart);
///    task (id, xstart, ystart+1);
///    task (id, xend-1, ystart+1);
///    ...
///    task (id, xend-1, yend-1);
inline void
parallel_for_2D (int64_t xstart, int64_t xend,
                 int64_t ystart, int64_t yend,
                 std::function<void(int id, int64_t i, int64_t j)>&& task,
                 parallel_options opt=0)
{
    parallel_for_chunked_2D (xstart, xend, 0, ystart, yend, 0,
            [&task](int id, int64_t xb, int64_t xe, int64_t yb, int64_t ye) {
        for (auto y = yb; y < ye; ++y)
            for (auto x = xb; x < xe; ++x)
                task (id, x, y);
    }, opt);
}



/// parallel_for, for a task that takes an int threadid and int64_t x & y
/// indices, running all of:
///    task (xstart, ystart);
///    ...
///    task (xend-1, ystart);
///    task (xstart, ystart+1);
///    task (xend-1, ystart+1);
///    ...
///    task (xend-1, yend-1);
inline void
parallel_for_2D (int64_t xstart, int64_t xend,
                 int64_t ystart, int64_t yend,
                 std::function<void(int64_t i, int64_t j)>&& task,
                 parallel_options opt=0)
{
    parallel_for_chunked_2D (xstart, xend, 0, ystart, yend, 0,
            [&task](int id, int64_t xb, int64_t xe, int64_t yb, int64_t ye) {
        for (auto y = yb; y < ye; ++y)
            for (auto x = xb; x < xe; ++x)
                task (x, y);
    }, opt);
}



// DEPRECATED(1.8): This version accidentally accepted chunksizes that
// weren't used. Preserve for a version to not break 3rd party apps.
OIIO_DEPRECATED("Use the version without chunk sizes (1.8)")
inline void
parallel_for_2D (int64_t xstart, int64_t xend, int64_t xchunksize,
                 int64_t ystart, int64_t yend, int64_t ychunksize,
                 std::function<void(int id, int64_t i, int64_t j)>&& task)
{
    parallel_for_2D (xstart, xend, ystart, yend,
                     std::forward<std::function<void(int,int64_t,int64_t)>>(task));
}

OIIO_NAMESPACE_END
