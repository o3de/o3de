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

// Description : An IReadStream implementation designed to work with AZ::IO::Streamer
//               instead of CStreamEngine.

#pragma once

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include "IStreamEngine.h"

namespace AZ
{
    namespace IO
    {
        class Request;
    }
}

//This class is a wrapper of AZ::IO::Request so Cry Classes can use AZ::IO::Streamer.
//Basicallythis replaces CReadStream.
class AZRequestReadStream
    : public IReadStream
{
public:
    AZ_CLASS_ALLOCATOR(AZRequestReadStream, AZ::SystemAllocator, 0);

    static AZRequestReadStream* Allocate(const EStreamTaskType tSource, const char* filename, IStreamCallback* callback,
        const StreamReadParams* params);

    int AddRef() override;
    int Release() override;

    DWORD_PTR GetUserData() override {return m_params.dwUserData; }

    // set user defined data into stream's params
    void SetUserData(DWORD_PTR userData) override { m_params.dwUserData = userData; };

    // returns true if the file read was not successful.
    bool IsError() override { return m_isError; };

    // returns true if the file read was completed (successfully or unsuccessfully)
    // check IsError to check if the whole requested file (piece) was read
    bool IsFinished() override { return m_isFinished; };

    // returns the number of bytes read so far (the whole buffer size if IsFinished())
    unsigned int GetBytesRead([[maybe_unused]] bool bWait) override { return static_cast<unsigned int>(m_numBytesRead); };

    // returns the buffer into which the data has been or will be read
    // at least GetBytesRead() bytes in this buffer are guaranteed to be already read
    const void* GetBuffer() override { return m_buffer; };

    // tries to stop reading the stream; this is advisory and may have no effect
    // but the callback will not be called after this. If you just destructing object,
    // dereference this object and it will automatically abort and release all associated resources.
    void Abort() override;
    bool TryAbort() override;



    // unconditionally waits until the callback is called
    // i.e. if the stream hasn't yet finish, it's guaranteed that the user-supplied callback
    // is called before return from this function (unless no callback was specified)
    void Wait(int maxWaitMillis = -1) override;

    const StreamReadParams& GetParams() const override {return m_params; }

    const EStreamTaskType GetCallerType() const override { return m_Type; }

    //We must define this one. But it is never used in the context of AZ::IO::Streamer.
    //Legacy Cry StreamEngine stuff.
    EStreamSourceMediaType GetMediaType() const override { return EStreamSourceMediaType::eStreamSourceTypeUnknown; }

    // return pointer to callback routine(can be NULL)
    IStreamCallback* GetCallback() const override { return m_callback; };

    // return IO error #
    unsigned GetError() const override { return m_IOError; };

    //   Returns IO error name
    const char* GetErrorName() const override;

    // return stream name
    const char* GetName() const override { return m_fileName.c_str(); };

    void FreeTemporaryMemory() override;

    // tries to raise the priority of the read; this is advisory and may have no effect
    void SetPriority(EStreamTaskPriority EPriority);
    uint64 GetPriority() const { return m_params.ePriority; };

    void* GetFileReadBuffer() { return m_buffer; } //GetBuffer from IReadStream is "const void *"
    AZStd::size_t GetFileSize() { return m_fileSize; }

    void SetFileRequest(AZ::IO::FileRequestPtr request) { m_fileRequest = AZStd::move(request); }
    AZ::IO::FileRequestPtr GetFileRequest() { return m_fileRequest; }

    void OnRequestComplete(AZ::IO::SizeType numBytesRead, void* buffer, AZ::IO::IStreamerTypes::RequestStatus requestState);

private:
    AZRequestReadStream();
    virtual ~AZRequestReadStream();

    // call the async callback
    void ExecuteAsyncCallback_CBLocked();
    void ExecuteSyncCallback_CBLocked();
    void RequestCompleteOnMainThread();

    AZStd::atomic_int m_refCount;

    CryCriticalSection m_callbackLock;
    StreamReadParams m_params;
    AZStd::semaphore m_wait;

    CryStringLocal m_fileName;
    AZ::IO::FileRequestPtr m_fileRequest;

    // Bytes actually read from media.
    void* m_buffer;

    // the type of the task
    EStreamTaskType m_Type;
    // the initial data from the user
    // the callback; may be NULL
    IStreamCallback* m_callback;

    AZ::IO::SizeType m_fileSize; //Expected number of bytes to be read.
    AZ::IO::SizeType m_numBytesRead; //On a successful read m_nBytesRead == m_fileSize;

    bool m_isAsyncCallbackExecuted;
    bool m_isSyncCallbackExecuted;
    bool m_isFileRequestComplete;

    bool m_isError;
    bool m_isFinished;
    unsigned int m_IOError;
};

TYPEDEF_AUTOPTR(AZRequestReadStream);
