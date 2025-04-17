/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceReflection.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Component/TickBus.h>

namespace AZ::Debug
{
    //! Trace Message Event Handler for Automation.
    //! Since TraceMessageBus will be called from multiple threads and
    //! python interpreter is single threaded, all the bus calls are
    //! queued into a list and called at the end of the frame in the main thread.
    //! @note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER
    //! macro as the signature needs to be changed to connect to Tick bus.
    class TraceMessageBusHandler
        : public AZ::Debug::TraceMessageBus::Handler
        , public AZ::BehaviorEBusHandler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(TraceMessageBusHandler, AZ::SystemAllocator);
        AZ_RTTI(TraceMessageBusHandler, "{5CDBAF09-5EB0-48AC-B327-2AF8601BB550}", AZ::BehaviorEBusHandler);

        TraceMessageBusHandler();

        using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<
            decltype(&TraceMessageBusHandler::OnPreAssert),
            decltype(&TraceMessageBusHandler::OnPreError),
            decltype(&TraceMessageBusHandler::OnPreWarning),
            decltype(&TraceMessageBusHandler::OnAssert),
            decltype(&TraceMessageBusHandler::OnError),
            decltype(&TraceMessageBusHandler::OnWarning),
            decltype(&TraceMessageBusHandler::OnException),
            decltype(&TraceMessageBusHandler::OnPrintf),
            decltype(&TraceMessageBusHandler::OnOutput)
        >;

        enum
        {
            FN_OnPreAssert = 0,
            FN_OnPreError,
            FN_OnPreWarning,
            FN_OnAssert,
            FN_OnError,
            FN_OnWarning,
            FN_OnException,
            FN_OnPrintf,
            FN_OnOutput,
            FN_MAX
        };

        static inline constexpr const char* m_functionNames[FN_MAX] =
        {
            "OnPreAssert",
            "OnPreError",
            "OnPreWarning",
            "OnAssert",
            "OnError",
            "OnWarning",
            "OnException",
            "OnPrintf",
            "OnOutput"
        };

        // AZ::BehaviorEBusHandler overrides...
        int GetFunctionIndex(const char* functionName) const override;
        void Disconnect(AZ::BehaviorArgument* id = nullptr) override;
        bool Connect(AZ::BehaviorArgument* id = nullptr) override;
        bool IsConnected() override;
        bool IsConnectedId(AZ::BehaviorArgument* id) override;

        // TraceMessageBus
        /*
         *  Note: Since at editor runtime there is already have a handler, for automation (OnPreAssert, OnPreWarning, OnPreWarning)
         *  must be used instead of (OnAssert, OnWarning, OnError)
        */
        bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnAssert(const char* message) override;
        bool OnError(const char* window, const char* message) override;
        bool OnWarning(const char* window, const char* message) override;
        bool OnException(const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
        bool OnOutput(const char* window, const char* message) override;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        void QueueMessageCall(AZStd::function<void()> messageCall);
        void FlushMessageCalls();

        AZStd::list<AZStd::function<void()>> m_messageCalls;
        AZStd::mutex m_messageCallsLock;
    };

    TraceMessageBusHandler::TraceMessageBusHandler()
    {
        m_events.resize(FN_MAX);

        SetEvent(&TraceMessageBusHandler::OnPreAssert, m_functionNames[FN_OnPreAssert]);
        SetEvent(&TraceMessageBusHandler::OnPreError, m_functionNames[FN_OnPreError]);
        SetEvent(&TraceMessageBusHandler::OnPreWarning, m_functionNames[FN_OnPreWarning]);
        SetEvent(&TraceMessageBusHandler::OnAssert, m_functionNames[FN_OnAssert]);
        SetEvent(&TraceMessageBusHandler::OnError, m_functionNames[FN_OnError]);
        SetEvent(&TraceMessageBusHandler::OnWarning, m_functionNames[FN_OnWarning]);
        SetEvent(&TraceMessageBusHandler::OnException, m_functionNames[FN_OnException]);
        SetEvent(&TraceMessageBusHandler::OnPrintf, m_functionNames[FN_OnPrintf]);
        SetEvent(&TraceMessageBusHandler::OnOutput, m_functionNames[FN_OnOutput]);
    }

    int TraceMessageBusHandler::GetFunctionIndex(const char* functionName) const
    {
        for (int i = 0; i < FN_MAX; ++i)
        {
            if (azstricmp(functionName, m_functionNames[i]) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    void TraceMessageBusHandler::Disconnect(AZ::BehaviorArgument* id)
    {
        AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::Disconnect(this, id);
        AZ::TickBus::Handler::BusDisconnect();
    }

    bool TraceMessageBusHandler::Connect(AZ::BehaviorArgument* id)
    {
        AZ::TickBus::Handler::BusConnect();
        return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::Connect(this, id);
    }

    bool TraceMessageBusHandler::IsConnected()
    {
        return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::IsConnected(this);
    }

    bool TraceMessageBusHandler::IsConnectedId(AZ::BehaviorArgument* id)
    {
        return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::IsConnectedId(this, id);
    }

    //////////////////////////////////////////////////////////////////////////
    // TraceMessageBusHandler Implementation
    inline bool TraceMessageBusHandler::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
    {
        QueueMessageCall(
            [this, fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
        {
            Call(FN_OnPreAssert, fileNameString.c_str(), line, funcString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
        {
            Call(FN_OnPreError, windowString.c_str(), fileNameString.c_str(), line, funcString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
        {
            return Call(FN_OnPreWarning, windowString.c_str(), fileNameString.c_str(), line, funcString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnAssert(const char* message)
    {
        QueueMessageCall(
            [this, messageString = AZStd::string(message)]()
        {
            return Call(FN_OnAssert, messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnError(const char* window, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
        {
            return Call(FN_OnError, windowString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnWarning(const char* window, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
        {
            return Call(FN_OnWarning, windowString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnException(const char* message)
    {
        QueueMessageCall(
            [this, messageString = AZStd::string(message)]()
        {
            return Call(FN_OnException, messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnPrintf(const char* window, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
        {
            return Call(FN_OnPrintf, windowString.c_str(), messageString.c_str());
        });
        return false;
    }

    inline bool TraceMessageBusHandler::OnOutput(const char* window, const char* message)
    {
        QueueMessageCall(
            [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
        {
            return Call(FN_OnOutput, windowString.c_str(), messageString.c_str());
        });
        return false;
    }

    void TraceMessageBusHandler::OnTick(
        [[maybe_unused]] float deltaTime,
        [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        FlushMessageCalls();
    }

    int TraceMessageBusHandler::GetTickOrder()
    {
        return AZ::TICK_LAST;
    }

    void TraceMessageBusHandler::QueueMessageCall(AZStd::function<void()> messageCall)
    {
        AZStd::lock_guard<decltype(m_messageCallsLock)> lock(m_messageCallsLock);
        m_messageCalls.emplace_back(messageCall);
    }

    void TraceMessageBusHandler::FlushMessageCalls()
    {
        AZStd::list<AZStd::function<void()>> messageCalls;
        {
            AZStd::lock_guard<decltype(m_messageCallsLock)> lock(m_messageCallsLock);
            m_messageCalls.swap(messageCalls); // Move calls to a new list to release the lock as soon as possible
        }

        for (auto& messageCall : messageCalls)
        {
            messageCall();
        }
    }

    void TraceReflect(ReflectContext* context)
    {
        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<AZ::Debug::TraceMessageBus>("TraceMessageBus")
                ->Attribute(AZ::Script::Attributes::Module, "debug")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Handler<TraceMessageBusHandler>()
                ;
        }
    }
} // namespace AZ::Debug
