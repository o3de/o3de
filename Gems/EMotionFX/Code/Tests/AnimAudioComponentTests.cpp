/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SystemComponentFixture.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzFramework/Components/TransformComponent.h>
#include <Integration/Components/AnimAudioComponent.h>

namespace EMotionFX
{
    class MockAudioProxyComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockAudioProxyComponent, "{DF130DF1-AE9D-486A-8015-3D7FD64DC4C0}");
        void Activate() override {}
        void Deactivate() override {}
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("AudioProxyService", 0x7da4c79c)); }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC("AudioProxyService", 0x7da4c79c)); }
        static void Reflect(AZ::ReflectContext*) {}
    };

    class MockMeshComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockMeshComponent, "{876F6E19-D67A-4966-81AE-0F34931602CA}");
        void Activate() override {}
        void Deactivate() override {}
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided) { provided.push_back(AZ_CRC("MeshService", 0x71d8a455)); }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible) { incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455)); }
        static void Reflect(AZ::ReflectContext*) {}
    };

    class MockActorComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockActorComponent, "{6B485E07-8466-4FD1-A7F9-D3D234201F5D}");
        void Activate() override {}
        void Deactivate() override {}
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("CharacterPhysicsDataService", 0x34757927));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
            incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext*) {}
    };

    class ComponentDependencyFixture
        : public SystemComponentFixture
    {
    public:
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_mockAudioProxyCompDesc = aznew MockAudioProxyComponent::DescriptorType;
            m_mockMeshCompDesc = aznew MockMeshComponent::DescriptorType;
            m_mockActorCompDesc = aznew MockActorComponent::DescriptorType;
            m_animAudioCompDesc = aznew EMotionFX::Integration::AnimAudioComponent::DescriptorType;
            m_txfmCompDesc = aznew AzFramework::TransformComponent::DescriptorType;

            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::RegisterComponentDescriptor, m_mockAudioProxyCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::RegisterComponentDescriptor, m_mockMeshCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::RegisterComponentDescriptor, m_mockActorCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::RegisterComponentDescriptor, m_animAudioCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::RegisterComponentDescriptor, m_txfmCompDesc);
        }

        void TearDown() override
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::UnregisterComponentDescriptor, m_mockAudioProxyCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::UnregisterComponentDescriptor, m_mockMeshCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::UnregisterComponentDescriptor, m_mockActorCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::UnregisterComponentDescriptor, m_animAudioCompDesc);
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::UnregisterComponentDescriptor, m_txfmCompDesc);

            delete m_mockMeshCompDesc;
            delete m_mockAudioProxyCompDesc;
            delete m_mockActorCompDesc;
            delete m_animAudioCompDesc;
            delete m_txfmCompDesc;

            SystemComponentFixture::TearDown();
        }

    protected:
        MockAudioProxyComponent::DescriptorType* m_mockAudioProxyCompDesc = nullptr;
        MockMeshComponent::DescriptorType* m_mockMeshCompDesc = nullptr;
        MockActorComponent::DescriptorType* m_mockActorCompDesc = nullptr;
        EMotionFX::Integration::AnimAudioComponent::DescriptorType* m_animAudioCompDesc = nullptr;
        AzFramework::TransformComponent::DescriptorType* m_txfmCompDesc = nullptr;
    };

    TEST_F(ComponentDependencyFixture, AnimAudioComponent_ResolveComponentDependencies)
    {
        AZ::Entity* entity = aznew AZ::Entity();

        entity->CreateComponent<AzFramework::TransformComponent>();
        entity->CreateComponent<EMotionFX::Integration::AnimAudioComponent>();
        entity->CreateComponent<MockAudioProxyComponent>();
        EXPECT_EQ(AZ::Entity::DependencySortResult::MissingRequiredService, entity->EvaluateDependencies());

        // Add a "MeshService", services still not met.
        auto meshComp = entity->CreateComponent<MockMeshComponent>();
        EXPECT_EQ(AZ::Entity::DependencySortResult::MissingRequiredService, entity->EvaluateDependencies());

        // Add an "EMotionFXActorService", incompatible with "MeshService" because MockActorComponent already provides "MeshService".
        entity->CreateComponent<MockActorComponent>();
        EXPECT_EQ(AZ::Entity::DependencySortResult::HasIncompatibleServices, entity->EvaluateDependencies());

        // Remove MeshComponent, services should resolve now.
        entity->RemoveComponent(meshComp);
        EXPECT_EQ(AZ::Entity::DependencySortResult::Success, entity->EvaluateDependencies());

        delete meshComp;
        delete entity;
    }

} // namespace EMotionFX
