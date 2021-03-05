// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

/////////////////////////////////////////////////////////////////////////
/// @file   thread.h
///
/// @brief  Wrappers and utilities for multithreading.
/////////////////////////////////////////////////////////////////////////


#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <OpenImageIO/atomic.h>
#include <OpenImageIO/dassert.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>



// OIIO_THREAD_ALLOW_DCLP, if set to 0, prevents us from using a dodgy
// "double checked lock pattern" (DCLP).  We are very careful to construct
// it safely and correctly, and these uses improve thread performance for
// us.  But it confuses Thread Sanitizer, so this switch allows you to turn
// it off. Also set to 0 if you don't believe that we are correct in
// allowing this construct on all platforms.
#ifndef OIIO_THREAD_ALLOW_DCLP
#    define OIIO_THREAD_ALLOW_DCLP 1
#endif



// Some helpful links:
//
// Descriptions of the "new" gcc atomic intrinsics:
//    https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
// Old gcc atomic intrinsics:
//    https://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Atomic-Builtins.html
// C++11 and beyond std::atomic:
//    http://en.cppreference.com/w/cpp/atomic



OIIO_NAMESPACE_BEGIN

/// Null mutex that can be substituted for a real one to test how much
/// overhead is associated with a particular mutex.
class null_mutex {
public:
    null_mutex() noexcept {}
    ~null_mutex() noexcept {}
    void lock() noexcept {}
    void unlock() noexcept {}
    void lock_shared() noexcept {}
    void unlock_shared() noexcept {}
    bool try_lock() noexcept { return true; }
};

/// Null lock that can be substituted for a real one to test how much
/// overhead is associated with a particular lock.
template<typename T> class null_lock {
public:
    null_lock(T& m) noexcept {}
};



using std::mutex;
using std::recursive_mutex;
using std::thread;
typedef std::lock_guard<mutex> lock_guard;
typedef std::lock_guard<recursive_mutex> recursive_lock_guard;



/// Yield the processor for the rest of the timeslice.
///
inline void
yield() noexcept
{
#if defined(__GNUC__)
    sched_yield();
#elif defined(_MSC_VER)
    SwitchToThread();
#else
#    error No yield on this platform.
#endif
}



// Slight pause
inline void
pause(int delay) noexcept
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    for (int i = 0; i < delay; ++i)
        __asm__ __volatile__("pause;");

#elif defined(__GNUC__) && (defined(__arm__) || defined(__s390__))
    for (int i = 0; i < delay; ++i)
        __asm__ __volatile__("NOP;");

#elif defined(_MSC_VER)
    for (int i = 0; i < delay; ++i) {
#    if defined(_WIN64)
        YieldProcessor();
#    else
        _asm pause
#    endif /* _WIN64 */
    }

#else
    // No pause on this platform, just punt
    for (int i = 0; i < delay; ++i)
        ;
#endif
}



// Helper class to deliver ever longer pauses until we yield our timeslice.
class atomic_backoff {
public:
    atomic_backoff(int pausemax = 16) noexcept
        : m_count(1)
        , m_pausemax(pausemax)
    {
    }

    void operator()() noexcept
    {
        if (m_count <= m_pausemax) {
            pause(m_count);
            m_count *= 2;
        } else {
            yield();
        }
    }

private:
    int m_count;
    int m_pausemax;
};




/// A spin_mutex is semantically equivalent to a regular mutex, except
/// for the following:
///  - A spin_mutex is just 1 byte, whereas a regular mutex is quite
///    large (44 bytes for pthread).
///  - A spin_mutex is extremely fast to lock and unlock, whereas a regular
///    mutex is surprisingly expensive just to acquire a lock.
///  - A spin_mutex takes CPU while it waits, so this can be very
///    wasteful compared to a regular mutex that blocks (gives up its
///    CPU slices until it acquires the lock).
///
/// The bottom line is that mutex is the usual choice, but in cases where
/// you need to acquire locks very frequently, but only need to hold the
/// lock for a very short period of time, you may save runtime by using
/// a spin_mutex, even though it's non-blocking.
///
/// N.B. A spin_mutex is only the size of a bool.  To avoid "false
/// sharing", be careful not to put two spin_mutex objects on the same
/// cache line (within 128 bytes of each other), or the two mutexes may
/// effectively (and wastefully) lock against each other.
///
class spin_mutex {
public:
    spin_mutex(void) noexcept {}
    ~spin_mutex(void) noexcept {}

    /// Copy constructor -- initialize to unlocked.
    ///
    spin_mutex(const spin_mutex&) noexcept {}

    /// Assignment does not do anything, since lockedness should not
    /// transfer.
    const spin_mutex& operator=(const spin_mutex&) noexcept { return *this; }

    /// Acquire the lock, spin until we have it.
    ///
    void lock() noexcept
    {
        // To avoid spinning too tightly, we use the atomic_backoff to
        // provide increasingly longer pauses, and if the lock is under
        // lots of contention, eventually yield the timeslice.
        atomic_backoff backoff;

        // Try to get ownership of the lock. Though experimentation, we
        // found that OIIO_UNLIKELY makes this just a bit faster on gcc
        // x86/x86_64 systems.
        while (!OIIO_UNLIKELY(try_lock())) {
#if OIIO_THREAD_ALLOW_DCLP
            // The full try_lock() involves a test_and_set, which
            // writes memory, and that will lock the bus.  But a normal
            // read of m_locked will let us spin until the value
            // changes, without locking the bus. So it's faster to
            // check in this manner until the mutex appears to be free.
            // HOWEVER... Thread Sanitizer things this is an instance of
            // an unsafe "double checked lock pattern" (DCLP) and flags it
            // as an error. I think it's a false negative, because the
            // outer loop is still an atomic check, the inner non-atomic
            // loop only serves to delay, and can't lead to a true data
            // race. But we provide this build-time switch to, at least,
            // give a way to use tsan for other checks.
            do {
                backoff();
            } while (*(volatile bool*)&m_locked);
#else
            backoff();
#endif
        }
    }

    /// Release the lock that we hold.
    ///
    void unlock() noexcept
    {
        // Fastest way to do it is with a clear with "release" semantics
        m_locked.clear(std::memory_order_release);
    }

    /// Try to acquire the lock.  Return true if we have it, false if
    /// somebody else is holding the lock.
    bool try_lock() noexcept
    {
        return !m_locked.test_and_set(std::memory_order_acquire);
    }

    /// Helper class: scoped lock for a spin_mutex -- grabs the lock upon
    /// construction, releases the lock when it exits scope.
    class lock_guard {
    public:
        lock_guard(spin_mutex& fm) noexcept
            : m_fm(fm)
        {
            m_fm.lock();
        }
        ~lock_guard() noexcept { m_fm.unlock(); }

    private:
        lock_guard() = delete;
        lock_guard(const lock_guard& other) = delete;
        lock_guard& operator=(const lock_guard& other) = delete;
        spin_mutex& m_fm;
    };

private:
    std::atomic_flag m_locked = ATOMIC_FLAG_INIT;  // initialize to unlocked
};


typedef spin_mutex::lock_guard spin_lock;



#if 0

// OLD CODE vvvvvvvv


/// Spinning reader/writer mutex.  This is just like spin_mutex, except
/// that there are separate locking mechanisms for "writers" (exclusive
/// holders of the lock, presumably because they are modifying whatever
/// the lock is protecting) and "readers" (non-exclusive, non-modifying
/// tasks that may access the protectee simultaneously).
class spin_rw_mutex {
public:
    /// Default constructor -- initialize to unlocked.
    ///
    spin_rw_mutex (void) { m_readers = 0; }

    ~spin_rw_mutex (void) { }

    /// Copy constructor -- initialize to unlocked.
    ///
    spin_rw_mutex (const spin_rw_mutex &) { m_readers = 0; }

    /// Assignment does not do anything, since lockedness should not
    /// transfer.
    const spin_rw_mutex& operator= (const spin_rw_mutex&) { return *this; }

    /// Acquire the reader lock.
    ///
    void read_lock () {
        // Spin until there are no writers active
        m_locked.lock();
        // Register ourself as a reader
        ++m_readers;
        // Release the lock, to let other readers work
        m_locked.unlock();
    }

    /// Release the reader lock.
    ///
    void read_unlock () {
        --m_readers;  // it's atomic, no need to lock to release
    }

    /// Acquire the writer lock.
    ///
    void write_lock () {
        // Make sure no new readers (or writers) can start
        m_locked.lock();
        // Spin until the last reader is done, at which point we will be
        // the sole owners and nobody else (reader or writer) can acquire
        // the resource until we release it.
#if OIIO_THREAD_ALLOW_DCLP
        while (*(volatile int *)&m_readers > 0)
                ;
#else
        while (m_readers > 0)
                ;
#endif
    }

    /// Release the writer lock.
    ///
    void write_unlock () {
        // Let other readers or writers get the lock
        m_locked.unlock ();
    }

    /// Acquire an exclusive ("writer") lock.
    void lock () { write_lock(); }

    /// Release an exclusive ("writer") lock.
    void unlock () { write_unlock(); }

    /// Acquire a shared ("reader") lock.
    void lock_shared () { read_lock(); }

    /// Release a shared ("reader") lock.
    void unlock_shared () { read_unlock(); }

    /// Helper class: scoped read lock for a spin_rw_mutex -- grabs the
    /// read lock upon construction, releases the lock when it exits scope.
    class read_lock_guard {
    public:
        read_lock_guard (spin_rw_mutex &fm) : m_fm(fm) { m_fm.read_lock(); }
        ~read_lock_guard () { m_fm.read_unlock(); }
    private:
        read_lock_guard(); // Do not implement
        read_lock_guard(const read_lock_guard& other); // Do not implement
        read_lock_guard& operator = (const read_lock_guard& other); // Do not implement
        spin_rw_mutex & m_fm;
    };

    /// Helper class: scoped write lock for a spin_rw_mutex -- grabs the
    /// read lock upon construction, releases the lock when it exits scope.
    class write_lock_guard {
    public:
        write_lock_guard (spin_rw_mutex &fm) : m_fm(fm) { m_fm.write_lock(); }
        ~write_lock_guard () { m_fm.write_unlock(); }
    private:
        write_lock_guard(); // Do not implement
        write_lock_guard(const write_lock_guard& other); // Do not implement
        write_lock_guard& operator = (const write_lock_guard& other); // Do not implement
        spin_rw_mutex & m_fm;
    };

private:
    OIIO_CACHE_ALIGN
    spin_mutex m_locked;   // write lock
    char pad1_[OIIO_CACHE_LINE_SIZE-sizeof(spin_mutex)];
    OIIO_CACHE_ALIGN
    atomic_int m_readers;  // number of readers
    char pad2_[OIIO_CACHE_LINE_SIZE-sizeof(atomic_int)];
};


#else

// vvv New spin rw lock Oct 2017

/// Spinning reader/writer mutex.  This is just like spin_mutex, except
/// that there are separate locking mechanisms for "writers" (exclusive
/// holders of the lock, presumably because they are modifying whatever
/// the lock is protecting) and "readers" (non-exclusive, non-modifying
/// tasks that may access the protectee simultaneously).
class spin_rw_mutex {
public:
    /// Default constructor -- initialize to unlocked.
    ///
    spin_rw_mutex() noexcept {}

    ~spin_rw_mutex() noexcept {}

    // Do not allow copy or assignment.
    spin_rw_mutex(const spin_rw_mutex&) = delete;
    const spin_rw_mutex& operator=(const spin_rw_mutex&) = delete;

    /// Acquire the reader lock.
    ///
    void read_lock() noexcept
    {
        // first increase the readers, and if it turned out nobody was
        // writing, we're done. This means that acquiring a read when nobody
        // is writing is a single atomic operation.
        int oldval = m_bits.fetch_add(1, std::memory_order_acquire);
        if (!(oldval & WRITER))
            return;
        // Oops, we incremented readers but somebody was writing. Backtrack
        // by subtracting, and do things the hard way.
        int expected = (--m_bits) & NOTWRITER;

        // Do compare-and-exchange until we can increase the number of
        // readers by one and have no writers.
        if (m_bits.compare_exchange_weak(expected, expected + 1,
                                         std::memory_order_acquire))
            return;
        atomic_backoff backoff;
        do {
            backoff();
            expected = m_bits.load() & NOTWRITER;
        } while (!m_bits.compare_exchange_weak(expected, expected + 1,
                                               std::memory_order_acquire));
    }

    /// Release the reader lock.
    ///
    void read_unlock() noexcept
    {
        // Atomically reduce the number of readers.  It's at least 1,
        // and the WRITER bit should definitely not be set, so this just
        // boils down to an atomic decrement of m_bits.
        m_bits.fetch_sub(1, std::memory_order_release);
    }

    /// Acquire the writer lock.
    ///
    void write_lock() noexcept
    {
        // Do compare-and-exchange until we have just ourselves as writer
        int expected = 0;
        if (m_bits.compare_exchange_weak(expected, WRITER,
                                         std::memory_order_acquire))
            return;
        atomic_backoff backoff;
        do {
            backoff();
            expected = 0;
        } while (!m_bits.compare_exchange_weak(expected, WRITER,
                                               std::memory_order_acquire));
    }

    /// Release the writer lock.
    ///
    void write_unlock() noexcept
    {
        // Remove the writer bit
        m_bits.fetch_sub(WRITER, std::memory_order_release);
    }

    /// Helper class: scoped read lock for a spin_rw_mutex -- grabs the
    /// read lock upon construction, releases the lock when it exits scope.
    class read_lock_guard {
    public:
        read_lock_guard (spin_rw_mutex &fm) noexcept : m_fm(fm) { m_fm.read_lock(); }
        ~read_lock_guard () noexcept { m_fm.read_unlock(); }
    private:
        read_lock_guard(const read_lock_guard& other) = delete;
        read_lock_guard& operator = (const read_lock_guard& other) = delete;
        spin_rw_mutex & m_fm;
    };

    /// Helper class: scoped write lock for a spin_rw_mutex -- grabs the
    /// read lock upon construction, releases the lock when it exits scope.
    class write_lock_guard {
    public:
        write_lock_guard (spin_rw_mutex &fm) noexcept : m_fm(fm) { m_fm.write_lock(); }
        ~write_lock_guard () noexcept { m_fm.write_unlock(); }
    private:
        write_lock_guard(const write_lock_guard& other) = delete;
        write_lock_guard& operator = (const write_lock_guard& other) = delete;
        spin_rw_mutex & m_fm;
    };

private:
    // Use one word to hold the reader count, with a high bit indicating
    // that it's locked for writing.  This will only work if we have
    // fewer than 2^30 simultaneous readers.  I think that should hold
    // us for some time.
    enum { WRITER = 1<<30, NOTWRITER = WRITER-1 };
    std::atomic<int> m_bits { 0 };
};

#endif


typedef spin_rw_mutex::read_lock_guard spin_rw_read_lock;
typedef spin_rw_mutex::write_lock_guard spin_rw_write_lock;



/// Mutex pool. Sometimes, we have lots of objects that need to be
/// individually locked for thread safety, but two separate objects don't
/// need to lock against each other. If there are many more objects than
/// threads, it's wasteful for each object to contain its own mutex. So a
/// solution is to make a mutex_pool -- a collection of several mutexes.
/// Each object uses a hash to choose a consistent mutex for itself, but
/// which will be unlikely to be locked simultaneously by different object.
/// Semantically, it looks rather like an associative array of mutexes. We
/// also ensure that the mutexes are all on different cache lines, to ensure
/// that they don't exhibit false sharing. Try to choose Bins larger than
/// the expected number of threads that will be simultaneously locking
/// mutexes.
template<class Mutex, class Key, class Hash, size_t Bins = 16>
class mutex_pool {
public:
    mutex_pool() noexcept {}
    Mutex& operator[](const Key& key) noexcept { return m_mutex[m_hash(key) % Bins].m; }

private:
    // Helper type -- force cache line alignment. This should make an array
    // of these also have padding so that each individual mutex is aligned
    // to its own cache line, thus eliminating any "false sharing."
    struct AlignedMutex {
        OIIO_CACHE_ALIGN Mutex m;
    };

    AlignedMutex m_mutex[Bins];
    Hash m_hash;
};



/// Simple thread group class: lets you spawn a group of new threads,
/// then wait for them to all complete.
class thread_group {
public:
    thread_group() {}
    ~thread_group() { join_all(); }

    void add_thread(thread* t)
    {
        if (t) {
            lock_guard lock(m_mutex);
            m_threads.emplace_back(t);
        }
    }

    template<typename FUNC, typename... Args>
    thread* create_thread(FUNC func, Args&&... args)
    {
        thread* t = new thread(func, std::forward<Args>(args)...);
        add_thread(t);
        return t;
    }

    void join_all()
    {
        lock_guard lock(m_mutex);
        for (auto& t : m_threads)
            if (t->joinable())
                t->join();
    }

    size_t size() const
    {
        lock_guard lock(m_mutex);
        return m_threads.size();
    }

private:
    mutable mutex m_mutex;
    std::vector<std::unique_ptr<thread>> m_threads;
};



/// thread_pool is a persistent set of threads watching a queue to which
/// tasks can be submitted.
///
/// Call default_thread_pool() to retrieve a pointer to a single shared
/// thread_pool that will be initialized the first time it's needed, running
/// a number of threads corresponding to the number of cores on the machine.
///
/// It's possible to create other pools, but it's not something that's
/// recommended unless you really know what you're doing and are careful
/// that the sum of threads across all pools doesn't cause you to be highly
/// over-threaded. An example of when this might be useful is if you want
/// one pool of 4 threads to handle I/O without interference from a separate
/// pool of 4 (other) threads handling computation.
///
/// Submitting an asynchronous task to the queue follows the following
/// pattern:
///
///     /* func that takes a thread ID followed possibly by more args */
///     result_t my_func (int thread_id, Arg1 arg1, ...) { }
///     pool->push (my_func, arg1, ...);
///
/// If you just want to "fire and forget", then:
///
///     pool->push (func, ...args...);
///
/// But if you need a result, or simply need to know when the task has
/// completed, note that the push() method will return a future<result_t>
/// that you can check, like this:
///
///     std::future<result_t> f = pool->push (my_task);
///
/// And then you can
///
///     find out if it's done:              if (f.valid()) ...
///     wait for it to get done:            f.wait();
///     get result (waiting if necessary):  result_t r = f.get();
///
/// A common idiom is to fire a bunch of sub-tasks at the queue, and then
/// wait for them to all complete. We provide a helper class, task_set,
/// to make this easy:
///
///     task_set tasks (pool);
///     for (int i = 0; i < n_subtasks; ++i)
///         tasks.push (pool->push (myfunc));
///     tasks.wait ();
///
/// Note that the tasks.wait() is optional -- it will be called
/// automatically when the task_set exits its scope.
///
/// The task function's first argument, the thread_id, is the thread number
/// for the pool, or -1 if it's being executed by a non-pool thread (this
/// can happen in cases where the whole pool is occupied and the calling
/// thread contributes to running the work load).
///
/// Thread pool. Have fun, be safe.
///
class OIIO_API thread_pool {
public:
    /// Initialize the pool.  This implicitly calls resize() to set the
    /// number of worker threads, defaulting to a number of workers that is
    /// one less than the number of hardware cores.
    thread_pool(int nthreads = -1);
    ~thread_pool();

    /// How many threads are in the pool?
    int size() const;

    /// Sets the number of worker threads in the pool. If the pool size is
    /// 0, any tasks added to the pool will be executed immediately by the
    /// calling thread. Requesting nthreads < 0 will cause it to resize to
    /// the number of hardware cores minus one (one less, to account for the
    /// fact that the calling thread will also contribute). BEWARE! Resizing
    /// the queue should not be done while jobs are running.
    void resize(int nthreads = -1);

    /// Return the number of currently idle threads in the queue. Zero
    /// means the queue is fully engaged.
    int idle() const;

    /// Run the user's function that accepts argument int - id of the
    /// running thread. The returned value is templatized std::future, where
    /// the user can get the result and rethrow any exceptions. If the queue
    /// has no worker threads, the task will be run immediately by the
    /// calling thread.
    template<typename F> auto push(F&& f) -> std::future<decltype(f(0))>
    {
        auto pck = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(
            std::forward<F>(f));
        if (size() < 1) {
            (*pck)(-1);  // No worker threads, run it with the calling thread
        } else {
            auto _f = new std::function<void(int id)>(
                [pck](int id) { (*pck)(id); });
            push_queue_and_notify(_f);
        }
        return pck->get_future();
    }

    /// Run the user's function that accepts an arbitrary set of arguments
    /// (also passed). The returned value is templatized std::future, where
    /// the user can get the result and rethrow any exceptions. If the queue
    /// has no worker threads, the task will be run immediately by the
    /// calling thread.
    template<typename F, typename... Rest>
    auto push (F && f, Rest&&... rest) ->std::future<decltype(f(0, rest...))> {
        auto pck = std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(
            std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Rest>(rest)...)
        );
        if (size() < 1) {
            (*pck)(-1); // No worker threads, run it with the calling thread
        } else {
            auto _f = new std::function<void(int id)>([pck](int id) {
                (*pck)(id);
            });
            push_queue_and_notify (_f);
        }
        return pck->get_future();
    }

    /// If there are any tasks on the queue, pull one off and run it (on
    /// this calling thread) and return true. Otherwise (there are no
    /// pending jobs), return false immediately. This utility is what makes
    /// it possible for non-pool threads to also run tasks from the queue
    /// when they would ordinarily be idle. The thread id of the caller
    /// should be passed.
    bool run_one_task(std::thread::id id);

    /// Return true if the calling thread is part of the thread pool. This
    /// can be used to limit a pool thread from inadvisedly adding its own
    /// subtasks to clog up the pool.
    /// DEPRECATED(2.1) -- use is_worker() instead.
    bool this_thread_is_in_pool() const;

    /// Register a thread (not already in the thread pool itself) as working
    /// on tasks in the pool. This is used to avoid recursion.
    void register_worker(std::thread::id id);
    /// De-register a thread, saying it is no longer in the process of
    /// taking work from the thread pool.
    void deregister_worker(std::thread::id id);
    /// Is the thread in the pool or currently engaged in taking tasks from
    /// the pool?
    bool is_worker(std::thread::id id) const;
    bool is_worker() const { return is_worker(std::this_thread::get_id()); }
    // Non-const versions: DEPRECATED(2.1)
    bool is_worker(std::thread::id id);

    /// How many jobs are waiting to run?  (Use with caution! Can be out of
    /// date by the time you look at it.)
    size_t jobs_in_queue() const;

    /// Is the pool very busy? Meaning that there are significantly more
    /// tasks in the queue waiting to run than there are threads in the
    /// pool. It may be wise for a caller to check this before submitting
    /// tasks -- if the queue is very busy, it's probably more expedient to
    /// execute the code directly rather than add it to an oversubscribed
    /// queue.
    bool very_busy() const;

private:
    // Disallow copy construction and assignment
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    // PIMPL pattern hides all the guts far away from the public API
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Utility function that helps us hide the implementation
    void push_queue_and_notify(std::function<void(int id)>* f);
};



/// Return a reference to the "default" shared thread pool. In almost all
/// ordinary circumstances, you should use this exclusively to get a
/// single shared thread pool, since creating multiple thread pools
/// could result in hilariously over-threading your application.
OIIO_API thread_pool* default_thread_pool ();



/// task_set is a group of future<void>'s from a thread_queue that you can
/// add to, and when you either call wait() or just leave the task_set's
/// scope, it will wait for all the tasks in the set to be done before
/// proceeding.
///
/// A typical idiom for using this is:
///
///    void myfunc (int id) { ... do something ... }
///
///    thread_pool* pool (default_thread_pool());
///    {
///        task_set tasks (pool);
///        // Launch a bunch of tasks into the thread pool
///        for (int i = 0; i < ntasks; ++i)
///            tasks.push (pool->push (myfunc));
///        // The following brace, by ending the scope of 'tasks', will
///        // wait for all those queue tasks to finish.
///    }
///
class OIIO_API task_set {
public:
    task_set(thread_pool* pool = nullptr)
        : m_pool(pool ? pool : default_thread_pool())
        , m_submitter_thread(std::this_thread::get_id())
    {
    }
    ~task_set() { wait(); }

    task_set(const task_set&) = delete;
    const task_set& operator=(const task_set&) = delete;

    // Return the thread id of the thread that set up this task_set and
    // submitted its tasks to the thread pool.
    std::thread::id submitter() const { return m_submitter_thread; }

    // Save a future (presumably returned by a threadpool::push() as part
    // of this task set.
    void push(std::future<void>&& f)
    {
        OIIO_DASSERT(
            std::this_thread::get_id() == submitter()
            && "All tasks in a tast_set should be added by the same thread");
        m_futures.emplace_back(std::move(f));
    }

    // Wait for the given taskindex (0..n-1, where n is the number of tasks
    // submitted as part of this task_set). If block == true, fully block
    // while waiting for that task to finish. If block is false, then busy
    // wait, and opportunistically run queue tasks yourself while you are
    // waiting for the task to finish.
    void wait_for_task(size_t taskindex, bool block = false);

    // Wait for all tasks in the set to finish. If block == true, fully
    // block while waiting for the pool threads to all finish. If block is
    // false, then busy wait, and opportunistically run queue tasks yourself
    // while you are waiting for other tasks to finish.
    void wait(bool block = false);

    // Debugging sanity check, called after wait(), to ensure that all the
    // tasks were completed.
    void check_done()
    {
        const std::chrono::milliseconds wait_time(0);
        for (auto&& f : m_futures)
            OIIO_ASSERT(f.wait_for(wait_time) == std::future_status::ready);
    }

private:
    thread_pool* m_pool;
    std::thread::id m_submitter_thread;
    std::vector<std::future<void>> m_futures;
};


OIIO_NAMESPACE_END
