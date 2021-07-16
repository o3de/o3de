/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class ITestSystem
    {
    public:
        AZ_RTTI(ITestSystem, "{E13935AC-5A72-432E-9E5D-72975CCC5CF9}");

        ITestSystem() = default;
        ITestSystem(ITestSystem&&) = delete;

        ITestSystem& operator=(ITestSystem&&) = delete;

        virtual ~ITestSystem() = default;

        virtual void SetNumber(uint32_t) = 0;

        virtual uint32_t GetNumber() const = 0;
    };

    class TestSystem
        : public ITestSystem
    {
    public:
        void Activate()
        {
            AZ::Interface<ITestSystem>::Register(this);
        }

        void Deactivate()
        {
            AZ::Interface<ITestSystem>::Unregister(this);
        }

        void SetNumber(uint32_t number) override
        {
            m_number = number;
        }

        uint32_t GetNumber() const override
        {
            return m_number;
        }

    private:
        uint32_t m_number = 0;
    };

    class TestSystemWithRegistrar
        : public AZ::Interface<ITestSystem>::Registrar
    {
    public:

        void SetNumber(uint32_t number) override
        {
            m_number = number;
        }

        uint32_t GetNumber() const override
        {
            return m_number;
        }

    private:
        uint32_t m_number = 0;
    };

    class InterfaceTest
        : public AllocatorsTestFixture
    {
    };

    TEST_F(InterfaceTest, EmptyInterfaceTest)
    {
        ITestSystem* testSystemInterface = AZ::Interface<ITestSystem>::Get();
        EXPECT_EQ(testSystemInterface, nullptr);
    }

    TEST_F(InterfaceTest, EmptyAfterDisconnectTest)
    {
        TestSystem system;
        system.Activate();

        ITestSystem* valid = AZ::Interface<ITestSystem>::Get();
        EXPECT_NE(valid, nullptr);

        system.Deactivate();

        ITestSystem* invalid = AZ::Interface<ITestSystem>::Get();
        EXPECT_EQ(invalid, nullptr);
    }

    TEST_F(InterfaceTest, ValidInterfaceTest)
    {
        TestSystem system;
        system.Activate();

        {
            ITestSystem* testSystemInterface = AZ::Interface<ITestSystem>::Get();
            EXPECT_NE(testSystemInterface, nullptr);

            testSystemInterface->SetNumber(1);
            EXPECT_EQ(testSystemInterface->GetNumber(), 1);
        }

        system.Deactivate();
    }

    TEST_F(InterfaceTest, RegistrarTest)
    {
        ITestSystem* testSystemInterface = AZ::Interface<ITestSystem>::Get();
        EXPECT_EQ(testSystemInterface, nullptr);

        {
            TestSystemWithRegistrar system;

            testSystemInterface = AZ::Interface<ITestSystem>::Get();
            EXPECT_NE(testSystemInterface, nullptr);
        }

        testSystemInterface = AZ::Interface<ITestSystem>::Get();
        EXPECT_EQ(testSystemInterface, nullptr);
    }

    TEST_F(InterfaceTest, RegisterTwiceAssertTest)
    {
        TestSystem testSystem1;
        TestSystem testSystem2;

        testSystem1.Activate();

        AZ_TEST_START_TRACE_SUPPRESSION;
        // This should fail.
        testSystem2.Activate();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        AZ_TEST_START_TRACE_SUPPRESSION;
        // This should also fail.
        testSystem2.Deactivate();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        testSystem1.Deactivate();
    }

    TEST_F(InterfaceTest, RegisterMismatchTest)
    {
        TestSystem testSystem1;
        TestSystem testSystem2;

        testSystem1.Activate();

        AZ_TEST_START_TRACE_SUPPRESSION;
        // This should fail.
        testSystem2.Deactivate();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        testSystem1.Deactivate();
    }
}
