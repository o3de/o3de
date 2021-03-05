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
#include "ChatPlay_precompiled.h"
#include "LibDyad.h"
#include <AzCore/std/parallel/atomic.h>

#include <CryThread.h>
#include <memory>

// Copied dirctly from CryCommon/CrySimpleManagedThread.h because this is the only file that included it.
//
// Adapter that allows us to use CrySimpleThread more safely (lifetime and ownership
// are managed). This implementation holds a shared reference to itself until
// Terminate so that it can ensure Run is called on a valid object.
//
// Users should bind anything they need to keep alive as a shared_pointer to the
// function that will be invoked, or a weak_ptr that they subsequently check before
// use. The function reference is cleared on Terminate freeing any resource handles
// that the thread has.
class CrySimpleManagedThread
    : protected CrySimpleThread < >
{
public:
    // Function type; user should AZStd::bind any necessary arguments
    typedef AZStd::function<void()> function_type;

    // Factory function; creates, names and starts the thread
    static std::shared_ptr<CrySimpleManagedThread> CreateThread(const char* name, const function_type& fn);

    // Waits for the thread to finish executing
    void Join();

protected:
    // Copies are not permitted (this also suppresses implicit move)
    CrySimpleManagedThread(const CrySimpleManagedThread&) = delete;
    void operator=(const CrySimpleManagedThread&) = delete;

    // Initializing constructor
    CrySimpleManagedThread(const function_type& function);

    // Ensures the thread object lives as long as necessary
    std::shared_ptr<CrySimpleManagedThread> m_self;

    // Function to run (user supplied)
    function_type m_function;

    // see CrySimpleThread<>
    void Run() override;
    void Terminate() override;
};

std::shared_ptr<CrySimpleManagedThread> CrySimpleManagedThread::CreateThread(const char* name, const function_type& fn)
{
    // Note: Can't make_shared here because make_shared can't access a protected constructor
    auto t = std::shared_ptr<CrySimpleManagedThread>(new CrySimpleManagedThread(fn));
    t->m_self = t;
    t->SetName(name);
    t->Start();
    return t;
}

void CrySimpleManagedThread::Join()
{
    WaitForThread();
    Stop();
}

CrySimpleManagedThread::CrySimpleManagedThread(const function_type& function)
    : m_function(function)
{
}

void CrySimpleManagedThread::Run()
{
    // Execute user supplied function
    if (m_function)
    {
        m_function();
    }
}

void CrySimpleManagedThread::Terminate()
{
    function_type empty;
    m_function.swap(empty);
    m_self.reset();
}

using namespace dyad;

/******************************************************************************/

namespace dyad
{
    // Arbitrarily selected invalid Id
    const StreamId InvalidStreamId = -1;

    // Data nugget associated with the handler for events on a dyad_Stream that
    // provide context to use in the event and other operations.
    struct DyadStreamContext
    {
        // Id assigned when the stream was created
        StreamId id;

        // Handler for stream events
        EventHandler handler;

        // Pointer to CDyad
        //
        // Note: The pointer is stored "raw" because shared_ptr would cause a
        // circular reference and weak_ptr incurs additional runtime overhead.
        //
        // This pattern is safe because Dyad owns the streams/thread that the
        // context is associated with and is thus guaranteed to exist...
        class CDyad* ptr;

        // Pointer to the stream the context is associated with. This is safe
        // because the context and the stream share the same life cycle.
        dyad_Stream* stream;
    };

    // Implementation of the IDyad interface
    class CDyad
        : public IDyad
    {
    public:

        CDyad();
        ~CDyad() override;

        // see IDyad::CreateStream
        StreamId CreateStream(const EventHandler&, const CreateCallback&) override;

        // see IDyad::PostStreamAction
        void PostStreamAction(StreamId, const StreamAction&) override;

        // see IDyad::CloseStream
        void CloseStream(StreamId) override;

    private:
        // Instances of "Action" are used to control the Dyad thread
        typedef AZStd::function<void()> Action;

        // Counter used to assign unique ids to streams on creation
        static StreamId ms_nextStreamId;

        // This is the thread that will be used for all Dyad operations
        std::shared_ptr<CrySimpleManagedThread> m_thread;

        // Action queue; access must be synchronized with the dyad thread
        AZStd::queue<Action> m_actions;

        // Used to synchronize access to the action queue
        CryCriticalSectionNonRecursive m_actionLock;

        // Map stream ids to context objects that hold stream metadata
        std::unordered_map<StreamId, DyadStreamContext> m_contexts;

        // Run flag used to signal the worker thread
        AZStd::atomic<bool> m_runThread;

        void Update(void);

        // Add an action to the action queue (thread-safe; will run the action on the Dyad thread)
        void PostAction(const Action&);

        // Handle callbacks from Dyad
        static void OnDyadEvent(dyad_Event*);

        void ThreadFunction();
    };

    StreamId CDyad::ms_nextStreamId = InvalidStreamId;
}

/******************************************************************************/
// Implementation of CDyad methods

CDyad::CDyad()
{
    m_runThread = true;
    auto function = AZStd::bind(&CDyad::ThreadFunction, this);
    m_thread = CrySimpleManagedThread::CreateThread("dyad", AZStd::move(function));
}

CDyad::~CDyad()
{
    m_runThread = false;
    m_thread->Join();
}

void CDyad::ThreadFunction()
{
    // Initialize the dyad library
    dyad_init();

    while (m_runThread)
    {
        Update();
    }
    ;

    // Shutdown the dyad library
    dyad_shutdown();
}

void CDyad::Update()
{
    if (dyad_getStreamCount() > 0)
    {
        // Note: Call to select(...) will wait for ~1 second
        dyad_update();
    }
    else
    {
        // Manually sleep for 1 second since there are no streams yet
        CrySleep(1000);
    }

    // Exchange action queue (lock prevents concurrent changes from user threads)
    AZStd::queue<Action> actions;
    m_actionLock.Lock();
    m_actions.swap(actions);
    m_actionLock.Unlock();

    // Execute all actions (making the queue empty again)
    while (!actions.empty())
    {
        if (const Action& action = actions.front())
        {
            action();
        }
        actions.pop();
    }
}

StreamId CDyad::CreateStream(const EventHandler& handler, const CreateCallback& callback)
{
    // We allocate an id before the actual stream is created to give the caller
    // something to use as a handle for later.
    StreamId id = ++ms_nextStreamId;

    // Post the stream creation as an action
    PostAction([=]()
    {
        // Actual stream creation
        dyad_Stream* s = dyad_newStream();
        CRY_ASSERT(s);

        // Create context for future reference
        auto result = m_contexts.emplace(std::make_pair(id, DyadStreamContext { id, handler, this, s }));
        CRY_ASSERT(result.second);
        DyadStreamContext* ctx = &result.first->second;

        // Dyad has no "register all", so we register for every event individually.
        dyad_addListener(s, DYAD_EVENT_DESTROY, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_ACCEPT, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_LISTEN, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_CONNECT, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_CLOSE, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_READY, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_DATA, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_LINE, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_ERROR, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_TIMEOUT, &CDyad::OnDyadEvent, ctx);
        dyad_addListener(s, DYAD_EVENT_TICK, &CDyad::OnDyadEvent, ctx);

        if (callback)
        {
            auto dyadStream = CDyadStream(s, id);
            callback(dyadStream);
        }
    });

    return id;
}

void CDyad::PostAction(const Action& a)
{
    m_actionLock.Lock();
    m_actions.push(a);
    m_actionLock.Unlock();
}

void CDyad::OnDyadEvent(dyad_Event* event)
{
    if (!event || !event->udata)
    {
        return;
    }

    // Extract context reference
    DyadStreamContext* ctx = reinterpret_cast<DyadStreamContext*>(event->udata);
    CRY_ASSERT(ctx); // This handler is only registered with a context so this should never happen

    if (ctx->handler)
    {
        auto dyadEvent = CDyadEvent(event);
        ctx->handler(dyadEvent);
    }

    // Check to see if the stream is being nuked
    if (event->type == DYAD_EVENT_DESTROY)
    {
        ctx->stream = nullptr;
        ctx->ptr->m_contexts.erase(ctx->id);
    }
}

void CDyad::PostStreamAction(StreamId id, const StreamAction& action)
{
    PostAction([=]()
    {
        auto iter = m_contexts.find(id);
        if (iter != m_contexts.end())
        {
            const auto& ctx = iter->second;
            CDyadStream stream(ctx.stream, ctx.id);
            action(stream);
        }
    });
}

void CDyad::CloseStream(StreamId id)
{
    PostStreamAction(id, [](CDyadStream& stream)
    {
        stream.Close();
    });
}

/******************************************************************************/
// CDyadStream methods (could be inlined)

CDyadStream::CDyadStream(dyad_Stream* stream, StreamId id)
    : m_stream(stream)
    , m_id(id)
{
    CRY_ASSERT(m_stream);
}

StreamId CDyadStream::GetId()
{
    return m_id;
}

int CDyadStream::Listen(int port)
{
    return dyad_listen(m_stream, port);
}

int CDyadStream::ListenEx(const char* host, int port, int backlog)
{
    return dyad_listenEx(m_stream, host, port, backlog);
}

bool CDyadStream::Connect(const char* host, int port)
{
    return !dyad_connect(m_stream, host, port);
}

void CDyadStream::Write(const void* data, int size)
{
    dyad_write(m_stream, data, size);
}

void CDyadStream::WriteFormat(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dyad_vwritef(m_stream, fmt, args);
    va_end(args);
}

void CDyadStream::WriteFormat(const char* fmt, va_list args)
{
    dyad_vwritef(m_stream, fmt, args);
}

void CDyadStream::End()
{
    dyad_end(m_stream);
}

void CDyadStream::Close()
{
    dyad_close(m_stream);
}

void CDyadStream::SetTimeout(double seconds)
{
    dyad_setTimeout(m_stream, seconds);
}

void CDyadStream::SetNoDelay(bool state)
{
    dyad_setNoDelay(m_stream, state ? 1 : 0);
}

StreamState CDyadStream::GetState()
{
    return static_cast<StreamState>(dyad_getState(m_stream));
}

const char* CDyadStream::GetAddress()
{
    return dyad_getAddress(m_stream);
}

int CDyadStream::GetPort()
{
    return dyad_getPort(m_stream);
}

int CDyadStream::GetBytesReceived()
{
    return dyad_getBytesReceived(m_stream);
}

int CDyadStream::GetBytesSent()
{
    return dyad_getBytesSent(m_stream);
}

int CDyadStream::GetSocket()
{
    return dyad_getSocket(m_stream);
}

/******************************************************************************/
// CDyadEvent methods (could be inlined)

CDyadEvent::CDyadEvent(const dyad_Event* event)
    : m_event(event)
{
    CRY_ASSERT(m_event);
}

EventType CDyadEvent::GetType()
{
    return static_cast<EventType>(m_event->type);
}

CDyadStream CDyadEvent::GetStream()
{
    DyadStreamContext* ctx = reinterpret_cast<DyadStreamContext*>(m_event->udata);
    CRY_ASSERT(ctx);

    return CDyadStream(m_event->stream, ctx->id);
}

const char* CDyadEvent::GetData()
{
    CRY_ASSERT(GetType() == EventType::Line || GetType() == EventType::Data);
    return m_event->data;
}

std::size_t CDyadEvent::GetDataLength()
{
    CRY_ASSERT(GetType() == EventType::Line || GetType() == EventType::Data);
    CRY_ASSERT(m_event->size >= 0);
    return static_cast<std::size_t>(m_event->size);
}

/******************************************************************************/
// Implementation of IDyad methods

std::shared_ptr<IDyad> IDyad::GetInstance()
{
    // Singleton is stored here as static local variable; this is the only
    // global/static reference held by the system.
    static std::weak_ptr<CDyad> instance;

    // Attempt to lock the current instance or, if that fails, create a new one
    std::shared_ptr<CDyad> ptr = instance.lock();
    if (!ptr)
    {
        ptr = std::make_shared<CDyad>();
        instance = ptr;
    }

    return ptr;
}
