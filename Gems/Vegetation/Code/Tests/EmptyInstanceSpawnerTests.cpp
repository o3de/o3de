/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzCore/Component/Entity.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <Vegetation/EmptyInstanceSpawner.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>

namespace UnitTest
{
    // Mock VegetationSystemComponent is needed to reflect only the EmptyInstanceSpawner.
    class MockEmptyInstanceVegetationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockEmptyInstanceVegetationSystemComponent, "{B2AF429A-4E3A-4A59-A425-5A191733D24A}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect)
        {
            Vegetation::InstanceSpawner::Reflect(reflect);
            Vegetation::EmptyInstanceSpawner::Reflect(reflect);
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("VegetationSystemService"));
        }
    };

    class EmptyInstanceSpawnerTests
        : public VegetationComponentTests
        , public Vegetation::DescriptorNotificationBus::Handler
    {
    public:
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockEmptyInstanceVegetationSystemComponent::CreateDescriptor());
        }

        void OnDescriptorAssetsLoaded() override { m_numOnLoadedCalls++; }

        int m_numOnLoadedCalls = 0;

    };

    TEST_F(EmptyInstanceSpawnerTests, BasicInitializationTest)
    {
        // Basic test to make sure we can construct / destroy without errors.

        Vegetation::EmptyInstanceSpawner instanceSpawner;
    }

    TEST_F(EmptyInstanceSpawnerTests, SpawnersAlwaysEqual)
    {
        // Two different instances of the EmptyInstanceSpawner should always be considered
        // data-equivalent.

        Vegetation::EmptyInstanceSpawner instanceSpawner1;
        Vegetation::EmptyInstanceSpawner instanceSpawner2;

        EXPECT_TRUE(instanceSpawner1 == instanceSpawner2);
    }

    TEST_F(EmptyInstanceSpawnerTests, LoadAndUnloadAssets)
    {
        // The spawner should successfully pretend to load/unload assets without errors.
        // ("Pretend" because an EmptyInstanceSpawner has no assets)

        Vegetation::EmptyInstanceSpawner instanceSpawner;
        Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);
        instanceSpawner.LoadAssets();

        // We expect this to be called immediately during LoadAssets for EmptyInstanceSpawner, so there's
        // no need to wait before checking it.
        EXPECT_TRUE(m_numOnLoadedCalls == 1);

        instanceSpawner.UnloadAssets();
        Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
    }

    TEST_F(EmptyInstanceSpawnerTests, CreateAndDestroyInstance)
    {
        // The spawner should successfully "create" and "destroy" an instance without errors.

        Vegetation::EmptyInstanceSpawner instanceSpawner;
        Vegetation::InstanceData instanceData;
        Vegetation::InstancePtr instance = instanceSpawner.CreateInstance(instanceData);
        EXPECT_TRUE(instance);
        instanceSpawner.DestroyInstance(0, instance);
    }

    TEST_F(EmptyInstanceSpawnerTests, SpawnerRegisteredWithDescriptor)
    {
        // Validate that the Descriptor successfully gets EmptyInstanceSpawner registered with it,
        // as long as InstanceSpawner and EmptyInstanceSpawner have been reflected.

        MockEmptyInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        Vegetation::Descriptor descriptor;
        descriptor.RefreshSpawnerTypeList();
        auto spawnerTypes = descriptor.GetSpawnerTypeList();
        EXPECT_TRUE(spawnerTypes.size() == 1);
        EXPECT_TRUE(spawnerTypes[0].first == Vegetation::EmptyInstanceSpawner::RTTI_Type());
    }

    TEST_F(EmptyInstanceSpawnerTests, DescriptorCreatesCorrectSpawner)
    {
        // Validate that the Descriptor successfully creates a new EmptyInstanceSpawner if we change
        // the spawner type on the Descriptor.

        MockEmptyInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        // We expect the Descriptor to start off with a Legacy Vegetation spawner, but then should correctly get an
        // EmptyInstanceSpawner after we change spawnerType.
        Vegetation::Descriptor descriptor;
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) != Vegetation::EmptyInstanceSpawner::RTTI_Type());
        descriptor.m_spawnerType = Vegetation::EmptyInstanceSpawner::RTTI_Type();
        descriptor.RefreshSpawnerTypeList();
        descriptor.SpawnerTypeChanged();
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) == Vegetation::EmptyInstanceSpawner::RTTI_Type());
    }
}
