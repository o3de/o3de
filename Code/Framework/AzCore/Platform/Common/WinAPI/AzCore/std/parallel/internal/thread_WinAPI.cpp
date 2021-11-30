/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/std/parallel/threadbus.h>

namespace AZStd
{
    namespace Platform
    {
        unsigned __stdcall PostThreadRun();
        HANDLE CreateThread(unsigned stackSize, unsigned (__stdcall* threadRunFunction)(void*), AZStd::Internal::thread_info* ti, unsigned int* id);
        unsigned HardwareConcurrency();
        void SetThreadName(HANDLE hThread, const char* threadName);
    }

    // Since we avoid including windows.h in header files in the code, we are declaring storage for CRITICAL_SECTION and CONDITION_VARIABLE
    // Make sure at compile that that the storage is enough to store the variables
    static_assert(sizeof(native_mutex_data_type) >= sizeof(CRITICAL_SECTION), "native_mutex_data_type is used to store CRITICAL_SECTION, it should be big enough!");
    static_assert(sizeof(native_cond_var_data_type) >= sizeof(CONDITION_VARIABLE), "native_mutex_data_type is used to store CONDITION_VARIABLE, it should be big enough!");

    namespace Internal
    {
        /**
         * Thread run function
         */
        unsigned __stdcall thread_run_function(void* param)
        {
            thread_info* ti = reinterpret_cast<thread_info*>(param);
            ti->execute();
            destroy_thread_info(ti);

            ThreadEventBus::Broadcast(&ThreadEventBus::Events::OnThreadExit, this_thread::get_id()); // goes to client listeners
            ThreadDrillerEventBus::Broadcast(&ThreadDrillerEventBus::Events::OnThreadExit, this_thread::get_id()); // goes to the profiler.

            return Platform::PostThreadRun();
        }

        /**
         * Create and run thread
         */
        HANDLE create_thread(const thread_desc* desc, thread_info* ti, unsigned int* id)
        {
            unsigned stackSize = 0;
            *id = native_thread_invalid_id;
            HANDLE hThread;
            if (desc && desc->m_stackSize != -1)
            {
                stackSize = desc->m_stackSize;
            }

            hThread = Platform::CreateThread(stackSize, &thread_run_function, ti, id);
            if (hThread == NULL)
            {
                return hThread;
            }

            if (desc && desc->m_priority >= -15 && desc->m_priority <= 15)
            {
                ::SetThreadPriority(hThread, desc->m_priority);
            }

            if (desc && desc->m_cpuId != -1)
            {
                SetThreadAffinityMask(hThread, DWORD_PTR(desc->m_cpuId));
            }

            ThreadEventBus::Broadcast(&ThreadEventBus::Events::OnThreadEnter, thread::id(*id), desc);
            ThreadDrillerEventBus::Broadcast(&ThreadDrillerEventBus::Events::OnThreadEnter, thread::id(*id), desc);

            ::ResumeThread(hThread);

            if (desc && desc->m_name)
            {
                Platform::SetThreadName(hThread, desc->m_name);
            }

            return hThread;
        }
    }


    thread::thread()
    {
        m_thread.m_id = native_thread_invalid_id;
    }

    thread::thread(Internal::thread_move_t<thread> rhs)
    {
        m_thread.m_handle = rhs->m_thread.m_handle;
        m_thread.m_id = rhs->m_thread.m_id;
        rhs->m_thread.m_id = native_thread_invalid_id;
    }

    thread::~thread()
    {
        AZ_Assert(!joinable(), "You must call detach or join before you delete a thread!");
    }

    void thread::join()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (m_thread.m_id != native_thread_invalid_id)
        {
            WaitForSingleObject(m_thread.m_handle, INFINITE);
            detach();
        }
    }
    void thread::detach()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (m_thread.m_id != native_thread_invalid_id)
        {
            CloseHandle(m_thread.m_handle);
            m_thread.m_id = native_thread_invalid_id;
        }
    }

    /// Return number of physical processors
    unsigned thread::hardware_concurrency()
    {
        return Platform::HardwareConcurrency();
    }
    //////////////////////////////////////////////////////////////////////////
}
