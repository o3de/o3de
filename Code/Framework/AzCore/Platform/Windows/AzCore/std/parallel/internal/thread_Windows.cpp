/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/conversions.h>

#include <process.h>

namespace AZStd
{
    namespace Platform
    {
        unsigned __stdcall PostThreadRun()
        {
            _endthreadex(0);
            return 0;
        }

        HANDLE CreateThread(unsigned stackSize, unsigned (__stdcall* threadRunFunction)(void*), AZStd::Internal::thread_info* ti, unsigned int* id)
        {
            return (HANDLE)_beginthreadex(0, stackSize, threadRunFunction, ti, CREATE_SUSPENDED, id);
        }

        unsigned HardwareConcurrency()
        {
            SYSTEM_INFO info = {};
            GetSystemInfo(&info);
            return info.dwNumberOfProcessors;
        }

        static bool SetThreadNameOS(HANDLE hThread, const char* threadName)
        {
            // SetThreadDescription was added in 1607 (aka RS1). Since we can't guarantee the user is running 1607 or later, we need to ask for the function from the kernel.
            using SetThreadDescriptionFunc = HRESULT(WINAPI*)(_In_ HANDLE hThread, _In_ PCWSTR lpThreadDescription);

            HMODULE kernel32Handle = ::GetModuleHandleW(L"Kernel32.dll");
            if (kernel32Handle)
            {
                SetThreadDescriptionFunc setThreadDescription = reinterpret_cast<SetThreadDescriptionFunc>(::GetProcAddress(kernel32Handle, "SetThreadDescription"));
                if (setThreadDescription)
                {
                    // Convert the thread name to Unicode
                    AZStd::wstring threadNameW;
                    AZStd::to_wstring(threadNameW, threadName);
                    if (!threadNameW.empty())
                    {
                        setThreadDescription(hThread, threadNameW.c_str());
                        return true;
                    }
                }
            }

            return false;
        }

        static bool SetThreadNameDebuggerOnly(DWORD threadId, const char* threadName)
        {
            // For understanding the types and values used here, please see:
            // https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code

            const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
            struct THREADNAME_INFO
            {
                DWORD dwType = 0x1000; // Must be 0x1000
                LPCSTR szName;         // Pointer to name (in user address space)
                DWORD dwThreadID;      // Thread ID (-1 for caller thread)
                DWORD dwFlags = 0;     // Reserved for future use; must be zero
            };
#pragma pack(pop)

            THREADNAME_INFO info;
            info.szName = threadName;
            info.dwThreadID = threadId;

            __try
            {
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR*>(&info));
                return true;
            }
            __except (GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        void SetThreadName(HANDLE hThread, const char* threadName)
        {
            if (!SetThreadNameOS(hThread, threadName))
            {
                SetThreadNameDebuggerOnly(GetThreadId(hThread), threadName);
            }
        }
    }
}
