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

// Description : Streaming Engine


#include "CrySystem_precompiled.h"
#include <ISystem.h>
#include <ILog.h>

#include "AZRequestReadStream.h"
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/Component/TickBus.h>
#include "StreamEngine.h"

AZRequestReadStream* AZRequestReadStream::Allocate(const EStreamTaskType tSource, const char* filename, IStreamCallback* callback,
    const StreamReadParams* params)
{
    //Once an async method is available to read file sizes this code should be removed:
    // and the file size should be known before calling this method and pass it as a
    // parameter to this method.
    //REMOVE In the Future START.
    AZ::IO::SizeType fileSize = 0;
    if (params && params->nSize)
    {
        fileSize = params->nSize;
    }
    else
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::Result res = fileIO->Size(filename, fileSize);
        if (!res)
        {
            AZ_Error("AZRequestReadStream", false, "Failed to read file size of %s", filename);
            return nullptr;
        }
        //REMOVE In the Future END.
    }

    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();

    AZRequestReadStream* retReq;
    retReq = aznew AZRequestReadStream();

    retReq->m_Type = tSource;
    retReq->m_fileName = filename;
    retReq->m_callback = callback;
    retReq->m_fileSize = fileSize;
    //REMARK: if params->pBuffer is NOT NULL, then retReq->m_buffer
    //should become params->pBuffer, this is called stream-in-place.
    //The only reason we are not doing this here is because
    //some platforms support stream-in-place to WRITE ONLY buffers.
    //Because there are no guarantees that low level streaming and decompression apis
    //would treat the output buffer as WRITE ONLY, we still allocate the buffer and memcpy
    //to  params->pBuffer upon the completion callback being called.
    //Once LY-98089 is complete/fixed, we should be able to safely
    //set retReq->m_buffer =  params->pBuffer and skip the memory allocation.
    retReq->m_buffer = azmalloc(fileSize, streamer->GetRecommendations().m_memoryAlignment);
    if (params)
    {
        retReq->m_params = *params;
    }

    return retReq;
}

//////////////////////////////////////////////////////////////////////////
AZRequestReadStream::AZRequestReadStream() : m_fileName(""), m_fileRequest(nullptr),
    m_buffer(nullptr), m_Type(eStreamTaskTypeTexture),
    m_callback(nullptr), m_fileSize(0), m_numBytesRead(0), m_isAsyncCallbackExecuted(false),
    m_isSyncCallbackExecuted(false), m_isFileRequestComplete(false), m_isError(false), m_isFinished(false),
    m_IOError(0)
{
    AZStd::atomic_init<int>(&m_refCount, 0);
    m_params = StreamReadParams();
}

//////////////////////////////////////////////////////////////////////////
AZRequestReadStream::~AZRequestReadStream()
{
    azfree(m_buffer);
}

// tries to stop reading the stream; this is advisory and may have no effect
// all the callbacks    will be called after this. If you just destructing object,
// dereference this object and it will automatically abort and release all associated resources.
void AZRequestReadStream::Abort()
{
    {
        CryAutoCriticalSection lock(m_callbackLock);
        // Increase ref counting to avoid preliminary destruction
        AZRequestReadStream_AutoPtr refCountLock(this);

        if (m_isFileRequestComplete || m_isError)
        {
            // It is possible the file I/O request to be completed by AZ::IO::Streamer,
            // but if the completion callback is deferred for the main thread then
            // the stream is not finished. So, only if it is finished then
            // it is safe to do nothing.
            if (m_isFinished)
            {
                return;
            }
        }

        m_isError = true;
        m_IOError = ERROR_USER_ABORT;
        m_isFileRequestComplete = true;
        m_numBytesRead = 0;

        if (m_fileRequest)
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            streamer->QueueRequest(streamer->Cancel(m_fileRequest));
        }

        // all the callbacks have to handle error cases and needs to be called anyway, even if the stream I/O is aborted
        ExecuteAsyncCallback_CBLocked();
        ExecuteSyncCallback_CBLocked();

        m_callback = nullptr;
    }

}

bool AZRequestReadStream::TryAbort()
{
    // Increase ref counting to avoid preliminary destruction
    AZRequestReadStream_AutoPtr refCountLock(this);

    if (!m_callbackLock.TryLock())
    {
        return false;
    }

    if (m_isFileRequestComplete || m_isError)
    {
        // It is possible the file I/O request to be completed by AZ::IO::Streamer,
        // but if the completion callback is deferred for the main thread then
        // the stream is not finished. So, only if it is finished then
        // it is safe to do nothing.
        if (m_isFinished)
        {
            m_callbackLock.Unlock();
            return false;
        }
    }

    m_isError = true;
    m_IOError = ERROR_USER_ABORT;
    m_isFileRequestComplete = true;
    m_numBytesRead = 0;

    if (m_fileRequest)
    {
        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        streamer->QueueRequest(streamer->Cancel(m_fileRequest));
    }

    // all the callbacks have to handle error cases and needs to be called anyway, even if the stream I/O is aborted
    ExecuteAsyncCallback_CBLocked();
    ExecuteSyncCallback_CBLocked();

    m_callback = nullptr;

    m_callbackLock.Unlock();

    return true;
}

// tries to raise the priority of the read; this is advisory and may have no effect
void AZRequestReadStream::SetPriority(EStreamTaskPriority ePriority)
{
    CryAutoCriticalSection lock(m_callbackLock);

    if (m_params.ePriority != ePriority)
    {
        m_params.ePriority = ePriority;
        if (m_fileRequest)
        {
            AZ::Interface<AZ::IO::IStreamer>::Get()->RescheduleRequest(m_fileRequest, AZ::IO::IStreamerTypes::s_noDeadline,
                CStreamEngine::CryStreamPriorityToAZStreamPriority(ePriority));
        }
    }
}

// unconditionally waits until the callback is called
// i.e. if the stream hasn't yet finish, it's guaranteed that the user-supplied callback
// is called before return from this function (unless no callback was specified)
void AZRequestReadStream::Wait(int maxWaitMillis)
{
    // lock this object to avoid preliminary destruction
    AZRequestReadStream_AutoPtr refCountLock(this);

    if (!m_isFinished && !m_isError && !m_fileRequest)
    {
        AZ_Error("AZRequestReadStream", false, "Stream for file %s is unwaitable", m_fileName.c_str());
        return;
    }

    if (maxWaitMillis > 0)
    {
        m_wait.try_acquire_for(AZStd::chrono::milliseconds(maxWaitMillis));
    }
    else
    {
        m_wait.acquire();
    }
}

//////////////////////////////////////////////////////////////////////////
const char* AZRequestReadStream::GetErrorName() const
{
    switch (m_IOError)
    {
    case ERROR_UNKNOWN_ERROR:
        return "Unknown error";
    case ERROR_UNEXPECTED_DESTRUCTION:
        return "Unexpected destruction";
    case ERROR_INVALID_CALL:
        return "Invalid call";
    case ERROR_CANT_OPEN_FILE:
        return "Cannot open the file";
    case ERROR_REFSTREAM_ERROR:
        return "Refstream error";
    case ERROR_OFFSET_OUT_OF_RANGE:
        return "Offset out of range";
    case ERROR_REGION_OUT_OF_RANGE:
        return "Region out of range";
    case ERROR_SIZE_OUT_OF_RANGE:
        return "Size out of range";
    case ERROR_CANT_START_READING:
        return "Cannot start reading";
    case ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case ERROR_ABORTED_ON_SHUTDOWN:
        return "Aborted on shutdown";
    case ERROR_OUT_OF_MEMORY_QUOTA:
        return "Out of memory quota";
    case ERROR_ZIP_CACHE_FAILURE:
        return "ZIP cache failure";
    case ERROR_USER_ABORT:
        return "User aborted";
    }
    return "Unrecognized error";
}

int AZRequestReadStream::AddRef()
{
    return m_refCount.fetch_add(1) + 1;
}

int AZRequestReadStream::Release()
{
    int refCount = m_refCount.fetch_sub(1);

#ifndef _RELEASE
    if (refCount < 1)
    {
        __debugbreak();
    }
#endif

    if (refCount == 1)
    {
        //UNUSUAL, yet necessary.
        //Why "delete this"?
        //So, AZRequestReadStream is a replacement of CReadStream. The original design of 
        //Cry Texture Mips Streaming makes use of CReadStream through IReadStreamPtr, which
        //is a smart pointer design that calls AddRef() and Release() but never calls "delete",
        //like AZStd::shared_ptr<> does. This means the original Cry design had a memory leak
        //because it never called delete on IReadStream objects. If you look at the original
        //code of CReadStream (StreamReadStream.cpp) , the static Allocate method has two paths
        //to allocate memory, one used a stack based memory allocation hack, and the other path
        //was doing a "new CReadStream". Using VS2017 debugger I found both paths to be used, but
        //"delete" and hence the destructor of CReadStream is never called causing minor memory leaks.
        //The best solution I found was to call "delete this" here and later when we chnage IReadStreamPtr
        //for AZstd::smart_ptr then AddRef() and Release() won't be needed anymore and this "delete this"
        //hack won't be necessary either.
        delete this;
    }

    return refCount - 1;
}

//////////////////////////////////////////////////////////////////////////
void AZRequestReadStream::ExecuteAsyncCallback_CBLocked()
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    if (!m_isAsyncCallbackExecuted && m_callback)
    {
        m_isAsyncCallbackExecuted = true;
        m_callback->StreamAsyncOnComplete(this, m_IOError);
    }
}

void AZRequestReadStream::ExecuteSyncCallback_CBLocked()
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    if (!m_isSyncCallbackExecuted && m_callback && (0 == (m_params.nFlags & IStreamEngine::FLAGS_NO_SYNC_CALLBACK)))
    {
        m_isSyncCallbackExecuted = true;

        AZRequestReadStream_AutoPtr refCountLock(this); // Stream can be freed inside the callback!

        m_callback->StreamOnComplete(this, m_IOError);

        m_isFinished = true;
        FreeTemporaryMemory();
    }

}

//////////////////////////////////////////////////////////////////////////
void AZRequestReadStream::FreeTemporaryMemory()
{
    // Make sure m_buffer is not freed if the file request is still in flight, as Streamer can still write to m_buffer in that case
    if (!m_fileRequest || AZ::Interface<AZ::IO::IStreamer>::Get()->HasRequestCompleted(m_fileRequest))
    {
        azfree(m_buffer);
        m_buffer = nullptr;
        m_numBytesRead = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void AZRequestReadStream::OnRequestComplete(AZ::IO::SizeType numBytesRead, [[maybe_unused]] void* buffer, AZ::IO::IStreamerTypes::RequestStatus requestState)
{
    CryAutoCriticalSection lock(m_callbackLock);

    if (!m_isFileRequestComplete)
    {
        switch (requestState)
        {
        case AZ::IO::IStreamerTypes::RequestStatus::Completed:
            m_IOError = 0;
            m_numBytesRead = static_cast<uint32>(numBytesRead);
            m_isError = false;
            if (m_params.pBuffer)
            {
                //In some systems, streaming-in-place is supported. The caveat
                //is that in some cases, the destination buffer is write-only. This is why
                //a final memcpy must be done here until support is added to AZ::IO::Streamer API
                //to decompress/load data into write-only buffers. SEE: LY-98089
                AZ_Assert(m_params.pBuffer != m_buffer, "Streaming-In-Place requires destination and source buffers to be different");
                memcpy(m_params.pBuffer, m_buffer, numBytesRead);
            }
            break;
        case AZ::IO::IStreamerTypes::RequestStatus::Canceled:
            m_IOError = ERROR_USER_ABORT;
            m_numBytesRead = 0;
            m_isError = true;
            break;
        default:
            m_IOError = ERROR_UNKNOWN_ERROR;
            m_numBytesRead = 0;
            m_isError = true;
            break;
        }

        ExecuteAsyncCallback_CBLocked();
        m_isFileRequestComplete = true;

        if (m_params.nFlags & IStreamEngine::FLAGS_NO_SYNC_CALLBACK)
        {
            // We do not need FileRequest here anymore, and not its temporary memory.
            m_fileRequest = nullptr;
            m_isFinished = true;
        }
        else
        {
            //The completion must be triggered from MainThread. (Typically only happens when loading Terrain Macro Textures
            AddRef();
            AZ::SystemTickBus::QueueFunction([this] {
                RequestCompleteOnMainThread();
            });
        }
    }

    m_wait.release();
}


void AZRequestReadStream::RequestCompleteOnMainThread()
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    // call asynchronous callback function if needed synchronously
    {
        CryAutoCriticalSection lock(m_callbackLock);
        ExecuteSyncCallback_CBLocked();
    }

    //Always called because before enqueuing this call was called AddRef()
    Release();
}

