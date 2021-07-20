/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/RPITestFixture.h>
#include <Common/TestFeatureProcessors.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::RPI;

    class FeatureProcessorFactoryTests
        : public RPITestFixture
    {
    protected:

        void SetUp() override
        {
            RPITestFixture::SetUp();
            TestFeatureProcessor1::Reflect(GetSerializeContext());
            TestFeatureProcessor2::Reflect(GetSerializeContext());
            TestFeatureProcessorImplementation::Reflect(GetSerializeContext());
            TestFeatureProcessorImplementation2::Reflect(GetSerializeContext());

            FeatureProcessorFactory::Get()->RegisterFeatureProcessor<TestFeatureProcessor1>();
            FeatureProcessorFactory::Get()->RegisterFeatureProcessor<TestFeatureProcessor2>();
            FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TestFeatureProcessorImplementation, TestFeatureProcessorInterface>();
            FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<TestFeatureProcessorImplementation2, TestFeatureProcessorInterface>();
        }

        void TearDown() override
        {
            FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessor1>();
            FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessor2>();
            FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation>();
            FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation2>();
            RPITestFixture::TearDown();
        }
    };

    TEST_F(FeatureProcessorFactoryTests, GetFeatureProcessorTypeId_MultipleFeatureProcessorsRegistered_ReturnsCorrectTypeId)
    {
        AZ::TypeId testFeatureProcessor1Id = FeatureProcessorFactory::Get()->GetFeatureProcessorTypeId(FeatureProcessorId{ "TestFeatureProcessor1" });
        EXPECT_EQ(testFeatureProcessor1Id, TestFeatureProcessor1::RTTI_Type());


        AZ::TypeId testFeatureProcessor2Id = FeatureProcessorFactory::Get()->GetFeatureProcessorTypeId(FeatureProcessorId{ "TestFeatureProcessor2" });
        EXPECT_EQ(testFeatureProcessor2Id, TestFeatureProcessor2::RTTI_Type());

        AZ::TypeId testFeatureProcessorImplementationId = FeatureProcessorFactory::Get()->GetFeatureProcessorTypeId(FeatureProcessorId{ "TestFeatureProcessorImplementation" });
        EXPECT_EQ(testFeatureProcessorImplementationId, TestFeatureProcessorImplementation::RTTI_Type());


        AZ::TypeId testFeatureProcessorImplementation2Id = FeatureProcessorFactory::Get()->GetFeatureProcessorTypeId(FeatureProcessorId{ "TestFeatureProcessorImplementation2" });
        EXPECT_EQ(testFeatureProcessorImplementation2Id, TestFeatureProcessorImplementation2::RTTI_Type());
    }


    TEST_F(FeatureProcessorFactoryTests, GetFeatureProcessorInterfaceTypeId_FeatureProcessorHasInterface_ReturnsCorrectTypeId)
    {
        AZ::TypeId testFeatureProcessorInterfaceId = FeatureProcessorFactory::Get()->GetFeatureProcessorInterfaceTypeId(FeatureProcessorId{ "TestFeatureProcessorImplementation" });
        EXPECT_EQ(testFeatureProcessorInterfaceId, TestFeatureProcessorInterface::RTTI_Type());


        AZ::TypeId testFeatureProcessorInterfaceId2 = FeatureProcessorFactory::Get()->GetFeatureProcessorInterfaceTypeId(FeatureProcessorId{ "TestFeatureProcessorImplementation2" });
        EXPECT_EQ(testFeatureProcessorInterfaceId2, TestFeatureProcessorInterface::RTTI_Type());

        EXPECT_EQ(testFeatureProcessorInterfaceId, testFeatureProcessorInterfaceId2);
    }

    TEST_F(FeatureProcessorFactoryTests, GetFeatureProcessorInterfaceTypeId_FeatureProcessorDoesNotHaveInterface_ReturnsNullTypeId)
    {
        AZ::TypeId testFeatureProcessor1InterfaceId = FeatureProcessorFactory::Get()->GetFeatureProcessorInterfaceTypeId(FeatureProcessorId{ "TestFeatureProcessor1" });
        EXPECT_EQ(testFeatureProcessor1InterfaceId, TypeId::CreateNull());

        AZ::TypeId testFeatureProcessor2InterfaceId = FeatureProcessorFactory::Get()->GetFeatureProcessorInterfaceTypeId(FeatureProcessorId{ "TestFeatureProcessor2" });
        EXPECT_EQ(testFeatureProcessor2InterfaceId, TypeId::CreateNull());
    }

    //
    // Two implementations of the same interface
    //
    TEST_F(FeatureProcessorFactoryTests, CreateFeatureProcessor_MultipleImplmentationsOfTheSameInterface_CreatesBothFeatureProcessors)
    {
        FeatureProcessorPtr implementation1 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        FeatureProcessorPtr implementation2 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });
        EXPECT_TRUE(implementation1 != nullptr);
        EXPECT_TRUE(implementation2 != nullptr);
        EXPECT_NE(implementation1, implementation2);
    }

    TEST_F(FeatureProcessorFactoryTests, UnregisterFeatureProcessor_MultipleImplmentationsOfTheSameInterface_OnlyTestFeatureProcessorImplementationIsUnregistered)
    {
        FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation>();

        // TestFeatureProcessorImplementation can no longer be created because it has been unregistered
        FeatureProcessorPtr implementation1 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        FeatureProcessorPtr implementation2 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });

        EXPECT_TRUE(implementation1 == nullptr);
        EXPECT_TRUE(implementation2 != nullptr);
    }

    TEST_F(FeatureProcessorFactoryTests, UnregisterFeatureProcessor_MultipleImplmentationsOfTheSameInterface_OnlyTestFeatureProcessorImplementation2IsUnregistered)
    {
        FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<TestFeatureProcessorImplementation2>();

        // TestFeatureProcessorImplementation can no longer be created because it has been unregistered
        FeatureProcessorPtr implementation1 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation::RTTI_TypeName() });
        FeatureProcessorPtr implementation2 = FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorImplementation2::RTTI_TypeName() });

        EXPECT_TRUE(implementation1 != nullptr);
        EXPECT_TRUE(implementation2 == nullptr);
    }

    //
    // Invalid cases
    //
    TEST_F(FeatureProcessorFactoryTests, CreateFeatureProcessor_ByInterfaceName_FailsToCreate)
    {
        EXPECT_TRUE(AZ::RPI::FeatureProcessorFactory::Get()->CreateFeatureProcessor(FeatureProcessorId{ TestFeatureProcessorInterface::RTTI_TypeName() }) == nullptr);
    }

    // Get typeid from interface
}
