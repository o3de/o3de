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

/*
* C++ abstraction for accessing Dyad functionality.
* Dyad is an IRC client library written in C.
*
* Note: Dyad uses global state, so only one instance of the underlying library
* can be active in a single process. If you intend to initialize Dyad yourself
* then it will not be safe to use this abstraction layer at the same time.
*
* All Dyad events and I/O are handled on their own thread. This allows Dyad to
* be used asynchronously but requires that users read and understand the comments
* about the threading model to avoid runtime issues with their event handlers.
*
* The lifetimes of all objects in this abstraction layer are managed. The user
* should not store references to stream or event objects outside the scope in
* which they occur, or bad things will happen.
*/
#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_LIBDYAD_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_LIBDYAD_H
#pragma once

#include "dyad.h"

namespace dyad
{
    // Enumeration of possible event types
    // For full descriptions of the events please refer to the Dyad documentation.
    enum class EventType
    {
        Destroy = DYAD_EVENT_DESTROY, // Stream is being destroyed; after this the stream simply doesn't exist
        Accept = DYAD_EVENT_ACCEPT, // TODO: Support not fully implemented
        Listen = DYAD_EVENT_LISTEN, // TODO: Support not fully implemented
        Connect = DYAD_EVENT_CONNECT, // Stream has connected to a remote host
        Close = DYAD_EVENT_CLOSE, // Stream is being closed; you can't stop it
        Ready = DYAD_EVENT_READY, // Stream is ready to send more data
        Data = DYAD_EVENT_DATA, // Data events contain the raw unterminated bytes received by Dyad
        Line = DYAD_EVENT_LINE, // Line events are delimited and terminated based on line breaks
        Error = DYAD_EVENT_ERROR, // After an Error event Dyad always closes the stream
        Timeout = DYAD_EVENT_TIMEOUT, // After a Timeout event Dyad always closes the stream
        Tick = DYAD_EVENT_TICK // Periodic update
    };

    // Enumeration of possible stream states.
    // For full descriptions of the states please refer to the Dyad documentation.
    enum class StreamState
    {
        Closed = DYAD_STATE_CLOSED,
        Closing = DYAD_STATE_CLOSING,
        Connecting = DYAD_STATE_CONNECTING,
        Connected = DYAD_STATE_CONNECTED,
        Listening = DYAD_STATE_LISTENING // TODO: Support not fully implemented
    };

    // Used to uniquely identify streams.
    //
    // The uniqueness applies at the process-level; i.e. the same id will not be
    // used more than once in a single process run.
    typedef int StreamId;

    // Constant that the implementation guarantees will not be used as a stream id
    extern const StreamId InvalidStreamId;

    // Wrapper for dyad_Stream
    //
    // Note: Dyad controls the lifetime of the underlying object and may delete
    // it at any time on the Dyad thread; this means it is only safe to use a
    // wrapped stream from the Dyad thread.
    class CDyadStream
    {
    public:
        // Does not accept null; must be constructed with a valid stream
        CDyadStream(dyad_Stream*, StreamId);

        // The unique id for this stream
        StreamId GetId();

        // C++ wrappers for Dyad API functions on streams
        // For full descriptions of the methods please refer to the Dyad documentation.

        int Listen(int port); // TODO: Support not fully implemented
        int ListenEx(const char* host, int port, int backlog); // TODO: Support not fully implemented
        bool Connect(const char* host, int port);
        void Write(const void* data, int size);
        void WriteFormat(const char* fmt, ...);
        void WriteFormat(const char* fmt, va_list args);
        void End();
        void Close();
        void SetTimeout(double seconds);
        void SetNoDelay(bool state);
        StreamState GetState();
        const char* GetAddress();
        int GetPort();
        int GetBytesReceived();
        int GetBytesSent();
        int GetSocket();

    private:
        CDyadStream() = delete;

        dyad_Stream* m_stream;
        StreamId m_id;
    };

    // Wrapper for dyad_Event
    //
    // Note: Dyad controls the lifetime of the underlying object; wrapped events
    // are only safe to use from the Dyad thread during event callbacks.
    class CDyadEvent
    {
    public:
        // Does not accept null; must be constructed with a valid event
        explicit CDyadEvent(const dyad_Event*);

        // C++ wrappers for access to the dyad_Event structure (partial)
        // For full descriptions of the methods please refer to the Dyad documentation.

        EventType GetType();
        CDyadStream GetStream();
        const char* GetData();
        std::size_t GetDataLength();

    private:
        CDyadEvent() = delete;
        const dyad_Event* m_event;
    };

    // The event handler allow the user to customize their response to dyad events.
    // It is registered when creating a new stream. (See CreateStream)
    //
    // Lifetime of the Dyad Stream: It is guaranteed to exist for the scope of
    // handler and no longer, so it is important that references to it are not
    // stored by user code.
    //
    // Threading: Handlers are invoked on the Dyad thread. Handlers that block
    // or otherwise interrupt processing will disrupt all Dyad operations.
    //
    typedef AZStd::function<void(CDyadEvent&)> EventHandler;

    class IDyad
    {
    public:
        // Access the singleton; releasing all instances of this pointer will cause
        // Dyad to shutdown. Note that some callbacks may also bind the singleton,
        // so destruction may not be immediate.
        static std::shared_ptr<IDyad> GetInstance();

        // Enforce virtualization on this interface
        virtual ~IDyad() = 0;

        // Called after a stream is created by CreateStream
        //
        // Lifetime: The stream lifetime is controlled by the Dyad library; it will
        // automatically destroy closed streams when it finds them. It is important
        // that the user code does not store any references to the stream object
        // directly since this destruction can happen asynchronously at any time.
        //
        // NOTE: New stream will be in the closed state on creation; it is important
        // to connect to something immediately or the stream will be destroyed by
        // dyad on the next update.
        //
        typedef AZStd::function<void(CDyadStream&)> CreateCallback;

        // Create a new dyad stream.
        //
        // User must supply an event handler and a callback for the creation event.
        //
        // Threading: The callback and handler will be invoked on the Dyad thread
        // and must not block or otherwise interrupt processing. CreateStream may
        // not be called by more than one thread at a time.
        //
        // The return value is the id of the stream; it can be used to close the
        // stream at a later point.
        virtual StreamId CreateStream(const EventHandler& handler, const CreateCallback& callback) = 0;

        // Action to perform on a stream
        typedef AZStd::function<void(CDyadStream&)> StreamAction;

        // Perform an action on a stream (posts to the Dyad thread)
        // Note: Will not be invoked if the stream no longer exists
        virtual void PostStreamAction(StreamId id, const StreamAction& action) = 0;

        // Closes a stream
        //
        // Note: This is an asynchronous request; the actual closure happens on
        // the Dyad thread. Callers are responsible for synchronizing against
        // the destruction events if they need to.
        virtual void CloseStream(StreamId id) = 0;
    };

    inline IDyad::~IDyad() {}
}

#endif // CRYINCLUDE_CRYACTION_CHATPLAY_LIBDYAD_H
