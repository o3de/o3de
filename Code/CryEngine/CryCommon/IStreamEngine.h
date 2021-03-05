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

// This is the prototypes of interfaces that will be used for asynchronous
// I/O (streaming).
// THIS IS NOT FINAL AND IS SUBJECT TO CHANGE WITHOUT NOTICE

// Some excerpts explaining basic ideas behind streaming design here:

/*
 * The idea is that the data loaded is ready for usage and ideally doesn't need further transformation,
 * therefore the client allocates the buffer (to avoid extra copy). All the data transformations should take place in the Resource Compiler. If you have to allocate a lot of small memory objects, you should revise this strategy in favor of one big allocation (again, that will be read directly from the compiled file).
 * Anyway, we can negotiate that the streaming engine allocates this memory.
 * In the end, it could make use of a memory pool, and copying data is not the bottleneck in our engine
 *
 * The client should take care of all fast operations. Looking up file size should be fast on the virtual
 * file system in a pak file, because the directory should be preloaded in memory
 */

#ifndef CRYINCLUDE_CRYCOMMON_ISTREAMENGINE_H
#define CRYINCLUDE_CRYCOMMON_ISTREAMENGINE_H
#pragma once


#include <list>
#include "smartptr.h"
#include <IThreadTask.h> // <> required for Interfuscator
#include "CryThread.h"

#include "IStreamEngineDefs.h"

class IStreamCallback;
class ICrySizer;

#define STREAM_TASK_TYPE_AUDIO_ALL ((1 << eStreamTaskTypeMusic) | (1 << eStreamTaskTypeSound) | (1 << eStreamTaskTypeFSBCache))

// Description:
//   This is used as parameter to the asynchronous read function
//   all the unnecessary parameters go here, because there are many of them.
struct StreamReadParams
{
public:
    StreamReadParams()
    {
        memset(this, 0, sizeof(*this));
        ePriority = estpNormal;
    }

    StreamReadParams (
        DWORD_PTR _dwUserData,
        EStreamTaskPriority _ePriority = estpNormal,
        unsigned _nLoadTime = 0,
        unsigned _nMaxLoadTime = 0,
        unsigned _nOffset = 0,
        unsigned _nSize = 0,
        void* _pBuffer = NULL,
        unsigned _nFlags = 0
        )
        :  dwUserData (_dwUserData)
        , ePriority(_ePriority)
        , nPerceptualImportance(0)
        , nLoadTime(_nLoadTime)
        , nMaxLoadTime(_nMaxLoadTime)
        , pBuffer (_pBuffer)
        , nOffset (_nOffset)
        , nSize (_nSize)
        , eMediaType(eStreamSourceTypeUnknown)
        , nFlags (_nFlags)
    {
    }

    // Summary:
    //   File name.
    //const char* szFile;

    // Summary:
    //   The callback.
    //IStreamCallback* pAsyncCallback;

    // Summary:
    //   The user data that'll be used to call the callback.
    DWORD_PTR dwUserData;

    // The priority of this read
    EStreamTaskPriority ePriority;

    // Value from 0-255 of the perceptual importance of the task (used for debugging task sheduling)
    uint8 nPerceptualImportance;

    // Description:
    //   The desirable loading time, in milliseconds, from the time of call
    //   0 means as fast as possible (desirably in this frame).
    unsigned nLoadTime;

    // Description:
    //   The maximum load time, in milliseconds. 0 means forever. If the read lasts longer, it can be discarded.
    //   WARNING: avoid too small max times, like 1-10 ms, because many loads will be discarded in this case.
    unsigned nMaxLoadTime;

    // Description:
    //   The buffer into which to read the file or the file piece
    //   if this is NULL, the streaming engine will supply the buffer.
    // Notes:
    //   DO NOT USE THIS BUFFER during read operation! DO NOT READ from it, it can lead to memory corruption!
    void* pBuffer;

    // Description:
    //   Offset in the file to read; if this is not 0, then the file read
    //   occurs beginning with the specified offset in bytes.
    //   The callback interface receives the size of already read data as nSize
    //   and generally behaves as if the piece of file would be a file of its own.
    unsigned nOffset;

    // Description:
    //   Number of bytes to read; if this is 0, then the whole file is read,
    //   if nSize == 0 && nOffset != 0, then the file from the offset to the end is read.
    //   If nSize != 0, then the file piece from nOffset is read, at most nSize bytes
    //   (if less, an error is reported). So, from nOffset byte to nOffset + nSize - 1 byte in the file.
    unsigned nSize;

    // Description:
    //   Media type to use when starting file request - if wrong, the request may take longer to complete
    EStreamSourceMediaType eMediaType;

    // Description:
    //   The combination of one or several flags from the stream engine general purpose flags.
    // See also:
    //   IStreamEngine::EFlags
    unsigned nFlags;
};

struct StreamReadBatchParams
{
    StreamReadBatchParams()
        : tSource((EStreamTaskType)0)
        , szFile(NULL)
        , pCallback(NULL)
    {
    }

    EStreamTaskType tSource;
    const char* szFile;
    IStreamCallback* pCallback;
    StreamReadParams params;
};

struct IStreamEngineListener
{
    // <interfuscator:shuffle>
    virtual ~IStreamEngineListener() {}

    virtual void OnStreamEnqueue(const void* pReq, const char* filename, EStreamTaskType source, const StreamReadParams& readParams) = 0;
    virtual void OnStreamComputedSortKey(const void* pReq, uint64 key) = 0;
    virtual void OnStreamBeginIO(const void* pReq, uint32 compressSize, uint32 readSize, EStreamSourceMediaType mediaType) = 0;
    virtual void OnStreamEndIO(const void* pReq) = 0;
    virtual void OnStreamBeginInflate(const void* pReq) = 0;
    virtual void OnStreamEndInflate(const void* pReq) = 0;
    virtual void OnStreamBeginDecrypt(const void* pReq) = 0;
    virtual void OnStreamEndDecrypt(const void* pReq) = 0;
    virtual void OnStreamBeginAsyncCallback(const void* pReq) = 0;
    virtual void OnStreamEndAsyncCallback(const void* pReq) = 0;
    virtual void OnStreamDone(const void* pReq) = 0;
    virtual void OnStreamPreempted(const void* pReq) = 0;
    virtual void OnStreamResumed(const void* pReq) = 0;
    // </interfuscator:shuffle>
};

// Description:
//   The highest level. There is only one StreamingEngine in the application
//   and it controls all I/O streams.
struct IStreamEngine
{
public:


    enum EJobType
    {
        ejtStarted = 1 << 0,
        ejtPending = 1 << 1,
        ejtFinished = 1 << 2,
    };

    // Summary:
    //   General purpose flags.
    enum EFlags
    {
        // Description:
        //   If this is set only asynchronous callback will be called.
        FLAGS_NO_SYNC_CALLBACK = BIT(0),
        // Description:
        //   If this is set the file will be read from disc directly, instead of from the pak system.
        FLAGS_FILE_ON_DISK = BIT(1),
        // Description:
        //   Ignore the tmp out of streaming memory for this request
        FLAGS_IGNORE_TMP_OUT_OF_MEM = BIT(2),
        // Description:
        //   External buffer is write only
        FLAGS_WRITE_ONLY_EXTERNAL_BUFFER = BIT(3),
    };

    // <interfuscator:shuffle>
    // Description:
    //   Starts asynchronous read from the specified file (the file may be on a
    //   virtual file system, in pak or zip file or wherever).
    //   Reads the file contents into the given buffer, up to the given size.
    //   Upon success, calls success callback. If the file is truncated or for other
    //   reason can not be read, calls error callback. The callback can be NULL (in this case, the client should poll
    //   the returned IReadStream object; the returned object must be locked for that)
    // NOTE: the error/success/ progress callbacks can also be called from INSIDE this function.
    // Arguments:
    //   tSource    -
    //   szFile     -
    //   pCallback  -
    //   pParams    - PLACEHOLDER for the future additional parameters (like priority), or really
    //                a pointer to a structure that will hold the parameters if there are too many of them.
    // Return Value:
    //   IReadStream is reference-counted and will be automatically deleted if you don't refer to it;
    //   if you don't store it immediately in an auto-pointer, it may be deleted as soon as on the next line of code,
    //   because the read operation may complete immediately inside StartRead() and the object is self-disposed
    //   as soon as the callback is called.
    // Remarks:
    //   In some implementations disposal of the old pointers happen synchronously
    //   (in the main thread) outside StartRead() (it happens in the entity update),
    //   so you're guaranteed that it won't trash inside the calling function. However, this may change in the future
    //   and you'll be required to assign it to IReadStream immediately (StartRead will return IReadStream_AutoPtr then).
    // See also:
    //   IReadStream,IReadStream_AutoPtr
    virtual IReadStreamPtr StartRead (const EStreamTaskType tSource, const char* szFile, IStreamCallback* pCallback = NULL, const StreamReadParams* pParams = NULL) = 0;

    // Pass a callback to preRequestCallback if you need to execute code right before the requests get enqueued; the callback is called only once per execution
    virtual size_t StartBatchRead(IReadStreamPtr* pStreamsOut, const StreamReadBatchParams* pReqs, size_t numReqs, AZStd::function<void ()>* preRequestCallback = nullptr) = 0;

    // Call this methods before/after submitting large number of new requests.
    virtual void BeginReadGroup() = 0;
    virtual void EndReadGroup() = 0;

    // Pause/resumes streaming of specific data types.
    // nPauseTypesBitmask is a bit mask of data types (ex, 1<<eStreamTaskTypeGeometry)
    virtual void PauseStreaming(bool bPause, uint32 nPauseTypesBitmask) = 0;

    // Get pause bit mask
    virtual uint32 GetPauseMask() const = 0;

    // Pause/resumes any IO active from the streaming engine
    virtual void PauseIO(bool bPause) = 0;

    // Description:
    //   Is the streaming data available on harddisc for fast streaming
    virtual bool IsStreamDataOnHDD() const = 0;

    // Description:
    //   Inform streaming engine that the streaming data is available on HDD
    virtual void SetStreamDataOnHDD(bool bFlag) = 0;

    // Description:
    //   Per frame update ofthe streaming engine, synchronous events are dispatched from this function.
    virtual void Update() = 0;

    // Description:
    //   Per frame update of the streaming engine, synchronous events are dispatched from this function, by particular TypesBitmask.
    virtual void Update(uint32 nUpdateTypesBitmask) = 0;

    // Description:
    //   Waits until all submitted requests are complete. (can abort all reads which are currently in flight)
    virtual void UpdateAndWait(bool bAbortAll = false) = 0;

    // Description:
    //   Puts the memory statistics into the given sizer object.
    //   According to the specifications in interface ICrySizer.
    // See also:
    //   ICrySizer
    virtual void GetMemoryStatistics(ICrySizer* pSizer) = 0;

#if defined(STREAMENGINE_ENABLE_STATS)
    // Description:
    //   Returns the streaming statistics collected from the previous call.
    virtual SStreamEngineStatistics& GetStreamingStatistics() = 0;
    virtual void ClearStatistics() = 0;

    // Description:
    //   returns the bandwidth used for the given type of streaming task
    virtual void GetBandwidthStats(EStreamTaskType type, float* bandwidth) = 0;
#endif

    // Description:
    //   Returns the counts of open streaming requests.
    virtual void GetStreamingOpenStatistics(SStreamEngineOpenStats& openStatsOut) = 0;

    virtual const char* GetStreamTaskTypeName(EStreamTaskType type) = 0;

#if defined(STREAMENGINE_ENABLE_LISTENER)
    // Description:
    //   Sets up a listener for stream events (used for statoscope)
    virtual void SetListener(IStreamEngineListener* pListener) = 0;
    virtual IStreamEngineListener* GetListener() = 0;
#endif

    virtual ~IStreamEngine() {}
    // </interfuscator:shuffle>
};

// Description:
//   This is the file "handle" that can be used to query the status
//   of the asynchronous operation on the file. The same object may be returned
//   for the same file to multiple clients.
// Notes:
//   It will actually represent the asynchronous object in memory, and will be
//   thread-safe reference-counted (both AddRef() and Release() will be virtual
//   and thread-safe, just like the others)
// Example:
//   USE:
//       IReadStream_AutoPtr pReadStream = pStreamEngine->StartRead ("bla.xxx", this);
//   OR:
//       pStreamEngine->StartRead ("MusicSystem","bla.xxx", this);
class IReadStream
{
public:
    // <interfuscator:shuffle>
    // Summary:
    //   Increment ref count, returns new count
    virtual int AddRef() = 0;
    // Summary:
    //   Decrement ref count, returns new count
    virtual int Release() = 0;
    // Summary:
    //   Returns true if the file read was not successful.
    virtual bool IsError() = 0;
    // Return Value:
    //   True if the file read was completed successfully.
    // Summary:
    //   Checks IsError to check if the whole requested file (piece) was read.
    virtual bool IsFinished() = 0;
    // Description:
    //   Returns the number of bytes read so far (the whole buffer size if IsFinished())
    // Arguments:
    //   bWait - if == true, then waits until the pending I/O operation completes.
    // Return Value:
    //   The total number of bytes read (if it completes successfully, returns the size of block being read)
    virtual unsigned int GetBytesRead(bool bWait = false) = 0;
    // Description:
    //   Returns the buffer into which the data has been or will be read
    //   at least GetBytesRead() bytes in this buffer are guaranteed to be already read.
    // Notes:
    //   DO NOT USE THIS BUFFER during read operation! DO NOT READ from it, it can lead to memory corruption!
    virtual const void* GetBuffer () = 0;

    // Description:
    //   Returns the transparent DWORD that was passed in the StreamReadParams::dwUserData field
    //   of the structure passed in the call to IStreamEngine::StartRead.
    // See also:
    //   StreamReadParams::dwUserData,IStreamEngine::StartRead
    virtual DWORD_PTR GetUserData() = 0;

    // Summary:
    //   Set user defined data into stream's params.
    virtual void SetUserData(DWORD_PTR dwUserData) = 0;

    // Description:
    //   Tries to stop reading the stream; this is advisory and may have no effect
    //   but the callback will not be called after this. If you just destructing object,
    //   dereference this object and it will automatically abort and release all associated resources.
    virtual void Abort() = 0;

    // Description:
    //   Tries to stop reading the stream, as long as IO or the async callback is not currently
    //   in progress.
    virtual bool TryAbort() = 0;

    // Summary:
    //   Unconditionally waits until the callback is called.
    //   if nMaxWaitMillis is not negative wait for the specified ammount of milliseconds then exit.
    // Example:
    //   If the stream hasn't yet finish, it's guaranteed that the user-supplied callback
    //   is called before return from this function (unless no callback was specified).
    virtual void Wait(int nMaxWaitMillis = -1) = 0;

    // Summary:
    //   Returns stream params.
    virtual const StreamReadParams& GetParams() const = 0;

    // Summary:
    //   Returns caller type.
    virtual const EStreamTaskType GetCallerType() const = 0;

    // Summary:
    //   Returns media type used to satisfy request - only valid once stream has begun read.
    virtual EStreamSourceMediaType GetMediaType() const = 0;

    // Summary:
    //   Returns pointer to callback routine(can be NULL).
    virtual IStreamCallback* GetCallback() const = 0;

    // Summary:
    //   Returns IO error #.
    virtual unsigned GetError() const = 0;

    // Summary:
    //   Returns IO error name
    virtual const char* GetErrorName() const = 0;

    // Summary:
    //   Returns stream name.
    virtual const char* GetName() const = 0;

    // Summary:
    //   Free temporary memory allocated for this stream, when not needed anymore.
    //   Can be called from Async callback, to free memory earlier, not waiting for synchrounus callback.
    virtual void FreeTemporaryMemory() = 0;
    // </interfuscator:shuffle>

protected:
    // Summary:
    //   The clients are not allowed to destroy this object directly; only via Release().
    virtual ~IReadStream() {}
};

TYPEDEF_AUTOPTR(IReadStream);

// Description:
//   CryPak supports asynchronous reading through this interface. The callback
//   is called from the main thread in the frame update loop.
//
//   The callback receives packets through StreamOnComplete() and
//   StreamOnProgress(). The second one can be used to update the asset based
//   on the partial data that arrived. the callback that will be called by the
//   streaming engine must be implemented by all clients that want to use
//   StreamingEngine services
// Remarks:
//   the pStream interface is guaranteed to be locked (have reference count > 0)
//   while inside the function, but can vanish any time outside the function.
//   If you need it, keep it from the beginning (after call to StartRead())
//   some or all callbacks MAY be called from inside IStreamEngine::StartRead()
//
// Example:
//   <code>
//   IStreamEngine *pStreamEngine = g_pISystem->GetStreamEngine();   // get streaming engine
//   IStreamCallback *pAsyncCallback = &MyClass;                                    // user
//
//   StreamReadParams params;
//
//   params.dwUserData = 0;
//   params.nSize = 0;
//   params.pBuffer = NULL;
//   params.nLoadTime = 10000;
//   params.nMaxLoadTime = 10000;
//
//   pStreamEngine->StartRead(  .. pAsyncCallback .. params .. );            // registers callback
//   </code>
class IStreamCallback
{
public:
    // <interfuscator:shuffle>
    virtual ~IStreamCallback(){}

    // Description:
    //   Signals that the file length for the request has been found, and that storage is needed
    //   Either a pointer to a block of nSize bytes can be returned, into which the file will be
    //   streamed, or NULL can be returned, in which case temporary memory will be allocated
    //   internally by the stream engine (which will be freed upon job completion).
    virtual void* StreamOnNeedStorage ([[maybe_unused]] IReadStream* pStream, [[maybe_unused]] unsigned nSize, [[maybe_unused]] bool& bAbortOnFailToAlloc) {return NULL; }

    // Description:
    //   Signals that reading the requested data has completed (with or without error).
    //   This callback is always called, whether an error occurs or not.
    //   pStream will signal either IsFinished() or IsError() and will hold the (perhaps partially) read data until this interface is released.
    //   GetBytesRead() will return the size of the file (the completely read buffer) in case of successful operation end
    //   or the size of partially read data in case of error (0 if nothing was read).
    //   Pending status is true during this callback, because the callback itself is the part of IO operation.
    //   nError == 0 : Success
    //   nError != 0 : Error code
    virtual void StreamAsyncOnComplete ([[maybe_unused]] IReadStream* pStream, [[maybe_unused]] unsigned nError) {}

    // Description:
    //   Signals that reading the requested data has completed (with or without error).
    //   This callback is always called, whether an error occurs or not.
    //   pStream will signal either IsFinished() or IsError() and will hold the (perhaps partially) read data until this interface is released.
    //   GetBytesRead() will return the size of the file (the completely read buffer) in case of successful operation end
    //   or the size of partially read data in case of error (0 if nothing was read).
    //   Pending status is true during this callback, because the callback itself is the part of IO operation.
    //   nError == 0 : Success
    //   nError != 0 : Error code
    virtual void StreamOnComplete ([[maybe_unused]] IReadStream* pStream, [[maybe_unused]] unsigned nError) {}
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ISTREAMENGINE_H
