// AMD AMDUtils code
// 
// Copyright(c) 2017 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "ThreadPool.h"

// This is a multithreaded shader cache. This is how it works:
//
// Each shader compilation is invoked by a app thread using the Async class, this class executes the shader compilation in a new thread.
// To prevent context swiches we need to limit the number of running threads to the number of cores. 
// For that reason there is a global counter that keeps track of the number of running threads. 
// This counter gets incremented when the thread is running a task and decremented when it finishes.
// It is also decremented when a thread is put into Wait mode and incremented when a thread is signaled AND there is a core available to resume the thread.
// If all cores are busy the app thread is put to Wait to prevent it from spawning more threads.
//
// When multiple threads attempt to compile the same shader it happens the following:
// 1) the thread that first comes gets to compile the shader
// 2) the rest of threads (let's say 'n') are put into _Wait_ mode
// 3) Since 'n' cores are now free, 'n' threads are resumed/spawned to keep executing other tasks
//
// This way all the cores should be plenty of work and thread context switches should be minimal.
//
// 

class Sync
{
    int m_count = 0;
    std::mutex m_mutex;
    std::condition_variable condition;
public:
    int Inc()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count++;
        return m_count;
    }

    int Dec()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count--;
        if (m_count == 0)
            condition.notify_all();
        return m_count;
    }

    int Get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_count;
    }

    void Reset()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count = 0;
        condition.notify_all();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_count != 0)
            condition.wait(lock);
    }

};

class Async
{
    static int s_activeThreads;
    static int s_maxThreads;
    static std::mutex s_mutex;
    static std::condition_variable s_condition;
    static bool s_bExiting;

    std::function<void()> m_job;
    Sync *m_pSync;
    std::thread *m_pT;

public:
    Async(std::function<void()> job, Sync *pSync = NULL);
    ~Async();
    static void Wait(Sync *pSync);
};

#define CACHE_ENABLE 
//#define CACHE_LOG 

template<typename  T>
class Cache
{
    struct CacheEntry
    {
        Sync m_Sync;
        T m_data;
    };

    std::map<size_t, CacheEntry> m_database;
    std::mutex m_mutex;

public:
    bool CacheMiss(size_t hash, T *pOut)
    {
#ifdef CACHE_ENABLE
        std::map<size_t, CacheEntry>::iterator it;

        // find whether the shader is in the cache, create an empty entry just so other threads know this thread will be compiling the shader
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            it = m_database.find(hash);

            // shader not found, we need to compile the shader!
            if (it == m_database.end())
            {               
                // inc syncing object so other threads requesting this same shader can tell there is a compilation in progress and they need to wait for this thread to finish.
                m_database[hash].m_Sync.Inc();
                return true;
            }
        }


        // If we have seen these shade before then:
        {
            // If there is a thread already trying to compile this shader then wait for that thread to finish
            if (it->second.m_Sync.Get() == 1)
            {
                #ifdef CACHE_LOG
                Trace(format("thread 0x%04x Wait: %p %i\n", GetCurrentThreadId(), hash, it->second.m_Sync.Get()));
                #endif
                Async::Wait(&it->second.m_Sync);
            }

            // if the shader was compiled then return it
            *pOut = it->second.m_data;
            
            #ifdef CACHE_LOG
            Trace(format("thread 0x%04x Was cache: %p \n", GetCurrentThreadId(), hash));
            #endif
            return false;
        }
#endif
        return true;
    }

    void UpdateCache(size_t hash, T *pValue)
    {
#ifdef CACHE_ENABLE
        std::map<size_t, CacheEntry>::iterator it;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            it = m_database.find(hash);
        }
        #ifdef CACHE_LOG
        Trace(format("thread 0x%04x Compi: %p %i\n", GetCurrentThreadId(), hash, it->second.m_Sync.Get()));
        #endif
        it->second.m_data = *pValue;
        //assert(it->second.m_Sync.Get() == 1);
        
        // The shader has been compiled, set sync to 0 to indicate it is compiled
        // This also wakes up all the threads waiting on  Async::Wait(&it->second.m_Sync);
        it->second.m_Sync.Dec();  
#endif
    }

    std::map<size_t, CacheEntry> *GetDatabase() 
    {
        return &m_database;
    };
};


