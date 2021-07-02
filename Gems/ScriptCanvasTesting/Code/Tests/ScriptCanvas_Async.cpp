/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

#include <ScriptCanvas/Core/SlotConfigurationDefaults.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvasEditor;

// Asynchronous ScriptCanvas Behaviors

#if AZ_COMPILER_MSVC

#include <future>

class AsyncEvent : public AZ::EBusTraits
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::EntityId;
    using MutexType = AZStd::recursive_mutex;
    static const bool LocklessDispatch = true;

    //////////////////////////////////////////////////////////////////////////

    virtual void OnAsyncEvent() = 0;
};

using AsyncEventNotificationBus = AZ::EBus<AsyncEvent>;

class LongRunningProcessSimulator3000
{
public:

    static void Run(const AZ::EntityId& listener)
    {
        int duration = 40;
        while (--duration > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }

        AsyncEventNotificationBus::Event(listener, &AsyncEvent::OnAsyncEvent);
    }
};

class AsyncNode
    : public ScriptCanvas::Node
    , protected AsyncEventNotificationBus::Handler
    , protected AZ::TickBus::Handler
{
public:
    AZ_COMPONENT(AsyncNode, "{0A7FF6C6-878B-42EC-A8BB-4D29C4039853}", ScriptCanvas::Node);

    bool IsEntryPoint() const { return true; }

    AsyncNode()
        : Node()
    {}

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AsyncNode, Node>()
                ->Version(1)
                ;
        }
    }

    void ConfigureSlots() override
    {
        AddSlot(ScriptCanvas::CommonSlots::GeneralInSlot());
        AddSlot(ScriptCanvas::CommonSlots::GeneralOutSlot());
    }

    void OnActivate() override
    {
        ScriptCanvasTestFixture::s_asyncOperationActive = true;
        AZ::TickBus::Handler::BusConnect();
        AsyncEventNotificationBus::Handler::BusConnect(GetEntityId());

        std::packaged_task<void()> task([this]() { LongRunningProcessSimulator3000::Run(GetEntityId()); }); // wrap the function

        m_eventThread = AZStd::make_shared<AZStd::thread>(AZStd::move(task)); // launch on a thread
    }

    void OnDeactivate() override
    {
        if (m_eventThread)
        {
            m_eventThread->join();
            m_eventThread.reset();
        }

        // We've received the event, no longer need the bus connection
        AsyncEventNotificationBus::Handler::BusDisconnect();

        // We're done, kick it out.
        SignalOutput(GetSlotId("Out"));

        // Disconnect from tick bus as well
        AZ::TickBus::Handler::BusDisconnect();
    }

    virtual void HandleAsyncEvent()
    {
        EXPECT_GT(m_duration, 0.f);

        Shutdown();
    }

    void OnAsyncEvent() override
    {
        HandleAsyncEvent();
    }

    void Shutdown()
    {
        ScriptCanvasTestFixture::s_asyncOperationActive = false;
    }

    void OnTick(float deltaTime, AZ::ScriptTimePoint) override
    {
        AZ_TracePrintf("Debug", "Awaiting async operation: %.2f\n", m_duration);
        m_duration += deltaTime;
    }

protected:
    AZStd::shared_ptr<AZStd::thread> m_eventThread;
private:
    double m_duration = 0.f;
};

TEST_F(ScriptCanvasTestFixture, Asynchronous_Behaviors)
{
    using namespace ScriptCanvas;

    RegisterComponentDescriptor<AsyncNode>();

    // Make the graph.
    ScriptCanvas::Graph* graph =nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    ASSERT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();

    AZ::EntityId startNodeId;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId asyncNodeId;
    CreateTestNode<AsyncNode>(graphUniqueId, asyncNodeId);

    EXPECT_TRUE(Connect(*graph, startNodeId, ScriptCanvas::CommonSlots::GeneralOutSlot::GetName(), asyncNodeId, ScriptCanvas::CommonSlots::GeneralInSlot::GetName()));
    
    {
        ScopedOutputSuppression supressOutput;
        graphEntity->Activate();

        // Tick the TickBus while the graph entity is active
        while (ScriptCanvasTestFixture::s_asyncOperationActive)
        {
            AZ::TickBus::ExecuteQueuedEvents();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));
        }
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
    // Fibonacci solver, used to compare against the graph version.
    long ComputeFibonacci(int digits)
    {
        int a = 0;
        int b = 1;
        long sum = 0;
        for (int i = 0; i < digits - 2; ++i)
        {
            sum = a + b;
            a = b;
            b = sum;
        }
        return sum;
    }
}

class AsyncFibonacciComputeNode
    : public AsyncNode
{
public:    
    AZ_COMPONENT(AsyncFibonacciComputeNode, "{B198F52D-708C-414B-BB90-DFF0462D7F03}", AsyncNode);

    AsyncFibonacciComputeNode()
        : AsyncNode()
    {}

    bool IsEntryPoint() const { return true; }

    static const int k_numberOfFibonacciDigits = 64;

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AsyncFibonacciComputeNode, AsyncNode>()
                ->Version(1)
                ;
        }
    }

    void OnActivate() override
    {
        AZ::TickBus::Handler::BusConnect();
        AsyncEventNotificationBus::Handler::BusConnect(GetEntityId());

        int digits = k_numberOfFibonacciDigits;

        std::promise<long> p;
        m_computeFuture = p.get_future();
        m_eventThread = AZStd::make_shared<AZStd::thread>([this, digits, p = AZStd::move(p)]() mutable
        {
            p.set_value(ComputeFibonacci(digits));
            AsyncEventNotificationBus::Event(GetEntityId(), &AsyncEvent::OnAsyncEvent);
        });
    }

    void HandleAsyncEvent() override
    {
        m_result = m_computeFuture.get();

        EXPECT_EQ(m_result, ComputeFibonacci(k_numberOfFibonacciDigits));
    }

    void OnTick(float deltaTime, AZ::ScriptTimePoint) override
    {
        AZ_TracePrintf("Debug", "Awaiting async fib operation: %.2f\n", m_duration);
        m_duration += deltaTime;

        if (m_result != 0)
        {
            Shutdown();
        }
    }

private:
    std::future<long> m_computeFuture;
    long m_result = 0;
    double m_duration = 0.f;
};

TEST_F(ScriptCanvasTestFixture, ComputeFibonacciAsyncGraphTest)
{
    using namespace ScriptCanvas;

    RegisterComponentDescriptor<AsyncNode>();
    RegisterComponentDescriptor<AsyncFibonacciComputeNode>();

    // Make the graph.
    ScriptCanvas::Graph* graph =nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    ASSERT_NE(graph, nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const ScriptCanvasId& graphUniqueId = graph->GetScriptCanvasId();

    AZ::EntityId startNodeId;
    CreateTestNode<ScriptCanvas::Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId asyncNodeId;
    CreateTestNode<AsyncFibonacciComputeNode>(graphUniqueId, asyncNodeId);

    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", asyncNodeId, "In"));

    graphEntity->Activate();

    // Tick the TickBus while the graph entity is active
    while (ScriptCanvasTestFixture::s_asyncOperationActive)
    {
        AZ::TickBus::ExecuteQueuedEvents();
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

#endif // AZ_COMPILER_MSVC
