/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Platform specific DXGL requirements implementation relying
//               on CryCommon and CrySystem.


#ifndef __GLCRYPLATFORM__
#define __GLCRYPLATFORM__

#include "DriverD3D.h"
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/FileOperations.h>

namespace NCryOpenGL
{
    inline void* Malloc(size_t size)
    {
        return CryModuleMalloc(size);
    }

    inline void* Calloc(size_t num, size_t size)
    {
        return CryModuleCalloc(num, size);
    }

    inline void* Realloc(void* memblock, size_t size)
    {
        return CryModuleRealloc(memblock, size);
    }

    inline void Free(void* memblock)
    {
        CryModuleFree(memblock);
    }

    inline void* Memalign(size_t size, size_t alignment)
    {
        return CryModuleMemalign(size, alignment);
    }

    inline void MemalignFree(void* memblock)
    {
        return CryModuleMemalignFree(memblock);
    }

    void LogMessage(ELogSeverity eSeverity, const char* szFormat, ...) PRINTF_PARAMS(2, 3);

    inline void LogMessage(ELogSeverity eSeverity, const char* szFormat, ...)
    {
        EValidatorSeverity eValidatorSeverity(VALIDATOR_COMMENT);
        switch (eSeverity)
        {
        case eLS_Error:
            eValidatorSeverity = VALIDATOR_ERROR;
            break;
        case eLS_Warning:
            eValidatorSeverity = VALIDATOR_WARNING;
            break;
        case eLS_Info:
            eValidatorSeverity = VALIDATOR_COMMENT;
            break;
        }

        va_list kArgs;
        va_start(kArgs, szFormat);
        GetISystem()->WarningV(VALIDATOR_MODULE_RENDERER, eValidatorSeverity, 0, 0, szFormat, kArgs);
        va_end(kArgs);
    }

    inline void BreakUnique(const char* file, uint32 line, const char* functionName)
    {
        AZ_Error("OpenGL Layer", false, "Unimplemented DirectX function %s being called in OpenGL in file %s line %d. Continuing but rendering artifacts may be present.", functionName, file, line);
    }

    inline uint32 GetCRC32(const void* pData, size_t uSize)
    {
        return CCrc32::Compute(pData, uSize);
    }

    inline LONG Exchange(LONG volatile* piDestination, LONG iExchange)
    {
#if defined(WIN32)
        return InterlockedExchange(piDestination, iExchange);
#else
        LONG iOldValue(__sync_lock_test_and_set(piDestination, iExchange));
        __sync_synchronize();
        return iOldValue;
#endif
    }

    inline LONG CompareExchange(LONG volatile* piDestination, LONG iExchange, LONG iComparand)
    {
        return CryInterlockedCompareExchange(piDestination, iExchange, iComparand);
    }

    inline LONG AtomicIncrement(LONG volatile* piDestination)
    {
        return CryInterlockedIncrement(reinterpret_cast<int volatile*>(piDestination));
    }

    inline LONG AtomicDecrement(LONG volatile* piDestination)
    {
        return CryInterlockedDecrement(reinterpret_cast<int volatile*>(piDestination));
    }

    typedef CryCriticalSection TCriticalSection;

    inline void LockCriticalSection(TCriticalSection* pCriticalSection)
    {
        pCriticalSection->Lock();
    }

    inline void UnlockCriticalSection(TCriticalSection* pCriticalSection)
    {
        pCriticalSection->Unlock();
    }

    struct STraceFile
    {
        STraceFile()
            : m_fileHandle(AZ::IO::InvalidHandle)
        {
        }

        ~STraceFile()
        {
            if (m_fileHandle != AZ::IO::InvalidHandle)
            {
                gEnv->pCryPak->FClose(m_fileHandle);
            }
        }

        bool Open(const char* szFileName, bool bBinary)
        {
            static const char* s_szTraceDirectory = "DXGLTrace";

            if (m_fileHandle != AZ::IO::InvalidHandle)
            {
                return false;
            }

            char acFullPath[MAX_PATH];
            sprintf_s(acFullPath, "%s/%s", s_szTraceDirectory, szFileName);
            const char* szMode(bBinary ? "wb" : "w");
            m_fileHandle = gEnv->pCryPak->FOpen(acFullPath, szMode);
            if (m_fileHandle != AZ::IO::InvalidHandle)
            {
                return true;
            }

            gEnv->pCryPak->MakeDir(s_szTraceDirectory, 0);
            m_fileHandle = gEnv->pCryPak->FOpen(acFullPath, szMode);
            return m_fileHandle != AZ::IO::InvalidHandle;
        }

        void Write(const void* pvData, uint32 uSize)
        {
            gEnv->pCryPak->FWrite(pvData, (size_t)uSize, 1, m_fileHandle);
        }

        void Printf(const char* szFormat, ...)
        {
            va_list kArgs;
            va_start(kArgs, szFormat);
            AZ::IO::PrintV(m_fileHandle, szFormat, kArgs);
            va_end(kArgs);
        }

        AZ::IO::HandleType m_fileHandle;
    };

    inline void RegisterConfigVariable(const char* szName, int* piVariable, int iDefaultValue)
    {
        gEnv->pConsole->Register(szName, piVariable, iDefaultValue);
    }

    inline void PushProfileLabel(const char* szName)
    {
        PROFILE_LABEL_PUSH(szName);
    }

    inline void PopProfileLabel(const char* szName)
    {
        PROFILE_LABEL_POP(szName);
    }
}



#endif //__GLCRYPLATFORM__
