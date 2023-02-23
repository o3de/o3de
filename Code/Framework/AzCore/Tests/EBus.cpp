/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/Random.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/AZTestShared/Utils/Utils.h>


#include <gtest/gtest.h>
// For GetTypeName<T>()
#include <gtest/internal/gtest-type-util.h>

using namespace AZ;

// TestBus implementation details
namespace BusImplementation
{
    // Interface for the benchmark bus
    class Interface
    {
    public:
        virtual ~Interface() = default;

        virtual int OnEvent() = 0;
        virtual void OnWait() = 0;
        virtual void Release() = 0;

        virtual bool Compare(const Interface* other) const = 0;
    };

    // Traits for the benchmark bus
    template <AZ::EBusAddressPolicy addressPolicy, AZ::EBusHandlerPolicy handlerPolicy, bool locklessDispatch = false>
    class Traits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = addressPolicy;
        static const AZ::EBusHandlerPolicy HandlerPolicy = handlerPolicy;
        static const bool LocklessDispatch = locklessDispatch;

        // Allow queuing
        static const bool EnableEventQueue = true;

        // Force locking
        using MutexType = AZStd::recursive_mutex;

        // Only specialize BusIdType if not single address
        using BusIdType = AZStd::conditional_t<AddressPolicy == AZ::EBusAddressPolicy::Single, AZ::NullBusId, int>;
        // Only specialize BusIdOrderCompare if addresses are multiple and ordered
        using BusIdOrderCompare = AZStd::conditional_t<AddressPolicy != EBusAddressPolicy::ByIdAndOrdered, AZ::NullBusIdCompare, AZStd::less<int>>;
    };

    template <typename Bus>
    class HandlerCommon
        : public Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(HandlerCommon, AZ::SystemAllocator);

        unsigned int m_eventCalls = 0;
        unsigned int m_expectedOrder = 0;
        unsigned int m_executedOrder = 0;

        HandlerCommon()
        {
            AZ::BetterPseudoRandom random;
            random.GetRandom(m_expectedOrder);
        }

        HandlerCommon(uint32_t handlerOrder)
        {
            m_expectedOrder = handlerOrder;
        }

        ~HandlerCommon() override
        {
            Bus::Handler::BusDisconnect();
        }

        bool Compare(const Interface* other) const override
        {
            return m_expectedOrder < reinterpret_cast<const HandlerCommon*>(other)->m_expectedOrder;
        }

        int OnEvent() override
        {
            ++m_eventCalls;
            m_executedOrder = s_nextExecution++;
            return 0;
        }

        void OnWait() override
        {
            AZStd::this_thread::yield();
        }

        void Release() override
        {
            delete this;
        }

    private:
        static int s_nextExecution;
    };

    template <typename Bus>
    int HandlerCommon<Bus>::s_nextExecution = 0;

    template <typename Bus>
    class MultiHandlerCommon
        : public Bus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiHandlerCommon, AZ::SystemAllocator);

        MultiHandlerCommon() = default;

        ~MultiHandlerCommon() override
        {
            Bus::MultiHandler::BusDisconnect();
        }

        int OnEvent() override
        {
            ++m_eventCalls;
            return 0;
        }

        void OnWait() override
        {
            AZStd::this_thread::yield();
        }

        void Release() override
        {
            delete this;
        }

        bool Compare(const Interface* other) const override
        {
            return m_expectedOrder < static_cast<const MultiHandlerCommon*>(other)->m_expectedOrder;
        }

    private:
        uint32_t m_eventCalls{};
        uint32_t m_expectedOrder{};
    };

    class InterfaceWithMutex
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = uint32_t;

        // Setting the MutexType to a value other than NullMutex
        // signals to the EBus system that the Ebus is able to be used in multiple threads
        // and therefore the EBus must be disconnected prior to the EBus internal handler destructor being invoked
        using MutexType = AZStd::recursive_mutex;

        virtual void OnEvent() = 0;
    };

    using InterfaceWithMutexBus = AZ::EBus<InterfaceWithMutex>;
}

class MutexBusHandler
    : public BusImplementation::InterfaceWithMutexBus::Handler
{
    void OnEvent() override
    {
        ++m_eventCalls;
    }
private:
    uint32_t m_eventCalls{};
};

// Definition of the benchmark bus, depending on supplied policies
template <AZ::EBusAddressPolicy addressPolicy, AZ::EBusHandlerPolicy handlerPolicy, bool locklessDispatch = false>
using TestBus = AZ::EBus<BusImplementation::Interface, BusImplementation::Traits<addressPolicy, handlerPolicy, locklessDispatch>>;

#define EBUS_TEST_ALIAS(BusType, AddressPolicy, HandlerPolicy)                                              \
    using BusType = TestBus<AZ::EBusAddressPolicy::AddressPolicy, AZ::EBusHandlerPolicy::HandlerPolicy>;    \
    namespace testing { namespace internal { template<> std::string GetTypeName<BusType>() { return #BusType; } } }

// Predefined benchmark bus instantiations
// Single
EBUS_TEST_ALIAS(OneToOne, Single, Single)
EBUS_TEST_ALIAS(OneToMany, Single, Multiple)
EBUS_TEST_ALIAS(OneToManyOrdered, Single, MultipleAndOrdered)
// ById
EBUS_TEST_ALIAS(ManyToOne, ById, Single)
EBUS_TEST_ALIAS(ManyToMany, ById, Multiple)
EBUS_TEST_ALIAS(ManyToManyOrdered, ById, MultipleAndOrdered)
// ByIdAndOrdered
EBUS_TEST_ALIAS(ManyOrderedToOne, ByIdAndOrdered, Single)
EBUS_TEST_ALIAS(ManyOrderedToMany, ByIdAndOrdered, Multiple)
EBUS_TEST_ALIAS(ManyOrderedToManyOrdered, ByIdAndOrdered, MultipleAndOrdered)

// Handler for multi-address buses
template <typename Bus, AZ::EBusAddressPolicy addressPolicy = Bus::Traits::AddressPolicy>
class Handler
    : public BusImplementation::HandlerCommon<Bus>
{
public:
    AZ_CLASS_ALLOCATOR(Handler, AZ::SystemAllocator);

    Handler(int id, bool connectOnConstruct)
    {
        m_busId = id;

        if (connectOnConstruct)
        {
            EXPECT_FALSE(this->BusIsConnected());
            Connect();
            EXPECT_TRUE(this->BusIsConnected());
        }
    }

    Handler(int id, int handlerOrder, bool connectOnConstruct)
        : BusImplementation::HandlerCommon<Bus>(handlerOrder)
    {
        m_busId = id;

        if (connectOnConstruct)
        {
            EXPECT_FALSE(this->BusIsConnected());
            Connect();
            EXPECT_TRUE(this->BusIsConnected());
        }
    }

    ~Handler() override = default;

    // Helper function for connecting without specifying an id
    void Connect()
    {
        this->BusConnect(m_busId);
    }

    void Disconnect()
    {
        this->BusDisconnect(m_busId);
    }

    int OnEvent() override
    {
        BusImplementation::HandlerCommon<Bus>::OnEvent();
        return 1;
    }

private:
    int m_busId = 0;
};

// Special handler for single address buses
template <typename Bus>
class Handler<Bus, AZ::EBusAddressPolicy::Single>
    : public BusImplementation::HandlerCommon<Bus>
{
public:
    AZ_CLASS_ALLOCATOR(Handler, AZ::SystemAllocator);

    Handler(int, bool connectOnConstruct)
    {
        if (connectOnConstruct)
        {
            EXPECT_FALSE(this->BusIsConnected());
            Connect();
            EXPECT_TRUE(this->BusIsConnected());
        }
    }

    Handler(int, int handlerOrder, bool connectOnConstruct)
        : BusImplementation::HandlerCommon<Bus>(handlerOrder)
    {
        if (connectOnConstruct)
        {
            EXPECT_FALSE(this->BusIsConnected());
            Connect();
            EXPECT_TRUE(this->BusIsConnected());
        }
    }

    // Helper function for connecting without specifying an id
    void Connect()
    {
        this->BusConnect();
    }

    void Disconnect()
    {
        this->BusDisconnect();
    }

    int OnEvent() override
    {
        BusImplementation::HandlerCommon<Bus>::OnEvent();
        return 2;
    }
};

// Handler for multi-address buses
template <typename Bus>
class MultiHandlerById
    : public BusImplementation::MultiHandlerCommon<Bus>
{
public:
    AZ_CLASS_ALLOCATOR(MultiHandlerById, AZ::SystemAllocator);

    MultiHandlerById(std::initializer_list<int> busIdList)
    {
        // We will bind at construction time to the bus. Disconnect is automatic when the object is
        // destroyed or we can call BusDisconnect()
        EXPECT_FALSE(this->BusIsConnected());
        Connect(busIdList);
        EXPECT_TRUE(this->BusIsConnected());
    }

    // Helper function for connecting on multiple ids
    void Connect(std::initializer_list<int> busIdList)
    {
        for (int busId : busIdList)
        {
            this->BusConnect(busId);
        }
    }

    void Disconnect(std::initializer_list<int> busIdList)
    {
        for (int busId : busIdList)
        {
            this->BusDisconnect(busId);
        }
    }

    int OnEvent() override
    {
        return BusImplementation::MultiHandlerCommon<Bus>::OnEvent();
    }
};


namespace UnitTest
{
    using BusTypesId = ::testing::Types<
        ManyToOne,        ManyToMany,        ManyToManyOrdered,
        ManyOrderedToOne, ManyOrderedToMany, ManyOrderedToManyOrdered>;
    using BusTypesAll = ::testing::Types<
        OneToOne,         OneToMany,         OneToManyOrdered,
        ManyToOne,        ManyToMany,        ManyToManyOrdered,
        ManyOrderedToOne, ManyOrderedToMany, ManyOrderedToManyOrdered>;

    template <typename Bus>
    class EBusTestAll
        : public LeakDetectionFixture
    {
    public:
        using BusHandler = Handler<Bus>;
        using BusMultiHandlerById = MultiHandlerById<Bus>;

        EBusTestAll()
        {
            Bus::GetOrCreateContext();
        }


        void TearDown() override
        {
            DestroyHandlers();
            LeakDetectionFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // Handler Helpers

        // Create an appropriate number of handlers for testing
        void CreateHandlers()
        {
            int numAddresses = HasMultipleAddresses() ? 3 : 1;
            int numHandlersPerAddress = HasMultipleHandlersPerAddress() ? 3 : 1;
            constexpr bool connectOnConstruct{ true };

            for (int address = 0; address < numAddresses; ++address)
            {
                for (int handler = 0; handler < numHandlersPerAddress; ++handler)
                {
                    m_handlers[address].emplace_back(aznew BusHandler(address, connectOnConstruct));
                    ++m_numHandlers;
                }
            }

            ValidateCalls(0);
        }

        // Gets the total number of handlers active
        int GetNumHandlers()
        {
            return m_numHandlers;
        }

        // Clears the handlers list without deleting them (useful for Release tests)
        void ClearHandlers()
        {
            m_handlers.clear();
            m_handlers.rehash(0);
            m_numHandlers = 0;
        }

        // Destroy all handlers
        void DestroyHandlers()
        {
            for (const auto& handlerPair : m_handlers)
            {
                for (BusHandler* handler : handlerPair.second)
                {
                    delete handler;
                }
            }
            ClearHandlers();

            EXPECT_FALSE(Bus::HasHandlers());
        }

        // Ensure that all active handlers have the expected call count, in the correct order
        // This should only be called after Broadcast()
        void ValidateCalls(int expected, bool isForward = true)
        {
            for (const auto& handlerPair : m_handlers)
            {
                ValidateCalls(expected, handlerPair.first, isForward);
            }

            // Validate address execution order
            if (AddressesAreOrdered())
            {
                // Collect the first handler from each address
                using PairType = AZStd::pair<int, BusHandler*>;
                AZStd::vector<PairType> sortedHandlers;
                for (const auto& handlerPair : m_handlers)
                {
                    PairType pair(handlerPair.first, handlerPair.second.front());

                    auto insertPos = AZStd::lower_bound(
                        sortedHandlers.begin(), sortedHandlers.end(),
                        pair,
                        [](const PairType& lhs, const PairType& rhs)
                    {
                        return lhs.first < rhs.first;
                    }
                    );

                    sortedHandlers.emplace(insertPos, pair);
                }

                // Iterate over the list, and validate that they were called in the correct order
                unsigned int lastExecuted = 0;
                for (const PairType& pair : sortedHandlers)
                {
                    if (lastExecuted > 0)
                    {
                        if (isForward)
                        {
                            EXPECT_LT(lastExecuted, pair.second->m_executedOrder);
                        }
                        else
                        {
                            EXPECT_GT(lastExecuted, pair.second->m_executedOrder);
                        }
                    }
                    lastExecuted = pair.second->m_executedOrder;
                }
            }
        }

        // Ensure that all active handlers have the expected call count, and were called in the correct order
        void ValidateCalls(int expected, int id, bool isForward = true)
        {
            auto& handlers = m_handlers[id];

            for (BusHandler* handler : handlers)
            {
                EXPECT_EQ(expected, handler->m_eventCalls);
            }

            // Validate handler execution order
            if (HandlersAreOrdered())
            {
                // Sort the handlers the same way we expect the bus to sort them
                auto sortedHandlers = handlers;
                AZStd::sort(sortedHandlers.begin(), sortedHandlers.end(), AZStd::bind(&BusHandler::Compare, AZStd::placeholders::_1, AZStd::placeholders::_2));

                // Iterate over the list, and validate that they were called in the correct order
                unsigned int lastExecuted = 0;
                for (const BusHandler* handler : sortedHandlers)
                {
                    if (lastExecuted > 0)
                    {
                        if (isForward)
                        {
                            EXPECT_LT(lastExecuted, handler->m_executedOrder);
                        }
                        else
                        {
                            EXPECT_GT(lastExecuted, handler->m_executedOrder);
                        }
                    }
                    lastExecuted = handler->m_executedOrder;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Metadata helpers
        bool HasMultipleAddresses()
        {
            return Bus::HasId;
        }

        bool AddressesAreOrdered()
        {
            return Bus::Traits::AddressPolicy == EBusAddressPolicy::ByIdAndOrdered;
        }

        bool HasMultipleHandlersPerAddress()
        {
            return Bus::Traits::HandlerPolicy != EBusHandlerPolicy::Single;
        }

        bool HandlersAreOrdered()
        {
            return Bus::Traits::HandlerPolicy == EBusHandlerPolicy::MultipleAndOrdered;
        }

    protected:
        AZStd::unordered_map<int, AZStd::vector<BusHandler*>> m_handlers;
        int m_numHandlers = 0;
    };
    TYPED_TEST_CASE(EBusTestAll, BusTypesAll);

    template <typename Bus>
    class EBusTestId
        : public EBusTestAll<Bus>
    {
    };
    TYPED_TEST_CASE(EBusTestId, BusTypesId);

    using BusTypesIdMultiHandlers = ::testing::Types<
        ManyToMany, ManyToManyOrdered,
        ManyOrderedToMany, ManyOrderedToManyOrdered>;
    template <typename Bus>
    class EBusTestIdMultiHandlers
        : public EBusTestAll<Bus>
    {
    };
    TYPED_TEST_CASE(EBusTestIdMultiHandlers, BusTypesIdMultiHandlers);

    //////////////////////////////////////////////////////////////////////////
    // Non-event functions

    TYPED_TEST(EBusTestAll, ConnectDisconnect)
    {
        using Bus = TypeParam;
        using Handler = typename EBusTestAll<Bus>::BusHandler;

        constexpr bool connectOnConstruct{ true };
        Handler meh(0, connectOnConstruct);
        EXPECT_EQ(0, meh.m_eventCalls);

        EXPECT_TRUE(Bus::HasHandlers());

        Bus::Broadcast(&Bus::Events::OnEvent);
        EXPECT_EQ(1, meh.m_eventCalls);

        EXPECT_TRUE(meh.BusIsConnected());
        meh.BusDisconnect(); // we disconnect from receiving events.
        EXPECT_FALSE(meh.BusIsConnected());
        EXPECT_FALSE(Bus::HasHandlers());

        // this signal will NOT trigger any calls.
        Bus::Broadcast(&Bus::Events::OnEvent);
        EXPECT_EQ(1, meh.m_eventCalls);
    }

    TYPED_TEST(EBusTestIdMultiHandlers, EnumerateHandlers_MultiHandler)
    {
        using Bus = TypeParam;
        using BusMultiHandlerById = typename EBusTestAll<Bus>::BusMultiHandlerById;

        BusMultiHandlerById sourceMultiHandler{ 0, 1, 2 };
        BusMultiHandlerById multiHandlerWithOverlappingIds{ 1, 3, 5 };

        // Test handlers' enumeration functionality
        Bus::EnumerateHandlers([](typename BusMultiHandlerById::Interface* interfaceInst) -> bool
        {
            interfaceInst->OnEvent();
            return true;
        });
    }

    TYPED_TEST(EBusTestId, FindFirstHandler)
    {
        using Bus = TypeParam;
        using Handler = typename EBusTestAll<Bus>::BusHandler;
        constexpr bool connectOnConstruct{ true };
        Handler meh0(0, connectOnConstruct);  /// <-- Bind to bus 0
        Handler meh1(1, connectOnConstruct);  /// <-- Bind to bus 1

        // Test handlers' enumeration functionality
        EXPECT_EQ(&meh0, Bus::FindFirstHandler(0));
        EXPECT_EQ(&meh1, Bus::FindFirstHandler(1));
        EXPECT_EQ(nullptr, Bus::FindFirstHandler(3));
    }

    //////////////////////////////////////////////////////////////////////////
    // Immediate calls

    TYPED_TEST(EBusTestAll, Broadcast)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        Bus::Broadcast(&Bus::Events::OnEvent);
        this->ValidateCalls(1);

        Bus::Broadcast(&Bus::Events::OnEvent);
        this->ValidateCalls(2);
    }

    TYPED_TEST(EBusTestAll, Broadcast_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::Broadcast(&Bus::Events::Release);
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    TYPED_TEST(EBusTestAll, BroadcastReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        Bus::BroadcastReverse(&Bus::Events::OnEvent);
        this->ValidateCalls(1, false);

        Bus::BroadcastReverse(&Bus::Events::OnEvent);
        this->ValidateCalls(2, false);
    }

    TYPED_TEST(EBusTestAll, BroadcastReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::BroadcastReverse(&Bus::Events::Release);
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    TYPED_TEST(EBusTestAll, BroadcastResult)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        int result = -1;
        Bus::BroadcastResult(result, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);
        this->ValidateCalls(1);

        result = -1;
        Bus::BroadcastResult(result, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);
        this->ValidateCalls(2);

        this->DestroyHandlers();

        result = -1;
        Bus::BroadcastResult(result, &Bus::Events::OnEvent);
        EXPECT_EQ(-1, result);
    }

    TYPED_TEST(EBusTestAll, BroadcastResultReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        int result = -1;
        Bus::BroadcastResultReverse(result, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);
        this->ValidateCalls(1, false);

        result = -1;
        Bus::BroadcastResultReverse(result, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);
        this->ValidateCalls(2, false);

        this->DestroyHandlers();

        result = -1;
        Bus::BroadcastResultReverse(result, &Bus::Events::OnEvent);
        EXPECT_EQ(-1, result);
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, Event)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        Bus::Event(1, &Bus::Events::OnEvent);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, Event_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Event(handlerPair.first, &Bus::Events::Release);
        }
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, EventReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        Bus::EventReverse(1, &Bus::Events::OnEvent);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);
    }

    // Test sending events (that delete this) on an address, backwards
    TYPED_TEST(EBusTestId, EventReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::EventReverse(handlerPair.first, &Bus::Events::Release);
        }
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events with results on an address
    TYPED_TEST(EBusTestId, EventResult)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        int result = -1;
        Bus::EventResult(result, 1, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);

        this->DestroyHandlers();

        result = -1;
        Bus::EventResult(result, 1, &Bus::Events::OnEvent);
        EXPECT_EQ(-1, result);
    }

    // Test sending events with results on an address
    TYPED_TEST(EBusTestId, EventResultReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        int result = -1;
        Bus::EventResultReverse(result, 1, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);

        this->DestroyHandlers();

        result = -1;
        Bus::EventResultReverse(result, 1, &Bus::Events::OnEvent);
        EXPECT_EQ(-1, result);
    }

    // Test sending events on a bound bus ptr
    TYPED_TEST(EBusTestId, BindEvent)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        Bus::Event(ptr, &Bus::Events::OnEvent);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);

        this->DestroyHandlers();

        // Validate that broadcasting/eventing after binding disconnecting all doesn't crash
        Bus::Broadcast(&Bus::Events::OnEvent);
        Bus::Event(ptr, &Bus::Events::OnEvent);
        Bus::Event(1, &Bus::Events::OnEvent);
    }

    // Test sending events (that delete this) on a bound bus ptr
    TYPED_TEST(EBusTestId, BindEvent_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::Event(ptr, &Bus::Events::Release);
        }
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on a bound bus ptr, backwards
    TYPED_TEST(EBusTestId, BindEventReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        Bus::EventReverse(ptr, &Bus::Events::OnEvent);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);

        this->DestroyHandlers();

        // Validate that broadcasting/eventing after binding disconnecting all doesn't crash
        Bus::BroadcastReverse(&Bus::Events::OnEvent);
        Bus::EventReverse(ptr, &Bus::Events::OnEvent);
        Bus::EventReverse(1, &Bus::Events::OnEvent);
    }

    // Test sending events (that delete this) on a bound bus ptr, backwards
    TYPED_TEST(EBusTestId, BindEventReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::EventReverse(ptr, &Bus::Events::Release);
        }
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on a bound bus ptr
    TYPED_TEST(EBusTestId, BindEventResult)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        int result = -1;
        Bus::EventResult(result, ptr, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);

        this->DestroyHandlers();

        // Validate that broadcasting/eventing after binding disconnecting all doesn't crash
        Bus::Broadcast(&Bus::Events::OnEvent);
        Bus::Event(ptr, &Bus::Events::OnEvent);
        Bus::Event(1, &Bus::Events::OnEvent);
    }

    // Test sending events on a bound bus ptr, backwards
    TYPED_TEST(EBusTestId, BindEventResultReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        int result = -1;
        Bus::EventResultReverse(result, ptr, &Bus::Events::OnEvent);
        EXPECT_LT(0, result);

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);

        this->DestroyHandlers();

        // Validate that broadcasting/eventing after binding disconnecting all doesn't crash
        Bus::BroadcastReverse(&Bus::Events::OnEvent);
        Bus::EventReverse(ptr, &Bus::Events::OnEvent);
        Bus::EventReverse(1, &Bus::Events::OnEvent);
    }

    //////////////////////////////////////////////////////////////////////////
    // Queued calls

    TYPED_TEST(EBusTestAll, QueueBroadcast)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        Bus::QueueBroadcast(&Bus::Events::OnEvent);
        this->ValidateCalls(0);
        Bus::ExecuteQueuedEvents();
        this->ValidateCalls(1);

        Bus::QueueBroadcast(&Bus::Events::OnEvent);
        this->ValidateCalls(1);
        Bus::ExecuteQueuedEvents();
        this->ValidateCalls(2);
    }

    TYPED_TEST(EBusTestAll, QueueBroadcast_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::QueueBroadcast(&Bus::Events::Release);
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    TYPED_TEST(EBusTestAll, QueueBroadcastReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        Bus::QueueBroadcastReverse(&Bus::Events::OnEvent);
        this->ValidateCalls(0, false);
        Bus::ExecuteQueuedEvents();
        this->ValidateCalls(1, false);

        Bus::QueueBroadcastReverse(&Bus::Events::OnEvent);
        this->ValidateCalls(1, false);
        Bus::ExecuteQueuedEvents();
        this->ValidateCalls(2, false);
    }

    TYPED_TEST(EBusTestAll, QueueBroadcastReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::QueueBroadcastReverse(&Bus::Events::Release);
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, QueueEvent)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        Bus::QueueEvent(1, &Bus::Events::OnEvent);
        this->ValidateCalls(0);
        Bus::ExecuteQueuedEvents();

        // Validate bus 1 has 1 calls, all others have 0
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, QueueEvent_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::QueueEvent(handlerPair.first, &Bus::Events::Release);
        }
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on an address
    TYPED_TEST(EBusTestId, QueueEventReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        // Signal OnEvent event on bus 1
        Bus::QueueEventReverse(1, &Bus::Events::OnEvent);
        this->ValidateCalls(0);
        Bus::ExecuteQueuedEvents();

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);
    }

    // Test sending events (that delete this) on an address, backwards
    TYPED_TEST(EBusTestId, QueueEventReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::QueueEventReverse(handlerPair.first, &Bus::Events::Release);
        }
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on a bound bus ptr
    TYPED_TEST(EBusTestId, QueueBindEvent)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        Bus::QueueEvent(ptr, &Bus::Events::OnEvent);
        this->ValidateCalls(0);
        Bus::ExecuteQueuedEvents();

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1);
        this->ValidateCalls(0, 2);
        this->ValidateCalls(0, 3);
    }

    // Test sending events (that delete this) on a bound bus ptr
    TYPED_TEST(EBusTestId, QueueBindEvent_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::QueueEvent(ptr, &Bus::Events::Release);
        }
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    // Test sending events on a bound bus ptr, backwards
    TYPED_TEST(EBusTestId, QueueBindEventReverse)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        Bus::Bind(ptr, 1);
        EXPECT_NE(nullptr, ptr);

        // Signal OnEvent event on bus 1
        Bus::QueueEventReverse(ptr, &Bus::Events::OnEvent);
        this->ValidateCalls(0);
        Bus::ExecuteQueuedEvents();

        // Validate bus 1 has 2 calls, all others have 1
        this->ValidateCalls(1, 1, false);
        this->ValidateCalls(0, 2, false);
        this->ValidateCalls(0, 3, false);
    }

    // Test sending events (that delete this) on a bound bus ptr, backwards
    TYPED_TEST(EBusTestId, QueueBindEventReverse_Release)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::QueueEventReverse(ptr, &Bus::Events::Release);
        }
        EXPECT_EQ(this->GetNumHandlers(), Bus::GetTotalNumOfEventHandlers());
        Bus::ExecuteQueuedEvents();
        EXPECT_FALSE(Bus::HasHandlers());

        this->ClearHandlers();
    }

    //////////////////////////////////////////////////////////////////////////
    // GetCurrentBusId calls
    TYPED_TEST(EBusTestId, Event_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            ASSERT_NE(nullptr, busId);
            EXPECT_EQ(1, *busId);
        };

        Bus::Event(1, busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EventResult_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*) -> const char*
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            if (busId)
            {
                EXPECT_EQ(1, *busId);
            }
            return "BusType";
        };

        const char* result{};
        Bus::EventResult(result, 1, busCallback);
        EXPECT_STREQ("BusType", result);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EventReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            ASSERT_NE(nullptr, busId);
            EXPECT_EQ(1, *busId);
        };

        Bus::EventReverse(1, busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EventResultReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            if (busId)
            {
                EXPECT_EQ(1, *busId);
            }
            return 7;
        };

        int32_t result{};
        Bus::EventResultReverse(result, 1, busCallback);
        EXPECT_EQ(7, result);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BindEvent_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        auto busCallback = [](typename Bus::InterfaceType*)
        {
            EXPECT_NE(nullptr, Bus::GetCurrentBusId());
        };

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::Event(ptr, busCallback);
        }

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BindEventResult_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        auto busCallback = [](typename Bus::InterfaceType*)
        {
            EXPECT_NE(nullptr, Bus::GetCurrentBusId());
            return true;
        };

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            bool result{};
            Bus::EventResult(result, ptr, busCallback);
            EXPECT_TRUE(result);
        }

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BindEventReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        auto busCallback = [](typename Bus::InterfaceType*)
        {
            EXPECT_NE(nullptr, Bus::GetCurrentBusId());
        };

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            Bus::EventReverse(ptr, busCallback);
        }

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BindEventResultReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        typename Bus::BusPtr ptr;
        auto busCallback = [](typename Bus::InterfaceType*)
        {
            EXPECT_NE(nullptr, Bus::GetCurrentBusId());
            return true;
        };

        for (const auto& handlerPair : this->m_handlers)
        {
            Bus::Bind(ptr, handlerPair.first);
            EXPECT_NE(nullptr, ptr);
            bool result{};
            Bus::EventResultReverse(result, ptr, busCallback);
            EXPECT_TRUE(result);
        }

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, Broadcast_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
        };

        Bus::Broadcast(busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BroadcastResult_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            return 16.0f;
        };

        float result{};
        Bus::BroadcastResult(result, busCallback);
        EXPECT_FLOAT_EQ(16.0f, result);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BroadcastReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
        };

        Bus::BroadcastReverse(busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, BroadcastResultReverse_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            return 8.0;
        };

        double result{};
        Bus::BroadcastResultReverse(result, busCallback);
        EXPECT_DOUBLE_EQ(8.0, result);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EnumerateHandlers_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            return true;
        };

        Bus::EnumerateHandlers(busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EnumerateHandlersId_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            if (busId)
            {
                EXPECT_EQ(1, *busId);
            }
            return true;
        };

        Bus::EnumerateHandlersId(1, busCallback);

        this->DestroyHandlers();
    }

    TYPED_TEST(EBusTestId, EnumerateHandlersPtr_GetCurrentBusId_ReturnsNonNullptr)
    {
        using Bus = TypeParam;

        this->CreateHandlers();

        auto busCallback = [](typename Bus::InterfaceType*)
        {
            const int* busId = Bus::GetCurrentBusId();
            EXPECT_NE(nullptr, busId);
            if(busId)
            {
                EXPECT_EQ(1, *busId);
            }
            return true;
        };

        typename Bus::BusPtr busPtr;
        Bus::Bind(busPtr, 1);
        EXPECT_NE(nullptr, busPtr);
        Bus::EnumerateHandlersPtr(busPtr, busCallback);

        this->DestroyHandlers();
    }

    class EBus
        : public LeakDetectionFixture
    {};

    TEST_F(EBus, DISABLED_CopyConstructorOfEBusHandlerDoesNotAssertInInternalDestructorOfHandler)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            MutexBusHandler sourceHandler;
            // Connect source handler to InterfaceWithMutexBus and then copy it over to a new instance
            // Afterwards disconnect the source handler from the InterfaceWithMutexBus
            sourceHandler.BusConnect(1);
            MutexBusHandler targetHandler(sourceHandler);
            sourceHandler.BusDisconnect();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }

    TEST_F(EBus, DISABLED_CopyAssignmentOfEBusHandlerDoesNotAssertInInternalDestructorOfHandler)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            MutexBusHandler sourceHandler;
            MutexBusHandler targetHandler;
            // Connect source handler to InterfaceWithMutexBus and then copy it over to a new instance
            // Afterwards disconnect the source handler from the InterfaceWithMutexBus
            sourceHandler.BusConnect(1);
            targetHandler = sourceHandler;
            sourceHandler.BusDisconnect();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }

    TEST_F(EBus, CopyConstructorOfEBusHandler_CopyFromConnected_DoesNotAssert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            MutexBusHandler sourceHandler;
            // Connect source handler to InterfaceWithMutexBus and then copy it over to a new instance
            // Afterwards disconnect the source handler from the InterfaceWithMutexBus
            sourceHandler.BusConnect(1);
            // Copy behavior which connects to source handler's bus if
            // Source handler was connected may be unexpected but it should not assert
            MutexBusHandler targetHandler(sourceHandler);
            sourceHandler.BusDisconnect();
            targetHandler.BusDisconnect();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }

    TEST_F(EBus, CopyOperatorOfEBusHandler_CopyToConnected_DoesNotAssert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            MutexBusHandler targetHandler;
            // Connect source handler to InterfaceWithMutexBus and then copy it over to a new instance
            // Afterwards disconnect the source handler from the InterfaceWithMutexBus
            targetHandler.BusConnect(1);
            // Copy behavior which connects to source handler's bus if
            // Source handler was connected may be unexpected but it should not assert
            MutexBusHandler sourceHandler;
            targetHandler = sourceHandler;
            sourceHandler.BusDisconnect();
            targetHandler.BusDisconnect();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }
    /**
    * Tests multi-bus handler (a singe ebus instance that can connect to multiple buses)
    */
    namespace MultBusHandler
    {
        /**
        * Create event that allows MULTI buses. By default we already allow multiple handlers per bus.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus interface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::MultiHandler
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler(MyEventGroupBus::BusIdType busId0, MyEventGroupBus::BusIdType busId1)
                : actionCalls(0)
                , sumCalls(0)
            {
                BusConnect(busId0); // connect to the specific bus
                BusConnect(busId1); // connect to the specific bus
            }

            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            void    OnAction(float x, float y) override
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) called\n", x, y); ++actionCalls;
            }

            float   OnSum(float x, float y) override
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) on called\n", sum, x, y); ++sumCalls; return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }

    TEST_F(EBus, MultBusHandler)
    {
        using namespace MultBusHandler;
        {
            MyEventHandler meh0(0, 1);     /// <-- Bind to bus 0 and 1

            // Signal OnAction event on all buses
            MyEventGroupBus::Broadcast(&MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);

            // Signal OnSum event
            MyEventGroupBus::Broadcast(&MyEventGroupBus::Events::OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);

            // Signal OnAction event on bus 0
            MyEventGroupBus::Event(0, &MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);
            EXPECT_EQ(3, meh0.actionCalls);

            // Signal OnAction event on bus 1
            MyEventGroupBus::Event(1, &MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);
            EXPECT_EQ(4, meh0.actionCalls);

            meh0.BusDisconnect(1); // we disconnect from receiving events on bus 1

            MyEventGroupBus::Broadcast(&MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);  // this signal will NOT trigger only one call
            EXPECT_EQ(5, meh0.actionCalls);
        }
    }

    /**
     *
     */
    namespace QueueMessageTest
    {
        class QueueTestEventsMultiBus
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZStd::mutex MutexType;

            typedef int            BusIdType;
            static const bool EnableEventQueue = true;
            //////////////////////////////////////////////////////////////////////////
            QueueTestEventsMultiBus()
                : m_callCount(0) {}
            virtual ~QueueTestEventsMultiBus() {}
            virtual void OnMessage() { m_callCount++; }

            int m_callCount;
        };
        typedef AZ::EBus<QueueTestEventsMultiBus> QueueTestMultiBus;

        class QueueTestEventsSingleBus
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            typedef AZStd::mutex MutexType;
            static const bool EnableEventQueue = true;
            //////////////////////////////////////////////////////////////////////////
            QueueTestEventsSingleBus()
                : m_callCount(0) {}
            virtual ~QueueTestEventsSingleBus() {}
            virtual void OnMessage() { m_callCount++; }

            int m_callCount;
        };
        typedef AZ::EBus<QueueTestEventsSingleBus> QueueTestSingleBus;

        JobManager*                    m_jobManager = nullptr;
        JobContext*                    m_jobContext = nullptr;
        QueueTestMultiBus::Handler*    m_multiHandler = nullptr;
        QueueTestSingleBus::Handler*   m_singleHandler = nullptr;
        QueueTestMultiBus::BusPtr      m_multiPtr = nullptr;

        void QueueMessage()
        {
            QueueTestMultiBus::QueueEvent(0, &QueueTestMultiBus::Events::OnMessage);
            QueueTestSingleBus::QueueBroadcast(&QueueTestSingleBus::Events::OnMessage);
        }

        void QueueMessagePtr()
        {
            QueueTestMultiBus::QueueEvent(m_multiPtr, &QueueTestMultiBus::Events::OnMessage);
            QueueTestSingleBus::QueueBroadcast(&QueueTestSingleBus::Events::OnMessage);
        }
    }

    TEST_F(EBus, QueueMessage)
    {
        using namespace QueueMessageTest;

        // Setup
        JobManagerDesc jobDesc;
        JobManagerThreadDesc threadDesc;
        jobDesc.m_workerThreads.push_back(threadDesc);
        jobDesc.m_workerThreads.push_back(threadDesc);
        jobDesc.m_workerThreads.push_back(threadDesc);
        m_jobManager = aznew JobManager(jobDesc);
        m_jobContext = aznew JobContext(*m_jobManager);
        JobContext::SetGlobalContext(m_jobContext);
        m_multiHandler = new QueueTestMultiBus::Handler();
        m_singleHandler = new QueueTestSingleBus::Handler();

        m_singleHandler->m_callCount = 0;
        m_multiHandler->m_callCount = 0;
        const int NumCalls = 5000;
        QueueTestMultiBus::Bind(m_multiPtr, 0);
        m_multiHandler->BusConnect(0);
        m_singleHandler->BusConnect();
        for (int i = 0; i < NumCalls; ++i)
        {
            Job* job = CreateJobFunction(&QueueMessageTest::QueueMessage, true);
            job->Start();
            job = CreateJobFunction(&QueueMessageTest::QueueMessagePtr, true);
            job->Start();
        }
        while (m_singleHandler->m_callCount < NumCalls * 2 || m_multiHandler->m_callCount < NumCalls * 2)
        {
            QueueTestMultiBus::ExecuteQueuedEvents();
            QueueTestSingleBus::ExecuteQueuedEvents();
            AZStd::this_thread::yield();
        }

        // use queuing generic functions to disconnect from the bus

        // the same as m_singleHandler.BusDisconnect(); but delayed until QueueTestSingleBus::ExecuteQueuedEvents()
        QueueTestSingleBus::QueueFunction(&QueueTestSingleBus::Handler::BusDisconnect, m_singleHandler);

        // the same as m_multiHandler.BusDisconnect(); but dalayed until QueueTestMultiBus::ExecuteQueuedEvents();
        QueueTestMultiBus::QueueFunction(static_cast<void(QueueTestMultiBus::Handler::*)()>(&QueueTestMultiBus::Handler::BusDisconnect), m_multiHandler);

        EXPECT_EQ(1, QueueTestSingleBus::GetTotalNumOfEventHandlers());
        EXPECT_EQ(1, QueueTestMultiBus::GetTotalNumOfEventHandlers());
        QueueTestSingleBus::ExecuteQueuedEvents();
        QueueTestMultiBus::ExecuteQueuedEvents();
        EXPECT_EQ(0, QueueTestSingleBus::GetTotalNumOfEventHandlers());
        EXPECT_EQ(0, QueueTestMultiBus::GetTotalNumOfEventHandlers());

        // Cleanup
        delete m_singleHandler;
        delete m_multiHandler;
        m_multiPtr = nullptr;
        JobContext::SetGlobalContext(nullptr);
        delete m_jobContext;
        delete m_jobManager;
    }

    class QueueEbusTest
        : public LeakDetectionFixture
    {

    };

    TEST_F(QueueEbusTest, QueueMessageNoQueueing_QueueMessage_Warning)
    {
        using namespace QueueMessageTest;
        {
            AZ::Test::AssertAbsorber assertAbsorber;
            QueueMessage();
            EXPECT_EQ(assertAbsorber.m_warningCount, 0);
        }
        QueueTestSingleBus::ExecuteQueuedEvents();
        QueueTestSingleBus::AllowFunctionQueuing(false);
        {
            AZ::Test::AssertAbsorber assertAbsorber;
            QueueMessage();
            EXPECT_EQ(assertAbsorber.m_warningCount, 1);
        }
        QueueTestMultiBus::ExecuteQueuedEvents();
        QueueTestSingleBus::AllowFunctionQueuing(true);

    }

    class ConnectDisconnectInterface
        : public EBusTraits
    {
    public:
        virtual ~ConnectDisconnectInterface() {}

        virtual void OnConnectChild() = 0;

        virtual void OnDisconnectMe() = 0;

        virtual void OnDisconnectAll() = 0;
    };
    typedef AZ::EBus<ConnectDisconnectInterface> ConnectDisconnectBus;

    class ConnectDisconnectHandler
        : public ConnectDisconnectBus::Handler
    {
        ConnectDisconnectHandler* m_child;
    public:
        ConnectDisconnectHandler(ConnectDisconnectHandler* child)
            : m_child(child)
        {
            s_handlers.push_back(this);

            if (child != nullptr)  // if we are the child don't connect yet
            {
                BusConnect();
            }
        }

        ~ConnectDisconnectHandler() override
        {
            s_handlers.erase(AZStd::find(s_handlers.begin(), s_handlers.end(), this));
        }

        void OnConnectChild() override
        {
            if (m_child)
            {
                m_child->BusConnect();
            }
        }
        void OnDisconnectMe() override
        {
            BusDisconnect();
        }

        void OnDisconnectAll() override
        {
            for (size_t i = 0; i < s_handlers.size(); ++i)
            {
                s_handlers[i]->BusDisconnect();
            }
        }

        static AZStd::fixed_vector<ConnectDisconnectHandler*, 5> s_handlers;
    };

    AZStd::fixed_vector<ConnectDisconnectHandler*, 5> ConnectDisconnectHandler::s_handlers;

    class ConnectDisconnectIdOrderedInterface
        : public EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBus interface settings
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef int BusIdType;
        //////////////////////////////////////////////////////////////////////////

        ConnectDisconnectIdOrderedInterface()
            : m_order(0) {}
        virtual ~ConnectDisconnectIdOrderedInterface() {}

        virtual void OnConnectChild() = 0;

        virtual void OnDisconnectMe() = 0;

        virtual void OnDisconnectAll(int busId) = 0;

        virtual bool Compare(const ConnectDisconnectIdOrderedInterface* rhs) const         { return m_order < rhs->m_order; }

        int m_order;
    };

    typedef AZ::EBus<ConnectDisconnectIdOrderedInterface> ConnectDisconnectIdOrderedBus;

    class ConnectDisconnectIdOrderedHandler
        : public ConnectDisconnectIdOrderedBus::Handler
    {
    public:
        ConnectDisconnectIdOrderedHandler(int id, int order, ConnectDisconnectIdOrderedHandler* child)
            : ConnectDisconnectIdOrderedBus::Handler()
            , m_child(child)
            , m_busId(id)
        {
            m_order = order;
            s_handlers.push_back(this);

            if (child != nullptr)  // if we are the child don't connect yet
            {
                BusConnect(m_busId);
            }
        }

        ~ConnectDisconnectIdOrderedHandler() override
        {
            s_handlers.erase(AZStd::find(s_handlers.begin(), s_handlers.end(), this));
        }

        void OnConnectChild() override
        {
            if (m_child)
            {
                m_child->BusConnect(m_busId);
            }
        }
        void OnDisconnectMe() override
        {
            BusDisconnect();
        }

        void OnDisconnectAll(int busId) override
        {
            for (size_t i = 0; i < s_handlers.size(); ++i)
            {
                if (busId == -1 || busId == s_handlers[i]->m_busId)
                {
                    s_handlers[i]->BusDisconnect();
                }
            }
        }

        static AZStd::fixed_vector<ConnectDisconnectIdOrderedHandler*, 5> s_handlers;

    protected:
        ConnectDisconnectIdOrderedHandler* m_child;
        int                                 m_busId;
    };

    AZStd::fixed_vector<ConnectDisconnectIdOrderedHandler*, 5> ConnectDisconnectIdOrderedHandler::s_handlers;

    /**
     * Tests a bus when we allow to disconnect while executing messages.
     */
    TEST_F(EBus, DisconnectInDispatch)
    {
        ConnectDisconnectHandler child(nullptr);
        EXPECT_EQ(0, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectHandler l(&child);
        EXPECT_EQ(1, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        // Test connect in the during the message call
        ConnectDisconnectBus::Broadcast(&ConnectDisconnectBus::Events::OnConnectChild); // connect the child object

        EXPECT_EQ(2, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectBus::Broadcast(&ConnectDisconnectBus::Events::OnDisconnectAll); // Disconnect all members during a message
        EXPECT_EQ(0, ConnectDisconnectBus::GetTotalNumOfEventHandlers());


        ConnectDisconnectIdOrderedHandler ch10(10, 1, nullptr);
        ConnectDisconnectIdOrderedHandler ch5(5, 20, nullptr);
        EXPECT_EQ(0, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectIdOrderedHandler pa10(10, 10, &ch10);
        ConnectDisconnectIdOrderedHandler pa20(20, 20, &ch5);
        EXPECT_EQ(2, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectIdOrderedBus::Broadcast(&ConnectDisconnectIdOrderedBus::Events::OnConnectChild); // connect the child object
        EXPECT_EQ(4, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());

        // Disconnect all members from bus 10 (it will be sorted first)
        // This we we can test a bus removal while traversing
        ConnectDisconnectIdOrderedBus::Broadcast(&ConnectDisconnectIdOrderedBus::Events::OnDisconnectAll, 10);
        EXPECT_EQ(2, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        // Now disconnect all buses
        ConnectDisconnectIdOrderedBus::Broadcast(&ConnectDisconnectIdOrderedBus::Events::OnDisconnectAll, -1);
        EXPECT_EQ(0, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
    }


    class DisconnectNextHandlerInterface
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        using BusIdType = int32_t;

        // Comparison function which always sorts to the end
        struct DisconnectNextHandlerLess
        {
            // Intrusive_multiset requires the first_argument_type parameter for it's comparison function, but it is deprecated in C++17
            // This should be removed when C++17 support is added
            using first_argument_type = DisconnectNextHandlerInterface*;
            constexpr bool operator()(const DisconnectNextHandlerInterface*, const DisconnectNextHandlerInterface*) const
            {
                return false;
            }
        };
        using BusHandlerOrderCompare = DisconnectNextHandlerLess;

        virtual void DisconnectNextHandler() = 0;
    };

    using DisconnectNextHandlerBus = AZ::EBus<DisconnectNextHandlerInterface>;

    class DisconnectNextHandlerByIdImpl
        : public DisconnectNextHandlerBus::MultiHandler
    {
    public:
        void DisconnectNextHandler() override
        {
            if (m_nextHandler)
            {
                m_nextHandler->BusDisconnect(*DisconnectNextHandlerBus::GetCurrentBusId());
                ++m_handlerDisconnectCounter;
            }
        }

        static constexpr int32_t firstBusAddress = 1;
        static constexpr int32_t secondBusAddress = 2;
        DisconnectNextHandlerByIdImpl* m_nextHandler{};
        int32_t m_handlerDisconnectCounter{};
    };

    constexpr int32_t DisconnectNextHandlerByIdImpl::firstBusAddress;
    constexpr int32_t DisconnectNextHandlerByIdImpl::secondBusAddress;

    /**
    * Tests disconnecting the next handler within a bus during a dispatch
    */
    TEST_F(EBus, DisconnectNextHandlerDuringDispatch_DoesNotCrash)
    {
        DisconnectNextHandlerByIdImpl multiHandler1;
        multiHandler1.BusConnect(DisconnectNextHandlerByIdImpl::firstBusAddress);
        multiHandler1.BusConnect(DisconnectNextHandlerByIdImpl::secondBusAddress);

        DisconnectNextHandlerByIdImpl multiHandler2;
        multiHandler2.BusConnect(DisconnectNextHandlerByIdImpl::firstBusAddress);
        multiHandler2.BusConnect(DisconnectNextHandlerByIdImpl::secondBusAddress);

        // Set the first handler m_nextHandler field to point to the second handler
        multiHandler1.m_nextHandler = &multiHandler2;

        // Disconnect the next handlers from the second bus address to catch any issues with the address hash_table iterators becoming invalidated
        DisconnectNextHandlerBus::Event(DisconnectNextHandlerByIdImpl::secondBusAddress, &DisconnectNextHandlerInterface::DisconnectNextHandler);
        EXPECT_EQ(1, multiHandler1.m_handlerDisconnectCounter);
        EXPECT_EQ(0, multiHandler2.m_handlerDisconnectCounter);

        DisconnectNextHandlerBus::Event(DisconnectNextHandlerByIdImpl::firstBusAddress, &DisconnectNextHandlerInterface::DisconnectNextHandler);
        EXPECT_EQ(2, multiHandler1.m_handlerDisconnectCounter);
        EXPECT_EQ(0, multiHandler2.m_handlerDisconnectCounter);
    }

    class DisconnectNextAddressInterface
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ByIdAndOrdered;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = int32_t;
        struct BusIdOrderLess
        {
            constexpr bool operator()(BusIdType lhs, BusIdType rhs) const
            {
                return lhs < rhs;
            }
        };
        using BusIdOrderCompare = BusIdOrderLess;

        virtual void DisconnectNextAddress() = 0;
    };

    using DisconnectNextAddressBus = AZ::EBus<DisconnectNextAddressInterface>;

    class DisconnectNextAddressImpl
        : public DisconnectNextAddressBus::Handler
    {
    public:
        void DisconnectNextAddress() override
        {
            if (m_nextAddressHandler)
            {
                m_nextAddressHandler->BusDisconnect();
                ++m_addressDisconnectCounter;
            }
        }

        static constexpr int32_t firstBusAddress = 1;
        static constexpr int32_t nextBusAddress = 2;
        DisconnectNextAddressImpl* m_nextAddressHandler{};
        int32_t m_addressDisconnectCounter{};
    };

    constexpr int32_t DisconnectNextAddressImpl::firstBusAddress;
    constexpr int32_t DisconnectNextAddressImpl::nextBusAddress;
    /**
    * Tests disconnecting the next address within a bus during a dispatch
    */
    TEST_F(EBus, DisconnectNextAddressDuringDispatch_DoesNotCrash)
    {
        DisconnectNextAddressImpl addressHandler1;
        addressHandler1.BusConnect(DisconnectNextAddressImpl::firstBusAddress);

        DisconnectNextAddressImpl addressHandler2;
        addressHandler2.BusConnect(DisconnectNextAddressImpl::nextBusAddress);
        addressHandler1.m_nextAddressHandler = &addressHandler2;

        // Disconnect the second address handler using the first address handler
        DisconnectNextAddressBus::Event(DisconnectNextAddressImpl::firstBusAddress, &DisconnectNextAddressInterface::DisconnectNextAddress);
        EXPECT_EQ(1, addressHandler1.m_addressDisconnectCounter);
        EXPECT_EQ(0, addressHandler2.m_addressDisconnectCounter);
    }

    /**
     * Test multiple handler.
     */
    namespace MultiHandlerTest
    {
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus Settings
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef unsigned int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}

            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void   OnAction() = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus<MyEventGroup> MyEventBus;

        class MultiHandler
            : public MyEventBus::MultiHandler
        {
        public:
            MultiHandler()
                : m_expectedCurrentId(0)
                , m_numCalls(0)
            {}

            void OnAction() override
            {
                const unsigned int* currentIdPtr = MyEventBus::GetCurrentBusId();
                ASSERT_NE(nullptr, currentIdPtr);
                EXPECT_EQ(*currentIdPtr, m_expectedCurrentId);
                ++m_numCalls;
            }

            unsigned int m_expectedCurrentId;
            unsigned int m_numCalls;
        };
    }
    TEST_F(EBus, MultiHandler)
    {
        using namespace MultiHandlerTest;
        MultiHandler ml;
        ml.BusConnect(10);
        ml.BusConnect(12);
        ml.BusConnect(13);

        // test copy handlers and make sure they attached to the same bus
        MultiHandler mlCopy = ml;
        EXPECT_EQ(0, mlCopy.m_numCalls);

        // Called outside of an even it should always return nullptr
        EXPECT_EQ(nullptr, MyEventBus::GetCurrentBusId());

        MyEventBus::Event(1, &MyEventBus::Events::OnAction);  // this should not trigger a call
        EXPECT_EQ(0, ml.m_numCalls);

        // Issues calls which we listen for
        ml.m_expectedCurrentId = 10;
        mlCopy.m_expectedCurrentId = 10;
        MyEventBus::Event(10, &MyEventBus::Events::OnAction);
        EXPECT_EQ(1, ml.m_numCalls);
        EXPECT_EQ(1, mlCopy.m_numCalls);  // make sure the handler copy is connected
        mlCopy.BusDisconnect();

        ml.m_expectedCurrentId = 12;
        MyEventBus::Event(12, &MyEventBus::Events::OnAction);
        EXPECT_EQ(2, ml.m_numCalls);

        ml.m_expectedCurrentId = 13;
        MyEventBus::Event(13, &MyEventBus::Events::OnAction);
        EXPECT_EQ(3, ml.m_numCalls);
    }

    // Non intrusive EBusTraits
    struct MyCustomTraits
        : public AZ::EBusTraits
    {
        // ... custom traits here
    };

    /**
     *  Interface that we don't own and we can't inherit traits
     */
    class My3rdPartyInterface
    {
    public:
        virtual void SomeEvent(int a) = 0;
    };

    // 3rd party interface (which is compliant with EBus requirements)
    typedef AZ::EBus<My3rdPartyInterface, MyCustomTraits> My3rdPartyBus1;

    // 3rd party interface that we want to wrap
    class My3rdPartyInterfaceWrapped
        : public My3rdPartyInterface
        , public AZ::EBusTraits
    {
    };

    typedef AZ::EBus<My3rdPartyInterfaceWrapped> My3rdPartyBus2;

    // regular interface trough traits inheritance, please look at the all the samples above

    // combine an ebus and an interface, so you don't need any typedefs. You will need to specialize a template so the bus can get it's traits
    // Keep in mind that this type will not allow for interfaces to be extended, but it's ok for final interfaces
    class MyEBusInterface
        : public AZ::EBus<MyEBusInterface, MyCustomTraits>
    {
    public:
        virtual void Event(int a) const = 0;
    };

    /**
      * Test and demonstrate different EBus implementations
      */
    namespace ImplementationTest
    {
        class Handler1
            : public My3rdPartyBus1::Handler
        {
        public:
            Handler1()
                : m_calls(0)
            {
                My3rdPartyBus1::Handler::BusConnect();
            }

            int m_calls;

        private:
            void SomeEvent(int a) override
            {
                (void)a;
                ++m_calls;
            }
        };

        class Handler2
            : public My3rdPartyBus2::Handler
        {
        public:
            Handler2()
                : m_calls(0)
            {
                My3rdPartyBus2::Handler::BusConnect();
            }

            int m_calls;

        private:
            void SomeEvent(int a) override
            {
                (void)a;
                ++m_calls;
            }
        };

        class Handler3
            : public MyEBusInterface::Handler
        {
        public:
            Handler3()
                : m_calls(0)
            {
                MyEBusInterface::Handler::BusConnect();
            }

            mutable int m_calls;

        private:
            void Event(int a) const override
            {
                (void)a;
                ++m_calls;
            }
        };
    }
    TEST_F(EBus, ExternalInterface)
    {
        using namespace ImplementationTest;
        Handler1 h1;
        Handler2 h2;
        Handler3 h3;

        // test copy of handler
        Handler1 h1Copy = h1;
        EXPECT_EQ(0, h1Copy.m_calls);

        My3rdPartyBus1::Broadcast(&My3rdPartyBus1::Events::SomeEvent, 1);
        EXPECT_EQ(1, h1.m_calls);
        EXPECT_EQ(1, h1Copy.m_calls);  // check that the copy works too
        My3rdPartyBus2::Broadcast(&My3rdPartyBus2::Events::SomeEvent, 2);
        EXPECT_EQ(1, h2.m_calls);
        MyEBusInterface::Broadcast(&MyEBusInterface::Events::Event, 3);
        EXPECT_EQ(1, h3.m_calls);
    }

    /**
    *
    */
    TEST_F(EBus, Results)
    {
        // Test the result logical aggregator for OR
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool> > or_false_false(false);
            or_false_false = false;
            or_false_false = false;
            EXPECT_FALSE(or_false_false.value);
        }

        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool> > or_true_false(false);
            or_true_false = true;
            or_true_false = false;
            EXPECT_TRUE(or_true_false.value);
        }

        // Test the result logical aggregator for AND
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_and<bool> > and_true_false(true);
            and_true_false = true;
            and_true_false = false;
            EXPECT_FALSE(and_true_false.value);
        }

        {
            AZ::EBusLogicalResult<bool, AZStd::logical_and<bool> > and_true_true(true);
            and_true_true = true;
            and_true_true = true;
            EXPECT_TRUE(and_true_true.value);
        }
    }

    // Routers, Bridging and Versioning

    /**
     * EBusInterfaceV1, since we want to keep binary compatibility (we don't need to recompile)
     * when we are implementing the version messaging we should not change the V1 EBus, all code
     * should be triggered from the new version that is not compiled is customer's code yet.
     */
    class EBusInterfaceV1 : public AZ::EBusTraits
    {
    public:
        virtual void OnEvent(int a)
        {
            (void)a;
        }
    };

    using EBusVersion1 = AZ::EBus<EBusInterfaceV1>;

    /**
     * Version 2 of the interface which communicates with Version 1 of the bus bidirectionally.
     */
    class EBusInterfaceV2 : public AZ::EBusTraits
    {
    public:
        /**
         * Router policy implementation that bridges two EBuses by default.
         * It this case we use it to implement versioning between V1 and V2
         * of a specific EBus version.
         */
        template<typename Bus>
        struct RouterPolicy : public EBusRouterPolicy<Bus>
        {
            struct V2toV1Router : public Bus::NestedVersionRouter
            {
                void OnEvent(int a, int b) override
                {
                    if (!m_policy->m_isOnEventRouting)
                    {
                        m_policy->m_isOnEventRouting = true;
                        this->template ForwardEvent<EBusVersion1>(&EBusVersion1::Events::OnEvent, a + b);
                        m_policy->m_isOnEventRouting = false;
                    }
                }

                typename Bus::RouterPolicy* m_policy = nullptr;
            };

            struct V1toV2Router : public EBusVersion1::Router
            {
                void OnEvent(int a) override
                {
                    if(!m_policy->m_isOnEventRouting)
                    {
                        m_policy->m_isOnEventRouting = true;
                        this->template ForwardEvent<Bus>(&Bus::Events::OnEvent, a, 0);
                        m_policy->m_isOnEventRouting = false;
                    }
                }

                typename Bus::RouterPolicy* m_policy = nullptr;
            };

            RouterPolicy()
            {
                m_v2toV1Router.m_policy = this;
                m_v1toV2Router.m_policy = this;
                m_v2toV1Router.BusRouterConnect(this->m_routers);
                m_v1toV2Router.BusRouterConnect();
            }

            ~RouterPolicy()
            {
                m_v2toV1Router.BusRouterDisconnect(this->m_routers);
                m_v1toV2Router.BusRouterDisconnect();
            }

            // State of current routed events to avoid loopbacks
            // this is NOT needed if we route only one way V2->V1 or V1->V2
            bool m_isOnEventRouting = false;

            // Possible optimization, When we are dealing with version we usually don't expect to have active use of the old version,
            // it's just for compatibility. Having routers trying to route to old version busses that rarely
            // have listeners will have it's overhead. To reduct that we can add m_onDemandRouters list that
            // have a pointer to a router and oder, so we can automatically connect that router only when
            // listeners are attached to the old version of the bus. We are talking only about NewVersion->OldVersion
            // bridge (the opposite can be always connected as the overhead will be on the OldVersion bus which we don't expect to use much anyway).
            V2toV1Router m_v2toV1Router;
            V1toV2Router m_v1toV2Router;
        };

        virtual void OnEvent(int a, int b) { (void)a; (void)b; }
    };

    using EBusVersion2 = AZ::EBus<EBusInterfaceV2>;

    namespace RoutingTest
    {
        class EBusInterceptor : public EBusVersion1::Router
        {
        public:
            void OnEvent(int a) override
            {
                EXPECT_EQ(1020, a);
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };

        class V1EventRouter : public EBusVersion1::Router
        {
        public:
            void OnEvent(int a) override
            {
                (void)a;
                m_numOnEvent++;
                EBusVersion1::SetRouterProcessingState(m_processingState);
            }

            int m_numOnEvent = 0;
            EBusVersion1::RouterProcessingState m_processingState = EBusVersion1::RouterProcessingState::SkipListeners;
        };

        class EBusVersion1Handler : public EBusVersion1::Handler
        {
        public:
            void OnEvent(int a) override
            {
                (void)a;
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };

        class EBusVersion2Handler : public EBusVersion2::Handler
        {
        public:
            void OnEvent(int a, int b) override
            {
                (void)a; (void)b;
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };
    }

#if !AZ_TRAIT_DISABLE_FAILED_EBUS_ROUTING_TEST
    TEST_F(EBus, Routing)
    {
        using namespace RoutingTest;
        EBusInterceptor interceptor;
        EBusVersion1Handler v1Handler;

        v1Handler.BusConnect();
        interceptor.BusRouterConnect();

        EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
        EXPECT_EQ(1, interceptor.m_numOnEvent);
        EXPECT_EQ(1, v1Handler.m_numOnEvent);

        interceptor.BusRouterDisconnect();

        EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
        EXPECT_EQ(1, interceptor.m_numOnEvent);
        EXPECT_EQ(2, v1Handler.m_numOnEvent);

        // routing events
        {
            // reset counter
            v1Handler.m_numOnEvent = 0;

            V1EventRouter v1Router;
            v1Router.BusRouterConnect();

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(1, v1Router.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);

            v1Router.BusRouterDisconnect();
        }

        // routing events and skipping further routing
        {
            // reset counter
            v1Handler.m_numOnEvent = 0;

            V1EventRouter v1RouterFirst, v1RouterSecond;
            v1RouterFirst.BusRouterConnect(-1);
            v1RouterSecond.BusRouterConnect();

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(1, v1RouterFirst.m_numOnEvent);
            EXPECT_EQ(1, v1RouterSecond.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);

            // now instruct router 1 to block any further event processing
            v1RouterFirst.m_processingState = EBusVersion1::RouterProcessingState::SkipListenersAndRouters;

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(2, v1RouterFirst.m_numOnEvent);
            EXPECT_EQ(1, v1RouterSecond.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);
        }

        // test bridging two EBus by using routers. This can be used to handle different bus versions.
        {
            EBusVersion2Handler v2Handler;
            v2Handler.BusConnect();

            // reset counter
            v1Handler.m_numOnEvent = 0;

            EBusVersion2::Broadcast(&EBusVersion2::Events::OnEvent, 10, 20);
            EXPECT_EQ(1, v1Handler.m_numOnEvent);
            EXPECT_EQ(1, v2Handler.m_numOnEvent);

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 30);
            EXPECT_EQ(2, v1Handler.m_numOnEvent);
            EXPECT_EQ(2, v2Handler.m_numOnEvent);
        }

        // We can test Queue and Event routing separately,
        // however they do use the same code path (as we don't queue routing and we just use the ID to differentiate between Broadcast and Event)

    }
#endif // !AZ_TRAIT_DISABLE_FAILED_EBUS_ROUTING_TEST

    struct LocklessEvents
        : public AZ::EBusTraits
    {
        using MutexType = AZStd::mutex;
        static const bool LocklessDispatch = true;

        virtual ~LocklessEvents() = default;
        virtual void RemoveMe() = 0;
        virtual void DeleteMe() = 0;
        virtual void Calculate(int x, int y, int z) = 0;
    };

    using LocklessBus = AZ::EBus<LocklessEvents>;

    struct LocklessImpl
        : public LocklessBus::Handler
    {
        uint32_t m_val;
        uint32_t m_maxSleep;
        LocklessImpl(uint32_t maxSleep = 0)
            : m_val(0)
            , m_maxSleep(maxSleep)
        {
            BusConnect();
        }

        ~LocklessImpl() override
        {
            BusDisconnect();
        }

        void RemoveMe() override
        {
            BusDisconnect();
        }
        void DeleteMe() override
        {
            delete this;
        }
        void Calculate(int x, int y, int z) override
        {
            m_val = x + (y * z);
            if (m_maxSleep)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(m_val % m_maxSleep));
            }
        }
    };

    void ThrashLocklessDispatch(uint32_t maxSleep = 0)
    {
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        LocklessImpl handler(maxSleep);

        auto work = [maxSleep]()
        {
            char sentinel[64] = { 0 };
            char* end = sentinel + AZ_ARRAY_SIZE(sentinel);
            for (int i = 1; i < cycleCount; ++i)
            {
                // Calculate() already includes a modulo-cycled sleep, add more random jitter
                uint32_t extraSleep_us = maxSleep ? rand() % maxSleep : 0;
                if (extraSleep_us % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(extraSleep_us));
                }
                LocklessBus::Broadcast(&LocklessBus::Events::Calculate, i, i * 2, i << 4);
                bool failed = (AZStd::find_if(&sentinel[0], end, [](char s) { return s != 0; }) != end);
                EXPECT_FALSE(failed);
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(EBus, ThrashLocklessDispatchYOLO)
    {
        ThrashLocklessDispatch();
    }

    TEST_F(EBus, ThrashLocklessDispatchSimulateWork)
    {
        ThrashLocklessDispatch(4);
    }

    TEST_F(EBus, DisconnectInLocklessDispatch)
    {
        LocklessImpl handler;
        AZ_TEST_START_TRACE_SUPPRESSION;
        LocklessBus::Broadcast(&LocklessBus::Events::RemoveMe);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(EBus, DeleteInLocklessDispatch)
    {
        LocklessImpl* handler = new LocklessImpl();
        AZ_UNUSED(handler);
        AZ_TEST_START_TRACE_SUPPRESSION;
        LocklessBus::Broadcast(&LocklessBus::Events::DeleteMe);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    namespace LocklessTest
    {
        struct LocklessConnectorEvents
            : public AZ::EBusTraits
        {
            using MutexType = AZStd::recursive_mutex;
            static const bool LocklessDispatch = true;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef uint32_t BusIdType;

            virtual ~LocklessConnectorEvents() = default;
            virtual void DoConnect() = 0;
            virtual void DoDisconnect() = 0;
        };

        using LocklessConnectorBus = AZ::EBus<LocklessConnectorEvents>;

        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef uint32_t BusIdType;

            virtual void Calculate(int x, int y, int z) = 0;

            virtual ~MyEventGroup() {}
        };

        using MyEventGroupBus = AZ::EBus< MyEventGroup >;

        struct DoubleEbusImpl
            : public LocklessConnectorBus::Handler,
            MyEventGroupBus::Handler
        {
            uint32_t m_id;
            uint32_t m_val;
            uint32_t m_maxSleep;

            DoubleEbusImpl(uint32_t id, uint32_t maxSleep)
                : m_id(id)
                , m_val(0)
                , m_maxSleep(maxSleep)
            {
                LocklessConnectorBus::Handler::BusConnect(m_id);
            }

            ~DoubleEbusImpl() override
            {
                MyEventGroupBus::Handler::BusDisconnect();
                LocklessConnectorBus::Handler::BusDisconnect();
            }

            void Calculate(int x, int y, int z) override
            {
                m_val = x + (y * z);
                if (m_maxSleep)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(m_val % m_maxSleep));
                }
            }

            void DoConnect() override
            {
                MyEventGroupBus::Handler::BusConnect(m_id);
            }

            void DoDisconnect() override
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }
        };
    }

    TEST_F(EBus, MixedLocklessTest)
    {
        using namespace LocklessTest;

        const int maxSleep = 5;
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 500 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        AZStd::vector<DoubleEbusImpl> handlerList;

        for (int i = 0; i < threadCount; i++)
        {
            handlerList.emplace_back(i, maxSleep);
        }

        auto work = []()
        {
            char sentinel[64] = { 0 };
            char* end = sentinel + AZ_ARRAY_SIZE(sentinel);
            for (int i = 1; i < cycleCount; ++i)
            {
                uint32_t id = rand() % threadCount;

                LocklessConnectorBus::Event(id, &LocklessConnectorBus::Events::DoConnect);

                // Calculate() already includes a modulo-cycled sleep, add more random jitter
                uint32_t extraSleep_us = maxSleep ? rand() % maxSleep : 0;
                if (extraSleep_us % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(extraSleep_us));
                }

                MyEventGroupBus::Event(id, &MyEventGroupBus::Events::Calculate, i, i * 2, i << 4);

                LocklessConnectorBus::Event(id, &LocklessConnectorBus::Events::DoDisconnect);

                bool failed = (AZStd::find_if(&sentinel[0], end, [](char s) { return s != 0; }) != end);
                EXPECT_FALSE(failed) << "sentinel memory unexpectedly tampered with while handling EBus events";
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    namespace MultithreadConnect
    {
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            virtual ~MyEventGroup() {}
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        struct MyEventGroupImpl :
            MyEventGroupBus::Handler
        {
            MyEventGroupImpl()
            {

            }

            ~MyEventGroupImpl() override
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }

            virtual void DoConnect()
            {
                MyEventGroupBus::Handler::BusConnect();
            }

            virtual void DoDisconnect()
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }
        };
    }

    TEST_F(EBus, MultithreadConnectTest)
    {
        using namespace MultithreadConnect;

        const int maxSleep = 5;
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        MyEventGroupImpl handler;

        auto work = [&handler]()
        {
            for (int i = 1; i < cycleCount; ++i)
            {
                handler.DoConnect();

                // add random jitter
                uint32_t extraSleep_us = maxSleep ? rand() % maxSleep : 0;
                if (extraSleep_us % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(extraSleep_us));
                }

                handler.DoDisconnect();
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    struct LocklessNullMutexEvents
        : public AZ::EBusTraits
    {
        using MutexType = AZ::NullMutex;
        static const bool LocklessDispatch = true;

        virtual ~LocklessNullMutexEvents() = default;
        virtual void AtomicIncrement() = 0;
    };

    using LocklessNullMutexBus = AZ::EBus<LocklessNullMutexEvents>;

    struct LocklessNullMutexImpl
        : public LocklessNullMutexBus::Handler
    {
        AZStd::atomic<uint64_t> m_val{};
        LocklessNullMutexImpl()
        {
            BusConnect();
        }

        ~LocklessNullMutexImpl() override
        {
            BusDisconnect();
        }

        void AtomicIncrement() override
        {
            ++m_val;
        }
    };

    void ThrashLocklessDispatchNullMutex()
    {
        constexpr size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        constexpr uint64_t expectedAtomicCount = threadCount * cycleCount;
        AZStd::thread threads[threadCount];

        LocklessNullMutexImpl handler;

        auto work = []()
        {
            for (int i = 0; i < cycleCount; ++i)
            {
                // add random jitter
                constexpr int maxSleep = 3;
                uint32_t extraSleep_us = rand() % maxSleep;
                if (extraSleep_us != 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(extraSleep_us));
                }
                LocklessNullMutexBus::Broadcast(&LocklessNullMutexBus::Events::AtomicIncrement);
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        EXPECT_EQ(expectedAtomicCount, static_cast<uint64_t>(handler.m_val));
    }

    TEST_F(EBus, LocklessDispatchWithNullMutex_Multithread_Thrash)
    {
        ThrashLocklessDispatchNullMutex();
    }

    namespace EBusResultsTest
    {
        class ResultClass
        {
        public:
            int m_value1 = 0;
            int m_value2 = 0;

            bool m_operator_called_const = false;
            bool m_operator_called_rvalue_ref = false;

            ResultClass() = default;
            ResultClass(const ResultClass&) = default;

            bool operator==(const ResultClass& b) const
            {
                return m_value1 == b.m_value1 && m_value2 == b.m_value2;
            }

            ResultClass& operator=(const ResultClass& b)
            {
                m_value1 = b.m_value1 + m_value1;
                m_value2 = b.m_value2 + m_value2;
                m_operator_called_const = true;
                m_operator_called_rvalue_ref = b.m_operator_called_rvalue_ref;
                return *this;
            }

            ResultClass& operator=(ResultClass&& b)
            {
                // combine together to prove its not just an assignment
                m_value1 = b.m_value1 + m_value1;
                m_value2 = b.m_value2 + m_value2;

                // but destroy the original value (emulating move op)
                b.m_value1 = 0;
                b.m_value2 = 0;

                m_operator_called_rvalue_ref = true;
                m_operator_called_const = b.m_operator_called_const;
                return *this;
            }
        };

        class ResultReducerClass
        {
        public:
            bool m_operator_called_const = false;
            bool m_operator_called_rvalue_ref = false;

            ResultClass operator()(const ResultClass& a, const ResultClass& b)
            {
                ResultClass newValue;
                newValue.m_value1 = a.m_value1 + b.m_value1;
                newValue.m_value2 = a.m_value2 + b.m_value2;
                m_operator_called_const = true;
                return newValue;
            }

            ResultClass operator()(const ResultClass& a, ResultClass&& b)
            {
                m_operator_called_rvalue_ref = true;
                ResultClass newValue;
                newValue.m_value1 = a.m_value1 + b.m_value1;
                newValue.m_value2 = a.m_value2 + b.m_value2;
                return newValue;
            }
        };

        class MyInterface
        {
        public:
            virtual ResultClass EventX() = 0;
            virtual const ResultClass& EventY() = 0;
        };

        using MyInterfaceBus = AZ::EBus<MyInterface, AZ::EBusTraits>;

        class MyListener : public MyInterfaceBus::Handler
        {
        public:
            MyListener(int value1, int value2)
            {
                m_result.m_value1 = value1;
                m_result.m_value2 = value2;
            }

            ~MyListener() override
            {
            }

            ResultClass EventX() override
            {
                return m_result;
            }

            const ResultClass& EventY() override
            {
                return m_result;
            }

            ResultClass m_result;
        };

    } // EBusResultsTest

    TEST_F(EBus, ResultsTest)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            ResultClass results;
            MyInterfaceBus::BroadcastResult(results, &MyInterfaceBus::Events::EventX);

            // ensure that the RVALUE-REF op was called:
            EXPECT_FALSE(results.m_operator_called_const);
            EXPECT_TRUE(results.m_operator_called_rvalue_ref);
            EXPECT_EQ(results.m_value1, 4); // 1 + 3
            EXPECT_EQ(results.m_value2, 6); // 2 + 4
            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            ResultClass results;
            MyInterfaceBus::BroadcastResult(results, &MyInterfaceBus::Events::EventY);

            // ensure that the const version of operator= was called.
            EXPECT_TRUE(results.m_operator_called_const);
            EXPECT_FALSE(results.m_operator_called_rvalue_ref);
            EXPECT_EQ(results.m_value1, 4); // 1 + 3
            EXPECT_EQ(results.m_value2, 6); // 2 + 4
            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }


        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values.
    TEST_F(EBus, ResultsTest_ReducerCorruption)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            EBusReduceResult<ResultClass, ResultReducerClass> resultreducer;
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventX);
            EXPECT_FALSE(resultreducer.unary.m_operator_called_const);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_rvalue_ref);

            // note that operator= is called TWICE here.  one on (val1+val2)
            // because the ebus results is defined as "value = unary(a, b)"
            // and in this case both operator = as well as the unary operate here.
            // meaning that the addition is actually run multiple times
            // once for (a+b) and then again, during value = unary(...) for a second time

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            EBusReduceResult<ResultClass, ResultReducerClass> resultreducer;
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventY);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_const); // we expect the const version to have been called this time
            EXPECT_FALSE(resultreducer.unary.m_operator_called_rvalue_ref);
            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }
        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values and operates correctly
    // even if the other form is used (where T is T&)
    TEST_F(EBus, ResultsTest_ReducerCorruption_Ref)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            ResultClass finalResult;
            EBusReduceResult<ResultClass&, ResultReducerClass> resultreducer(finalResult);

            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventX);
            EXPECT_FALSE(resultreducer.unary.m_operator_called_const);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_rvalue_ref);

            EXPECT_FALSE(finalResult.m_operator_called_const);
            EXPECT_TRUE(finalResult.m_operator_called_rvalue_ref);

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            ResultClass finalResult;
            EBusReduceResult<ResultClass&, ResultReducerClass> resultreducer(finalResult);
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventY);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_const);  // EventY is const, so we expect this to have happened again
            EXPECT_FALSE(resultreducer.unary.m_operator_called_rvalue_ref);

            // we still expect the actual finalresult to have been populated via RVALUE REF MOVE
            EXPECT_FALSE(finalResult.m_operator_called_const);
            EXPECT_TRUE(finalResult.m_operator_called_rvalue_ref);

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }
        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values.
    TEST_F(EBus, ResultsTest_AggregatorCorruption)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            EBusAggregateResults<ResultClass> resultarray;
            MyInterfaceBus::BroadcastResult(resultarray, &MyInterfaceBus::Events::EventX);
            EXPECT_EQ(resultarray.values.size(), 2);
            // bus connection is unordered, so we just have to find the two values on it, can't assume they're in same order.
            EXPECT_TRUE(resultarray.values[0] == val1.m_result || resultarray.values[1] == val1.m_result);
            EXPECT_TRUE(resultarray.values[0] == val2.m_result || resultarray.values[1] == val2.m_result);

            if (resultarray.values[0] == val1.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val2.m_result);
            }

            if (resultarray.values[0] == val2.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val1.m_result);
            }

            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            EBusAggregateResults<ResultClass> resultarray;
            MyInterfaceBus::BroadcastResult(resultarray, &MyInterfaceBus::Events::EventY);
            // bus connection is unordered, so we just have to find the two values on it, can't assume they're in same order.
            EXPECT_TRUE(resultarray.values[0] == val1.m_result || resultarray.values[1] == val1.m_result);
            EXPECT_TRUE(resultarray.values[0] == val2.m_result || resultarray.values[1] == val2.m_result);

            if (resultarray.values[0] == val1.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val2.m_result);
            }

            if (resultarray.values[0] == val2.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val1.m_result);
            }

            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }


        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    namespace EBusEnvironmentTest
    {
        class MyInterface
        {
        public:
            virtual void EventX() = 0;
        };

        using MyInterfaceBus = AZ::EBus<MyInterface, AZ::EBusTraits>;

        class MyInterfaceListener : public MyInterfaceBus::Handler
        {
        public:
            MyInterfaceListener(int environmentId = -1)
                : m_environmentId(environmentId)
                , m_numEventsX(0)
            {
            }

            void EventX() override
            {
                ++m_numEventsX;
            }

            int m_environmentId; ///< EBus environment id. -1 is global, otherwise index in the environment array.
            int m_numEventsX;
        };

        class ParallelSeparateEBusEnvironmentProcessor
        {
        public:

            using JobaToProcessArray = AZStd::vector<ParallelSeparateEBusEnvironmentProcessor, AZ::OSStdAllocator>;

            ParallelSeparateEBusEnvironmentProcessor()
            {
                m_busEvironment = AZ::EBusEnvironment::Create();
            }

            ~ParallelSeparateEBusEnvironmentProcessor()
            {
                AZ::EBusEnvironment::Destroy(m_busEvironment);
            }

            void ProcessSomethingInParallel(size_t jobId)
            {
                m_busEvironment->ActivateOnCurrentThread();

                EXPECT_EQ(0, MyInterfaceBus::GetTotalNumOfEventHandlers()); // If environments are properly separated we should have no listeners!"

                MyInterfaceListener uniqueListener((int)jobId);
                uniqueListener.BusConnect();

                const int numEventsToBroadcast = 100;

                for (int i = 0; i < numEventsToBroadcast; ++i)
                {
                    // from now on all EBus calls happen in unique environment
                    MyInterfaceBus::Broadcast(&MyInterfaceBus::Events::EventX);
                }

                uniqueListener.BusDisconnect();

                // Test that we have only X num events
                EXPECT_EQ(uniqueListener.m_numEventsX, numEventsToBroadcast); // If environments are properly separated we should get only the events from our environment!

                m_busEvironment->DeactivateOnCurrentThread();
            }

            static void ProcessJobsRange(JobaToProcessArray* jobs, size_t startIndex, size_t endIndex)
            {
                for (size_t i = startIndex; i <= endIndex; ++i)
                {
                    (*jobs)[i].ProcessSomethingInParallel(i);
                }
            }

            AZ::EBusEnvironment* m_busEvironment;
        };
    } // EBusEnvironmentTest

    TEST_F(EBus, EBusEnvironment)
    {
        using namespace EBusEnvironmentTest;
        ParallelSeparateEBusEnvironmentProcessor::JobaToProcessArray jobsToProcess;
        jobsToProcess.resize(10000);

        MyInterfaceListener globalListener;
        globalListener.BusConnect();

        // broadcast on global bus
        MyInterfaceBus::Broadcast(&MyInterfaceBus::Events::EventX);

        // spawn a few threads to process those jobs
        AZStd::thread thread1(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 0, 1999));
        AZStd::thread thread2(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 2000, 3999));
        AZStd::thread thread3(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 4000, 5999));
        AZStd::thread thread4(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 6000, 7999));
        AZStd::thread thread5(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 8000, 9999));

        thread5.join();
        thread4.join();
        thread3.join();
        thread2.join();
        thread1.join();

        globalListener.BusDisconnect();

        EXPECT_EQ(1, globalListener.m_numEventsX); // If environments are properly separated we should get only the events the global/default Environment!
    }

    // Test disconnecting while in ConnectionPolicy
    class BusWithConnectionPolicy
        : public AZ::EBusTraits
    {
    public:
        virtual ~BusWithConnectionPolicy() = default;

        virtual void MessageWhichOccursDuringConnect() = 0;

        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr&, typename Bus::Context&, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& , const typename Bus::BusIdType&)
            {
                handler->MessageWhichOccursDuringConnect();
            }
        };
    };
    using BusWithConnectionPolicyBus = AZ::EBus<BusWithConnectionPolicy>;


    class HandlerWhichDisconnectsDuringDelivery
        : public BusWithConnectionPolicyBus::Handler
    {
        void MessageWhichOccursDuringConnect() override
        {
            BusDisconnect();
        }
    };


    TEST_F(EBus, ConnectionPolicy_DisconnectDuringDelivery)
    {
        HandlerWhichDisconnectsDuringDelivery handlerTest;
        handlerTest.BusConnect();
    }

    class BusWithConnectionPolicyUnlocksBeforeHandler
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        virtual ~BusWithConnectionPolicyUnlocksBeforeHandler() = default;

        virtual int GetPreUnlockDelay() const { return 0; }
        virtual int GetPostUnlockDelay() const { return 0; }
        virtual bool ShouldUnlock() const { return true; }
        virtual void MessageWhichOccursDuringConnect() = 0;

        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr&, typename Bus::Context&, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType&)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(handler->GetPreUnlockDelay()));

                if (handler->ShouldUnlock())
                {
                    connectLock.unlock();
                }

                AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(handler->GetPostUnlockDelay()));
                handler->MessageWhichOccursDuringConnect();
            }
        };
    };
    using BusWithConnectionPolicyUnlocksBus = AZ::EBus<BusWithConnectionPolicyUnlocksBeforeHandler>;

    class DelayUnlockHandler
        : public BusWithConnectionPolicyUnlocksBus::Handler
    {
    public:
        DelayUnlockHandler() = default;
        DelayUnlockHandler(int preDelay, int postDelay) :
            m_preDelay(preDelay),
            m_postDelay(postDelay)
        {

        }
        void MessageWhichOccursDuringConnect() override
        {
            if (m_connectMethod)
            {
                m_connectMethod();
            }
            m_didConnect = true;
        }

        int GetPreUnlockDelay() const override { return m_preDelay; }
        int GetPostUnlockDelay() const override { return m_postDelay; }
        bool ShouldUnlock() const override { return m_shouldUnlock; }

        bool m_shouldUnlock{ true };
        AZStd::atomic_bool m_didConnect{ false };

        int m_preDelay{ 0 };
        int m_postDelay{ 0 };
        AZStd::function<void()> m_connectMethod;
    };

    TEST_F(EBus, ConnectionPolicy_DisconnectDuringDeliveryUnlocked_Success)
    {
        DelayUnlockHandler handlerTest;
        handlerTest.m_connectMethod = [&handlerTest]() { handlerTest.BusDisconnect(); };
        handlerTest.BusConnect();
        EXPECT_EQ(handlerTest.m_didConnect, true);
    }

    TEST_F(EBus, ConnectionPolicy_DisconnectDuringDeliveryDelayUnlocked_Success)
    {
        constexpr int numTests = 100;
        for (int disconnectTest = 0; disconnectTest < numTests; ++disconnectTest)
        {
            DelayUnlockHandler handlerTest(0, 1);
            handlerTest.BusConnect();
            AZStd::thread disconnectThread([&handlerTest]()
            {
                handlerTest.BusDisconnect();
            }
            );
            disconnectThread.join();
            EXPECT_EQ(handlerTest.m_didConnect, true);
        }
    }

    TEST_F(EBus, ConnectionPolicy_DisconnectDuringDeliveryPreDelayUnlocked_Success)
    {
        constexpr int numTests = 100;
        for (int disconnectTest = 0; disconnectTest < numTests; ++disconnectTest)
        {
            DelayUnlockHandler handlerTest(1, 0);
            handlerTest.BusConnect();
            AZStd::thread disconnectThread([&handlerTest]()
            {
                handlerTest.BusDisconnect();
            }
            );
            disconnectThread.join();
            EXPECT_EQ(handlerTest.m_didConnect, true);
        }
    }

    TEST_F(EBus, ConnectionPolicy_WaitOnSecondHandlerWhileStillLocked_CantComplete)
    {
        DelayUnlockHandler waitHandler;
        // Test without releasing the lock - this is expected to prevent our second handler from connecting
        // so will block this thread
        waitHandler.m_shouldUnlock = false;

        DelayUnlockHandler connectHandler;
        waitHandler.m_connectMethod = [&connectHandler]()
        {
            constexpr int waitMsMax = 100;
            auto startTime = AZStd::chrono::steady_clock::now();
            auto endTime = startTime + AZStd::chrono::milliseconds(waitMsMax);

            // The other bus should not be able to complete because we're still holding the connect lock
            while (AZStd::chrono::steady_clock::now() < endTime && !connectHandler.BusIsConnected())
            {
                AZStd::this_thread::yield();
            }

            EXPECT_GE(AZStd::chrono::steady_clock::now(), endTime);
        };
        AZStd::thread connectThread([&connectHandler, &waitHandler]()
        {
            constexpr int waitMsMax = 100;
            auto startTime = AZStd::chrono::steady_clock::now();
            auto endTime = startTime + AZStd::chrono::milliseconds(waitMsMax);
            while (AZStd::chrono::steady_clock::now() < endTime && !waitHandler.m_didConnect)
            {
                AZStd::this_thread::yield();
            }
            connectHandler.BusConnect();
        }
        );
        waitHandler.BusConnect();
        connectThread.join();
        EXPECT_EQ(connectHandler.m_didConnect, true);
        EXPECT_EQ(waitHandler.m_didConnect, true);
        waitHandler.BusDisconnect();
        connectHandler.BusDisconnect();
    }

    TEST_F(EBus, ConnectionPolicy_WaitOnSecondHandlerWhileUnlocked_CanComplete)
    {
        constexpr int numTests = 20;
        for (int connectTest = 0; connectTest < numTests; ++connectTest)
        {
            DelayUnlockHandler waitHandler;
            DelayUnlockHandler connectHandler;
            waitHandler.m_connectMethod = [&connectHandler]()
            {
                // Check that a connection for the connectHandler has occured
                // within a 1 second, which should be more than enough
                // time for a connection to occur even when the system is under load
                constexpr int waitMsMax = 1000;
                auto startTime = AZStd::chrono::steady_clock::now();
                auto endTime = startTime + AZStd::chrono::milliseconds(waitMsMax);

                // The other bus should be able to connect
                while (AZStd::chrono::steady_clock::now() < endTime && !connectHandler.m_didConnect)
                {
                    AZStd::this_thread::yield();
                }
                EXPECT_EQ(connectHandler.BusIsConnected(), true);
                EXPECT_LE(AZStd::chrono::steady_clock::now(), endTime);
            };
            AZStd::thread connectThread([&connectHandler]()
            {
                connectHandler.BusConnect();
            });
            waitHandler.BusConnect();
            connectThread.join();
            EXPECT_EQ(connectHandler.m_didConnect, true);
            EXPECT_EQ(waitHandler.m_didConnect, true);
            waitHandler.BusDisconnect();
            connectHandler.BusDisconnect();
        }
    }

    class IdBusWithConnectionPolicy
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = int64_t;
        virtual ~IdBusWithConnectionPolicy() = default;

        virtual void MessageWhichOccursDuringConnect() = 0;
        virtual void MessageWhichOccursDuringDisconnect() = 0;

        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr&, typename Bus::Context&, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& , const typename Bus::BusIdType&)
            {
                handler->MessageWhichOccursDuringConnect();
            }
            static void Disconnect([[maybe_unused]] typename Bus::Context& context, typename Bus::HandlerNode& handler, [[maybe_unused]] typename Bus::BusPtr& ptr)
            {
                handler->MessageWhichOccursDuringDisconnect();
            }
        };
    };

    using IdBusWithConnectionPolicyBus = AZ::EBus<IdBusWithConnectionPolicy>;

    class MultiHandlerWhichDisconnectsDuringDelivery
        : public IdBusWithConnectionPolicyBus::MultiHandler
    {
        void MessageWhichOccursDuringConnect() override
        {
            auto busIdRef = IdBusWithConnectionPolicyBus::GetCurrentBusId();
            EXPECT_NE(nullptr, busIdRef);
            BusDisconnect(*busIdRef);
        }
        void MessageWhichOccursDuringDisconnect() override
        {
            auto busIdRef = IdBusWithConnectionPolicyBus::GetCurrentBusId();
            EXPECT_NE(nullptr, busIdRef);
        }
    };

    static constexpr int64_t multiHandlerTestBusId = 42;

    TEST_F(EBus, MultiHandlerConnectionPolicy_DisconnectDuringDelivery)
    {
        MultiHandlerWhichDisconnectsDuringDelivery multiHandlerTest;
        multiHandlerTest.BusConnect(multiHandlerTestBusId);
        EXPECT_EQ(0U, IdBusWithConnectionPolicyBus::GetTotalNumOfEventHandlers());
    }

    class BusWithConnectionAndDisconnectPolicy
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = int32_t;

        virtual ~BusWithConnectionAndDisconnectPolicy() = default;

        virtual void MessageWhichOccursDuringConnect(int32_t id) = 0;
        virtual void MessageWhichOccursDuringDisconnect(int32_t id) = 0;

        template<class Bus>
        struct ConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& ptr, [[maybe_unused]] typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard&, const typename Bus::BusIdType& id)
            {
                EXPECT_EQ(handler.GetBusId(), id);
                EXPECT_EQ(handler.m_holder, ptr);
                handler->MessageWhichOccursDuringConnect(handler.GetBusId());
            }
            static void Disconnect([[maybe_unused]] typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::BusPtr& ptr)
            {
                EXPECT_EQ(handler.m_holder, ptr);
                handler->MessageWhichOccursDuringDisconnect(handler.GetBusId());
            }
        };
    };

    using BusWithConnectionAndDisconnectPolicyBus = AZ::EBus<BusWithConnectionAndDisconnectPolicy>;

    struct HandlerTrackingConnectionDisconnectionIds
        : public BusWithConnectionAndDisconnectPolicyBus::Handler
    {
        void MessageWhichOccursDuringConnect(int32_t id) override
        {
            m_lastConnectId = id;
        }

        void MessageWhichOccursDuringDisconnect(int32_t id) override
        {
            m_lastDisconnectId = id;
        }

        int32_t m_lastConnectId = 0;
        int32_t m_lastDisconnectId = 0;
    };

    TEST_F(EBus, ConnectionPolicy_ConnectDisconnect_CorrectIds)
    {
        HandlerTrackingConnectionDisconnectionIds idsHandler;

        EXPECT_EQ(idsHandler.m_lastConnectId, 0);
        EXPECT_EQ(idsHandler.m_lastDisconnectId, 0);

        idsHandler.BusConnect(123);
        EXPECT_TRUE(idsHandler.BusIsConnectedId(123));
        EXPECT_EQ(idsHandler.m_lastConnectId, 123);

        idsHandler.BusDisconnect(123);
        EXPECT_FALSE(idsHandler.BusIsConnectedId(123));
        EXPECT_EQ(idsHandler.m_lastDisconnectId, 123);

        idsHandler.BusConnect(234);
        EXPECT_TRUE(idsHandler.BusIsConnectedId(234));
        EXPECT_EQ(idsHandler.m_lastConnectId, 234);

        idsHandler.BusDisconnect();
        EXPECT_FALSE(idsHandler.BusIsConnectedId(234));
        EXPECT_EQ(idsHandler.m_lastDisconnectId, 234);
    }

    struct LastHandlerDisconnectInterface
        : public AZ::EBusTraits
    {
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef size_t BusIdType;

        virtual void OnEvent() = 0;
    };

    using LastHandlerDisconnectBus = AZ::EBus<LastHandlerDisconnectInterface>;

    struct LastHandlerDisconnectHandler
        : public LastHandlerDisconnectBus::Handler
    {
        void OnEvent() override
        {
            ++m_numOnEvents;
            BusDisconnect();
        }

        unsigned int m_numOnEvents = 0;
    };

    TEST_F(EBus, LastHandlerDisconnectForward)
    {
        LastHandlerDisconnectHandler lastHandler;
        lastHandler.BusConnect(0);
        EBUS_EVENT_ID(0, LastHandlerDisconnectBus, OnEvent);
        EXPECT_FALSE(lastHandler.BusIsConnected());
        EXPECT_EQ(1, lastHandler.m_numOnEvents);
    }

    TEST_F(EBus, LastHandlerDisconnectReverse)
    {
        LastHandlerDisconnectHandler lastHandler;
        lastHandler.BusConnect(0);
        EBUS_EVENT_ID_REVERSE(0, LastHandlerDisconnectBus, OnEvent);
        EXPECT_FALSE(lastHandler.BusIsConnected());
        EXPECT_EQ(1, lastHandler.m_numOnEvents);
    }

    struct DisconnectAssertInterface
        : public AZ::EBusTraits
    {
        using MutexType = AZStd::recursive_mutex;

        virtual ~DisconnectAssertInterface() = default;
        virtual void OnEvent() {};
    };

    using DisconnectAssertBus = AZ::EBus<DisconnectAssertInterface>;

    struct DisconnectAssertHandler
        : public DisconnectAssertBus::Handler
    {

    };

    TEST_F(EBus, HandlerDestroyedWithoutDisconnect_Asserts)
    {
        // EBus handlers with a non-NullMutex assert on disconnect if they have not been explicitly disconnected before the internal EBus::Handler destructor is invoked.
        // The reason for the assert is because the BusDisconnect call will lock the EBus context mutex to safely disconnect the handler, but if the handler is still
        // connected to the EBus, another thread could access it after the vtable for the derived class has been reset.

        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            DisconnectAssertHandler handler;
            handler.BusConnect();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(EBus, HandlerDestroyedAfterDisconnect_DoesNotAssert)
    {
        {
            DisconnectAssertHandler handler;
            handler.BusConnect();
            handler.BusDisconnect();
        }
    }

    struct SingleHandlerPerIdTestRequests
        : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = int32_t;
    };

    using SingleHandlerPerIdTestRequestBus = AZ::EBus<SingleHandlerPerIdTestRequests>;

    struct SingleHandlerPerIdTestImpl : public SingleHandlerPerIdTestRequestBus::Handler
    {
        void Connect(SingleHandlerPerIdTestRequestBus::BusIdType busId)
        {
            SingleHandlerPerIdTestRequestBus::Handler::BusConnect(busId);
        }

        void Disconnect()
        {
            SingleHandlerPerIdTestRequestBus::Handler::BusDisconnect();
        }
    };

    struct MultipleHandlersPerIdTestRequests
        : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = int32_t;
    };

    using MultipleHandlersPerIdTestRequestBus = AZ::EBus<MultipleHandlersPerIdTestRequests>;

    struct MultipleHandlersPerIdTestImpl
        : public MultipleHandlersPerIdTestRequestBus::MultiHandler
    {
        void Connect(MultipleHandlersPerIdTestRequestBus::BusIdType busId)
        {
            MultipleHandlersPerIdTestRequestBus::MultiHandler::BusConnect(busId);
        }

        void Disconnect()
        {
            MultipleHandlersPerIdTestRequestBus::MultiHandler::BusDisconnect();
        }
    };

    TEST_F(EBus, HasHandlersAddressId)
    {
        SingleHandlerPerIdTestImpl idTestRequest;
        idTestRequest.Connect(4);

        // note: arbitrary numbers selected
        EXPECT_TRUE(SingleHandlerPerIdTestRequestBus::HasHandlers(4));
        EXPECT_FALSE(SingleHandlerPerIdTestRequestBus::HasHandlers(10));

        idTestRequest.Disconnect();
    }

    TEST_F(EBus, HasHandlersWithSingleHandlerListeningToMultipleIds)
    {
        MultipleHandlersPerIdTestImpl idTestRequest;
        idTestRequest.Connect(4);
        idTestRequest.Connect(7);
        idTestRequest.Connect(10);

        // note: arbitrary numbers selected
        EXPECT_TRUE(MultipleHandlersPerIdTestRequestBus::HasHandlers(4));
        EXPECT_TRUE(MultipleHandlersPerIdTestRequestBus::HasHandlers(7));
        EXPECT_TRUE(MultipleHandlersPerIdTestRequestBus::HasHandlers(10));
        EXPECT_FALSE(MultipleHandlersPerIdTestRequestBus::HasHandlers(15));

        idTestRequest.Disconnect();
    }

    TEST_F(EBus, HasHandlersWithMultipleHandlersOnSameId)
    {
        MultipleHandlersPerIdTestImpl id1, id2;
        id1.Connect(4);
        id2.Connect(4);

        EXPECT_TRUE(MultipleHandlersPerIdTestRequestBus::HasHandlers(4));
        EXPECT_FALSE(MultipleHandlersPerIdTestRequestBus::HasHandlers(7));
    }

    TEST_F(EBus, HasHandlersWithTwoSingleHandlersOnDifferentIds)
    {
        SingleHandlerPerIdTestImpl id1, id2;
        id1.Connect(4);
        id2.Connect(5);

        EXPECT_TRUE(SingleHandlerPerIdTestRequestBus::HasHandlers(4));
        EXPECT_TRUE(SingleHandlerPerIdTestRequestBus::HasHandlers(5));
        EXPECT_FALSE(SingleHandlerPerIdTestRequestBus::HasHandlers(7));
    }

    TEST_F(EBus, HasHandlersAddressPtr)
    {
        // note: arbitrary numbers selected
        constexpr SingleHandlerPerIdTestRequestBus::BusIdType validBusId = 4;
        constexpr SingleHandlerPerIdTestRequestBus::BusIdType invalidBusId = 10;

        SingleHandlerPerIdTestImpl idTestRequest;
        idTestRequest.Connect(validBusId);

        SingleHandlerPerIdTestRequestBus::BusPtr validBusIdPtr;
        SingleHandlerPerIdTestRequestBus::Bind(validBusIdPtr, validBusId);
        SingleHandlerPerIdTestRequestBus::BusPtr invalidBusIdPtr;
        SingleHandlerPerIdTestRequestBus::Bind(invalidBusIdPtr, invalidBusId);

        EXPECT_TRUE(SingleHandlerPerIdTestRequestBus::HasHandlers(validBusIdPtr));
        EXPECT_FALSE(SingleHandlerPerIdTestRequestBus::HasHandlers(invalidBusIdPtr));

        idTestRequest.Disconnect();
    }

    // IsInDispatchThisThread
    struct IsInThreadDispatchRequests
        : AZ::EBusTraits
    {
        using MutexType = AZStd::recursive_mutex;
    };

    using IsInThreadDispatchBus = AZ::EBus<IsInThreadDispatchRequests>;

    class IsInThreadDispatchHandler
        : public IsInThreadDispatchBus::Handler
    {};

    TEST_F(EBus, InvokingIsInThisThread_ReturnsSuccess_OnlyIfThreadIsInDispatch)
    {
        IsInThreadDispatchHandler handler;
        handler.BusConnect();

        auto ThreadDispatcher = [](IsInThreadDispatchRequests*)
        {
            EXPECT_TRUE(IsInThreadDispatchBus::IsInDispatchThisThread());
            auto PerThreadBusDispatch = []()
            {
                EXPECT_FALSE(IsInThreadDispatchBus::IsInDispatchThisThread());
            };
            AZStd::array threads{ AZStd::thread(PerThreadBusDispatch), AZStd::thread(PerThreadBusDispatch) };
            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }
        };

        static constexpr size_t ThreadDispatcherIterations = 4;
        for (size_t iteration = 0; iteration < ThreadDispatcherIterations; ++iteration)
        {
            EXPECT_FALSE(IsInThreadDispatchBus::IsInDispatchThisThread());
            IsInThreadDispatchBus::Broadcast(ThreadDispatcher);
            EXPECT_FALSE(IsInThreadDispatchBus::IsInDispatchThisThread());
        }
    }

    // Thread Dispatch Policy
    struct ThreadDispatchTestBusTraits
        : AZ::EBusTraits
    {
        using MutexType = AZStd::recursive_mutex;

        struct PostThreadDispatchTestInvoker
        {
            ~PostThreadDispatchTestInvoker();
        };

        template <typename DispatchMutex>
        struct ThreadDispatchTestLockGuard
        {
            ThreadDispatchTestLockGuard(DispatchMutex& contextMutex)
                : m_lock{ contextMutex }
            {}
            ThreadDispatchTestLockGuard(DispatchMutex& contextMutex, AZStd::adopt_lock_t adopt_lock)
                : m_lock{ contextMutex, adopt_lock }
            {}
            ThreadDispatchTestLockGuard(const ThreadDispatchTestLockGuard&) = delete;
            ThreadDispatchTestLockGuard& operator=(const ThreadDispatchTestLockGuard&) = delete;
        private:
            PostThreadDispatchTestInvoker m_threadPolicyInvoker;
            using LockType = AZStd::conditional_t<LocklessDispatch, AZ::Internal::NullLockGuard<DispatchMutex>, AZStd::scoped_lock<DispatchMutex>>;
            LockType m_lock;
        };

        template <typename DispatchMutex, bool IsLocklessDispatch>
        using DispatchLockGuard = ThreadDispatchTestLockGuard<DispatchMutex>;

        static inline AZStd::atomic<int32_t> s_threadPostDispatchCalls;
    };

    class ThreadDispatchTestRequests
    {
    public:
        virtual void FirstCall() = 0;
        virtual void SecondCall() = 0;
        virtual void ThirdCall() = 0;
    };

    using ThreadDispatchTestBus = AZ::EBus<ThreadDispatchTestRequests, ThreadDispatchTestBusTraits>;

    ThreadDispatchTestBusTraits::PostThreadDispatchTestInvoker::~PostThreadDispatchTestInvoker()
    {
        if (!ThreadDispatchTestBus::IsInDispatchThisThread())
        {
            ++s_threadPostDispatchCalls;
        }
    }

    class ThreadDispatchTestHandler
        : public ThreadDispatchTestBus::Handler
    {
    public:
        void Connect()
        {
            ThreadDispatchTestBus::Handler::BusConnect();
        }
        void Disconnect()
        {
            ThreadDispatchTestBus::Handler::BusDisconnect();
        }

        void FirstCall() override
        {
            ThreadDispatchTestBus::Broadcast(&ThreadDispatchTestBus::Events::SecondCall);
        }
        void SecondCall() override
        {
            ThreadDispatchTestBus::Broadcast(&ThreadDispatchTestBus::Events::ThirdCall);
        }
        void ThirdCall() override
        {
        }
    };

    template <typename ParamType>
    class EBusParamFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<ParamType>
    {};

    struct ThreadDispatchParams
    {
        size_t m_threadCount{};
        size_t m_handlerCount{};
    };

    using ThreadDispatchParamFixture = EBusParamFixture<ThreadDispatchParams>;

    INSTANTIATE_TEST_CASE_P(
        ThreadDispatch,
        ThreadDispatchParamFixture,
        ::testing::Values(
            ThreadDispatchParams{ 1, 1 },
            ThreadDispatchParams{ 2, 1 },
            ThreadDispatchParams{ 1, 2 },
            ThreadDispatchParams{ 2, 2 },
            ThreadDispatchParams{ 16, 8 }
            )
    );

    TEST_P(ThreadDispatchParamFixture, CustomDispatchLockGuard_InvokesPostDispatchFunction_AfterThreadHasFinishedDispatch)
    {
        ThreadDispatchTestBusTraits::s_threadPostDispatchCalls = 0;
        ThreadDispatchParams threadDispatchParams = GetParam();
        AZStd::vector<AZStd::thread> testThreads;
        AZStd::vector<ThreadDispatchTestHandler> testHandlers(threadDispatchParams.m_handlerCount);
        for (ThreadDispatchTestHandler& testHandler : testHandlers)
        {
            testHandler.Connect();
        }

        static constexpr size_t DispatchThreadCalls = 3;
        const size_t totalThreadDispatchCalls = threadDispatchParams.m_threadCount * DispatchThreadCalls;

        auto DispatchThreadWorker = []()
        {
            ThreadDispatchTestBus::Broadcast(&ThreadDispatchTestBus::Events::FirstCall);
            ThreadDispatchTestBus::Broadcast(&ThreadDispatchTestBus::Events::SecondCall);
            ThreadDispatchTestBus::Broadcast(&ThreadDispatchTestBus::Events::ThirdCall);
        };

        for (size_t threadIndex = 0; threadIndex < threadDispatchParams.m_threadCount; ++threadIndex)
        {
            testThreads.emplace_back(DispatchThreadWorker);
        }

        for (AZStd::thread& thread : testThreads)
        {
            thread.join();
        }

        for (ThreadDispatchTestHandler& testHandler : testHandlers)
        {
            testHandler.Disconnect();
        }

        EXPECT_EQ(totalThreadDispatchCalls, ThreadDispatchTestBusTraits::s_threadPostDispatchCalls);
        ThreadDispatchTestBusTraits::s_threadPostDispatchCalls = 0;
    }


    struct ReentrantEBusUseTestRequests : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = int32_t;

        // This event will directly call itself recursively via EBus.
        virtual void EventDirectlyCallsItself(bool expectedReentrantResult) = 0;

        // The first event will call the second event via EBus.
        virtual void EventCallsOtherEventOnSameEBus() = 0;
        virtual bool EventCalledByOtherEventOnSameEBus() = 0;

        // The first event will call the second event via EBus, but on different bus IDs.
        virtual void EventCallsOtherEventOnDifferentEBusId(BusIdType busId) = 0;
        virtual bool EventCalledByOtherEventOnDifferentEBusId() = 0;
    };

    using ReentrantEBusUseTestRequestBus = AZ::EBus<ReentrantEBusUseTestRequests>;

    struct ReentrantEBusUseTestImpl : public ReentrantEBusUseTestRequestBus::Handler
    {
        ReentrantEBusUseTestImpl(ReentrantEBusUseTestRequestBus::BusIdType busId)
        {
            m_busId = busId;
            ReentrantEBusUseTestRequestBus::Handler::BusConnect(busId);
        }

        ~ReentrantEBusUseTestImpl()
        {
            ReentrantEBusUseTestRequestBus::Handler::BusDisconnect();
        }

        void EventDirectlyCallsItself(bool expectedReentrantResult) override
        {
            // Verify that we get the expected API result. (We use ASSERT_EQ because a test failure here might cause infinite recursion)
            ASSERT_EQ(ReentrantEBusUseTestRequestBus::HasReentrantEBusUseThisThread(), expectedReentrantResult);

            // Avoid infinite recursion. :)
            if (ReentrantEBusUseTestRequestBus::HasReentrantEBusUseThisThread())
            {
                return;
            }

            // This event calls itself via a nested EBus call. We expect the nested call to detect the reentrancy.
            ReentrantEBusUseTestRequestBus::Event(m_busId, &ReentrantEBusUseTestRequestBus::Events::EventDirectlyCallsItself, true);
        }

        void EventCallsOtherEventOnSameEBus() override
        {
            // Call a second event on the same EBus and verify that it was called.
            bool otherEventCalled = false;
            ReentrantEBusUseTestRequestBus::EventResult(otherEventCalled, m_busId, 
                &ReentrantEBusUseTestRequestBus::Events::EventCalledByOtherEventOnSameEBus);
            EXPECT_TRUE(otherEventCalled);
        }

        bool EventCalledByOtherEventOnSameEBus() override
        {
            // Verify that even though two different events have been called on the same EBus,
            // it is still considered reentrant use of the ebus itself.
            EXPECT_TRUE(ReentrantEBusUseTestRequestBus::HasReentrantEBusUseThisThread());
            return true;
        }

        void EventCallsOtherEventOnDifferentEBusId(BusIdType busId) override
        {
            // Call a second event on a different EBus and verify that it was called.
            bool otherEventCalled = false;
            ReentrantEBusUseTestRequestBus::EventResult(otherEventCalled, busId,
                &ReentrantEBusUseTestRequestBus::Events::EventCalledByOtherEventOnDifferentEBusId);
            EXPECT_TRUE(otherEventCalled);
        }

        bool EventCalledByOtherEventOnDifferentEBusId() override
        {
            // Verify that two different nested events on the same EBus but with different EBus IDs will not be detected as reentrant.
            EXPECT_FALSE(ReentrantEBusUseTestRequestBus::HasReentrantEBusUseThisThread());
            return true;
        }


    protected:
        ReentrantEBusUseTestRequestBus::BusIdType m_busId;
    };

    TEST_F(EBus, ReentrantEBusUsageDetectedFromNestedDirectCalls)
    {
        constexpr int32_t busId = 4;
        ReentrantEBusUseTestImpl reentrantEBusUseTestRequest(busId);

        constexpr bool expectedReentrantResult = false;
        ReentrantEBusUseTestRequestBus::Event(
            busId, &ReentrantEBusUseTestRequestBus::Events::EventDirectlyCallsItself, expectedReentrantResult);
    }

    TEST_F(EBus, ReentrantEBusUsageDetectedFromTwoSeparateCallsOnSameBus)
    {
        constexpr int32_t busId = 4;
        ReentrantEBusUseTestImpl reentrantEBusUseTestRequest(busId);

        ReentrantEBusUseTestRequestBus::Event(busId, &ReentrantEBusUseTestRequestBus::Events::EventCallsOtherEventOnSameEBus);
    }

    TEST_F(EBus, ReentrantEBusUsageNotDetectedFromTwoSeparateCallsOnSameBusWithDifferentIds)
    {
        constexpr int32_t busId = 4;
        ReentrantEBusUseTestImpl reentrantEBusUseTestRequest(busId);

        constexpr int32_t secondBusId = 8;
        ReentrantEBusUseTestImpl secondReentrantEBusUseTestRequest(secondBusId);

        ReentrantEBusUseTestRequestBus::Event(busId,
            &ReentrantEBusUseTestRequestBus::Events::EventCallsOtherEventOnDifferentEBusId, secondBusId);
    }

} // namespace UnitTest

#if defined(HAVE_BENCHMARK)
//-------------------------------------------------------------------------
// PERF TESTS
//-------------------------------------------------------------------------
namespace Benchmark
{
    namespace BenchmarkSettings
    {
        namespace
        {
            // How many addresses/handlers count as "many"
            static const int Many = 1000;
        }

        void Common(::benchmark::internal::Benchmark* benchmark)
        {
            benchmark
                ->Unit(::benchmark::kNanosecond)
                ;
        }

        void OneToOne(::benchmark::internal::Benchmark* benchmark)
        {
            Common(benchmark);
            benchmark
                ->ArgNames({ { "Addresses" },{ "Handlers" } })
                ->Args({ 0, 0 })
                ->Args({ 1, 1 })
                ;
        }

        void OneToMany(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({ 1, Many })
                ;
        }

        void ManyToOne(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({ Many, 1 })
                ;
        }

        void ManyToMany(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({    1, Many })
                ->Args({ Many, 1 })
                ->Args({ Many, Many })
                ;
        }

        // Expected that this will be called after one of the above, so Common not called
        void Multithreaded(::benchmark::internal::Benchmark* benchmark)
        {
            benchmark
                ->ThreadRange(1, 8)
                ->ThreadPerCpu();
                ;
        }
    }

    // AZ Benchmark environment used to initialize all EBus Handlers and then shared them with each benchmark test
    template<typename Bus>
    class BM_EBusEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
    {
    public:
        using BusType = Bus;
        using HandlerT = Handler<Bus>;

        BM_EBusEnvironment()
        {
        }

        void SetUpBenchmark() override
        {
            // Created the container for the EBusHandlers
            m_handlers = std::make_unique<std::vector<HandlerT>>();

            // Connect handlers
            constexpr bool multiAddress = Bus::Traits::AddressPolicy != AZ::EBusAddressPolicy::Single;
            constexpr bool multiHandler = Bus::Traits::HandlerPolicy != AZ::EBusHandlerPolicy::Single;
            constexpr int64_t numAddresses{ multiAddress ? BenchmarkSettings::Many : 1 };
            constexpr int64_t numHandlers{ multiHandler ? BenchmarkSettings::Many : 1 };
            constexpr bool connectOnConstruct{ false };

            AZ::BetterPseudoRandom random;

            m_handlers->reserve(numAddresses * numHandlers);
            for (int64_t address = 0; address < numAddresses; ++address)
            {
                for (int64_t handler = 0; handler < numHandlers; ++handler)
                {
                    int handlerOrder{};
                    random.GetRandom(handlerOrder);
                    m_handlers->emplace_back(HandlerT(static_cast<int>(address), handlerOrder, connectOnConstruct));
                }
            }
        }

        void TearDownBenchmark() override
        {
            // Deallocate the memory associated with the EBusHandlers
            m_handlers.reset();
        }

        void Connect(::benchmark::State& state)
        {
            int64_t numAddresses = state.range(0);
            int64_t numHandlers = state.range(1);
            const size_t totalHandlers = static_cast<size_t>(numAddresses * numHandlers);

            // Connect handlers
            for (size_t handlerIndex = 0; handlerIndex < totalHandlers; ++handlerIndex)
            {
                if(handlerIndex < m_handlers->size())
                {
                    (*m_handlers)[handlerIndex].Connect();
                }
            }
        }

        void Disconnect(::benchmark::State& state)
        {
            int64_t numAddresses = state.range(0);
            int64_t numHandlers = state.range(1);
            const size_t totalHandlers = static_cast<size_t>(numAddresses * numHandlers);

            // Disconnect handlers
            for (size_t handlerIndex = 0; handlerIndex < totalHandlers; ++handlerIndex)
            {
                if (handlerIndex < m_handlers->size())
                {
                    (*m_handlers)[handlerIndex].Disconnect();
                }
            }
        }

    protected:
        std::unique_ptr<std::vector<HandlerT>> m_handlers;
    };

    // Using a variable template to initialize the benchmark EBus_Environment on template instantiation
    template<typename Bus>
    static BM_EBusEnvironment<Bus>& s_benchmarkEBusEnv = AZ::Test::RegisterBenchmarkEnvironment<BM_EBusEnvironment<Bus>>();

// Internal macro callback for listing all buses requiring ids
#define BUS_BENCHMARK_PRIVATE_LIST_ID(cb, fn)    \
    cb(fn, ManyToOne, ManyToOne)                 \
    cb(fn, ManyToMany, ManyToMany)               \
    cb(fn, ManyToManyOrdered, ManyToMany)        \
    cb(fn, ManyOrderedToOne, ManyToOne)          \
    cb(fn, ManyOrderedToMany, ManyToMany)        \
    cb(fn, ManyOrderedToManyOrdered, ManyToMany)

// Internal macro callback for listing all buses
#define BUS_BENCHMARK_PRIVATE_LIST_ALL(cb, fn)  \
    cb(fn, OneToOne, OneToOne)                  \
    cb(fn, OneToMany, OneToMany)                \
    cb(fn, OneToManyOrdered, OneToMany)         \
    BUS_BENCHMARK_PRIVATE_LIST_ID(cb, fn)

// Internal macro callback for registering a benchmark
#define BUS_BENCHMARK_PRIVATE_REGISTER(fn, BusDef, SettingsFn) BENCHMARK_TEMPLATE(fn, BusDef)->Apply(&BenchmarkSettings::SettingsFn);

// Register a benchmark for all bus permutations requiring ids
#define BUS_BENCHMARK_REGISTER_ID(fn) BUS_BENCHMARK_PRIVATE_LIST_ID(BUS_BENCHMARK_PRIVATE_REGISTER, fn)

// Register a benchmark for all bus permutations
#define BUS_BENCHMARK_REGISTER_ALL(fn) BUS_BENCHMARK_PRIVATE_LIST_ALL(BUS_BENCHMARK_PRIVATE_REGISTER, fn)

    //////////////////////////////////////////////////////////////////////////
    // Single Threaded Events/Broadcasts
    //////////////////////////////////////////////////////////////////////////

    // Baseline benchmark for raw vtable call
    static void BM_EBus_RawCall(::benchmark::State& state)
    {
        constexpr bool connectOnConstruct{ true };
        AZStd::unique_ptr<Handler<OneToOne>> handler = AZStd::make_unique<Handler<OneToOne>>(0, connectOnConstruct);

        while (state.KeepRunning())
        {
            handler->OnEvent();
        }
    }
    BENCHMARK(BM_EBus_RawCall)->Apply(&BenchmarkSettings::Common);

#define BUS_BENCHMARK_PRIVATE_REGISTER_CONNECTION(fn, BusDef, _) BENCHMARK_TEMPLATE(fn, BusDef)->Apply(&BenchmarkSettings::Common);

    template <typename Bus>
    static void BM_EBus_BusConnect(::benchmark::State& state)
    {
        constexpr bool connectOnConstruct{ false };
        Handler<Bus> handler{ 0, connectOnConstruct };

        while (state.KeepRunning())
        {
            handler.Connect();

            // Pause timing, and disconnect
            state.PauseTiming();
            handler.BusDisconnect();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_PRIVATE_LIST_ALL(BUS_BENCHMARK_PRIVATE_REGISTER_CONNECTION, BM_EBus_BusConnect);

    template <typename Bus>
    static void BM_EBus_BusDisconnect(::benchmark::State& state)
    {
        constexpr bool connectOnConstruct{ true };
        Handler<Bus> handler{ 0, connectOnConstruct };

        while (state.KeepRunning())
        {
            handler.BusDisconnect();

            // Pause timing, and reconnect
            state.PauseTiming();
            handler.Connect();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_PRIVATE_LIST_ALL(BUS_BENCHMARK_PRIVATE_REGISTER_CONNECTION, BM_EBus_BusDisconnect);

#undef BUS_BENCHMARK_PRIVATE_REGISTER_CONNECTION

    template <typename Bus>
    static void BM_EBus_EnumerateHandlers(::benchmark::State& state)
    {
        auto OnEventVisitor = [](typename Bus::InterfaceType* interfaceInst) -> bool
        {
            interfaceInst->OnEvent();
            return true;
        };

        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            Bus::EnumerateHandlers(OnEventVisitor);
        }

        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_EnumerateHandlers);

    template <typename Bus>
    static void BM_EBus_Broadcast(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnEvent);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_Broadcast);

    template <typename Bus>
    static void BM_EBus_BroadcastResult(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            int result = 0;
            Bus::BroadcastResult(result, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_BroadcastResult);

    template <typename Bus>
    static void BM_EBus_Event(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            Bus::Event(1, &Bus::Events::OnEvent);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_Event);

    template <typename Bus>
    static void BM_EBus_EventResult(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            int result = 0;
            Bus::EventResult(result, 1, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventResult);

    template <typename Bus>
    static void BM_EBus_EventCached(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        typename Bus::BusPtr cachedPtr;
        constexpr typename Bus::BusIdType firstConnectedAddressId{ 0 };
        Bus::Bind(cachedPtr, firstConnectedAddressId);

        while (state.KeepRunning())
        {
            Bus::Event(cachedPtr, &Bus::Events::OnEvent);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventCached);

    template <typename Bus>
    static void BM_EBus_EventCachedResult(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        typename Bus::BusPtr cachedPtr;
        constexpr typename Bus::BusIdType firstConnectedAddressId{ 0 };
        Bus::Bind(cachedPtr, firstConnectedAddressId);

        while (state.KeepRunning())
        {
            int result = 0;
            Bus::EventResult(result, cachedPtr, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventCachedResult);

    //////////////////////////////////////////////////////////////////////////
    // Broadcast/Event Queuing
    //////////////////////////////////////////////////////////////////////////

    // Broadcast
    template <typename Bus>
    static void BM_EBus_QueueBroadcast(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            Bus::QueueBroadcast(&Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_QueueBroadcast);

    template <typename Bus>
    static void BM_EBus_ExecuteBroadcast(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueBroadcast(&Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);

    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_ExecuteBroadcast);

    // Event
    template <typename Bus>
    static void BM_EBus_QueueEvent(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            Bus::QueueEvent(1, &Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_QueueEvent);

    template <typename Bus>
    static void BM_EBus_ExecuteEvent(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueEvent(1, &Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_ExecuteEvent);

    // Event Cached
    template <typename Bus>
    static void BM_EBus_QueueEventCached(::benchmark::State& state)
    {
        typename Bus::BusPtr cachedPtr;
        constexpr typename Bus::BusIdType firstConnectedAddressId{ 0 };
        Bus::Bind(cachedPtr, firstConnectedAddressId);

        while (state.KeepRunning())
        {
            Bus::QueueEvent(cachedPtr, &Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_QueueEventCached);

    template <typename Bus>
    static void BM_EBus_ExecuteQueueCached(::benchmark::State& state)
    {
        s_benchmarkEBusEnv<Bus>.Connect(state);
        typename Bus::BusPtr cachedPtr;
        constexpr typename Bus::BusIdType firstConnectedAddressId{ 0 };
        Bus::Bind(cachedPtr, firstConnectedAddressId);

        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueEvent(cachedPtr, &Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
        s_benchmarkEBusEnv<Bus>.Disconnect(state);
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_ExecuteQueueCached);

    //////////////////////////////////////////////////////////////////////////
    // Multithreaded Broadcasts
    //////////////////////////////////////////////////////////////////////////

    static void BM_EBus_Multithreaded_Locks(::benchmark::State& state)
    {
        using Bus = TestBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Multiple, false>;

        AZStd::unique_ptr<BM_EBusEnvironment<Bus>> ebusBenchmarkEnv;
        if (state.thread_index() == 0)
        {
            ebusBenchmarkEnv = AZStd::make_unique<BM_EBusEnvironment<Bus>>();
            ebusBenchmarkEnv->SetUpBenchmark();
            ebusBenchmarkEnv->Connect(state);
        }

        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnWait);
        };

        if (state.thread_index() == 0)
        {
            ebusBenchmarkEnv->Disconnect(state);
            ebusBenchmarkEnv->TearDownBenchmark();
        }
    }
    BENCHMARK(BM_EBus_Multithreaded_Locks)->Apply(&BenchmarkSettings::OneToMany)->Apply(&BenchmarkSettings::Multithreaded);

    static void BM_EBus_Multithreaded_Lockless(::benchmark::State& state)
    {
        using Bus = TestBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Multiple, true>;

        AZStd::unique_ptr<BM_EBusEnvironment<Bus>> ebusBenchmarkEnv;
        if (state.thread_index() == 0)
        {
            ebusBenchmarkEnv = AZStd::make_unique<BM_EBusEnvironment<Bus>>();
            ebusBenchmarkEnv->SetUpBenchmark();
            ebusBenchmarkEnv->Connect(state);
        }

        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnWait);
        };

        if (state.thread_index() == 0)
        {
            ebusBenchmarkEnv->Disconnect(state);
            ebusBenchmarkEnv->TearDownBenchmark();
        }
    }
    BENCHMARK(BM_EBus_Multithreaded_Lockless)->Apply(&BenchmarkSettings::OneToMany)->Apply(&BenchmarkSettings::Multithreaded);
}

#endif // HAVE_BENCHMARK

