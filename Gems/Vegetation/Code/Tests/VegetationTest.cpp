/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "VegetationTest.h"
#include "VegetationMocks.h"

//////////////////////////////////////////////////////////////////////////

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Source/Components/AreaBlenderComponent.h>

#include <Source/Components/AreaBlenderComponent.h>
#include <Source/Components/BlockerComponent.h>
#include <Source/Components/DescriptorListCombinerComponent.h>
#include <Source/Components/DescriptorListComponent.h>
#include <Source/Components/DescriptorWeightSelectorComponent.h>
#include <Source/Components/DistanceBetweenFilterComponent.h>
#include <Source/Components/DistributionFilterComponent.h>
#include <Source/Components/LevelSettingsComponent.h>
#include <Source/Components/MeshBlockerComponent.h>
#include <Source/Components/PositionModifierComponent.h>
#include <Source/Components/RotationModifierComponent.h>
#include <Source/Components/ScaleModifierComponent.h>
#include <Source/Components/ShapeIntersectionFilterComponent.h>
#include <Source/Components/SlopeAlignmentModifierComponent.h>
#include <Source/Components/SpawnerComponent.h>
#include <Source/Components/SurfaceAltitudeFilterComponent.h>
#include <Source/Components/SurfaceMaskDepthFilterComponent.h>
#include <Source/Components/SurfaceMaskFilterComponent.h>
#include <Source/Components/SurfaceSlopeFilterComponent.h>

#include <Source/Debugger/AreaDebugComponent.h>

namespace UnitTest
{
    struct VegetationComponentTestsBasics
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
        }

        template <typename Component, typename MockComponent1>
        void CreateWith()
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            AZ::Entity entity;
            entity.CreateComponent<Component>();
            entity.CreateComponent<MockComponent1>();

            entity.Init();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::State::Init);

            entity.Activate();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::State::Active);

            entity.Deactivate();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::State::Init);
        }

        template <typename ComponentA, typename ComponentB>
        bool IsComponentCompatible()
        {
            AZ::ComponentDescriptor::DependencyArrayType providedServicesA;
            ComponentA::GetProvidedServices(providedServicesA);

            AZ::ComponentDescriptor::DependencyArrayType incompatibleServicesB;
            ComponentB::GetIncompatibleServices(incompatibleServicesB);

            for (auto providedServiceA : providedServicesA)
            {
                for (auto incompatibleServiceB : incompatibleServicesB)
                {
                    if (providedServiceA == incompatibleServiceB)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        template <typename ComponentA, typename ComponentB>
        bool AreComponentsCompatible()
        {
            return IsComponentCompatible<ComponentA, ComponentB>() && IsComponentCompatible<ComponentB, ComponentA>();
        }

        bool BeginElementMinMaxTests(
            AZ::SerializeContext* sc,
            void* instance,
            const AZ::SerializeContext::ClassData* classData,
            const AZ::SerializeContext::ClassElement* classElement)
        {
            (void)instance;

            if (classElement)
            {
                // if we are a pointer, then we may be pointing to a derived type.
                if (classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                {
                    // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
                    instance = *(void**)(instance);
                    if (instance && classElement->m_azRtti)
                    {
                        AZ::Uuid actualClassId = classElement->m_azRtti->GetActualUuid(instance);
                        if (actualClassId != classElement->m_typeId)
                        {
                            // we are pointing to derived type, adjust class data, uuid and pointer.
                            classData = sc->FindClassData(actualClassId);
                            if (classData)
                            {
                                instance = classElement->m_azRtti->Cast(instance, classData->m_azRtti->GetTypeId());
                            }
                        }
                    }
                }
            }

            //check editor data numeric elements for min/max attributes
            if (classElement &&
                classElement->m_editData)
            {
                if (classElement->m_editData->m_elementId == AZ::Edit::UIHandlers::Default ||
                    classElement->m_editData->m_elementId == AZ::Edit::UIHandlers::Slider ||
                    classElement->m_editData->m_elementId == AZ::Edit::UIHandlers::SpinBox)
                {
                    if (classElement->m_typeId == azrtti_typeid<AZ::u64>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::u32>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::u16>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::u8>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::s64>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::s32>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::s16>() ||
                        classElement->m_typeId == azrtti_typeid<AZ::s8>() ||
                        classElement->m_typeId == azrtti_typeid<float>() ||
                        classElement->m_typeId == azrtti_typeid<double>())
                    {
                        EXPECT_TRUE(classElement->m_editData->FindAttribute(AZ::Edit::Attributes::Min) != nullptr);
                        EXPECT_TRUE(classElement->m_editData->FindAttribute(AZ::Edit::Attributes::Max) != nullptr);
                    }
                }
            }
            return true;
        }

        bool EndfElementMinMaxTests()
        {
            return true;
        }

        template<typename T>
        void ValidateHasMinMaxRanges()
        {
            //create a serialize context with edit context enabled
            AZ::SerializeContext serializeContext(true, true);

            //must reflect AZ::Entity to register AZ::ComponentConfig so new serialize context asserts don't fail test
            AZ::Entity::Reflect(&serializeContext);

            //reflect and inspect the object
            T::Reflect(&serializeContext);

            T object;
            serializeContext.EnumerateObject(&object,
                AZStd::bind(&VegetationComponentTestsBasics::BeginElementMinMaxTests, this, &serializeContext, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3),
                AZStd::bind(&VegetationComponentTestsBasics::EndfElementMinMaxTests, this),
                AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
        }
    };

    TEST_F(VegetationComponentTestsBasics, VerifyCompatibility)
    {
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::DescriptorWeightSelectorComponent, Vegetation::DescriptorWeightSelectorComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::PositionModifierComponent, Vegetation::PositionModifierComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::RotationModifierComponent, Vegetation::RotationModifierComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::ScaleModifierComponent, Vegetation::ScaleModifierComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::DescriptorListComponent, Vegetation::DescriptorListComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::DescriptorListComponent, Vegetation::DescriptorListCombinerComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::DescriptorListCombinerComponent, Vegetation::DescriptorListComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::DescriptorListCombinerComponent, Vegetation::DescriptorListCombinerComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::AreaBlenderComponent, Vegetation::AreaBlenderComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::AreaBlenderComponent, Vegetation::BlockerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::AreaBlenderComponent, Vegetation::SpawnerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::AreaBlenderComponent, Vegetation::MeshBlockerComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::BlockerComponent, Vegetation::AreaBlenderComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::BlockerComponent, Vegetation::BlockerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::BlockerComponent, Vegetation::SpawnerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::BlockerComponent, Vegetation::MeshBlockerComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::SpawnerComponent, Vegetation::AreaBlenderComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::SpawnerComponent, Vegetation::BlockerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::SpawnerComponent, Vegetation::SpawnerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::SpawnerComponent, Vegetation::MeshBlockerComponent>()));

        EXPECT_FALSE((AreComponentsCompatible<Vegetation::MeshBlockerComponent, Vegetation::AreaBlenderComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::MeshBlockerComponent, Vegetation::BlockerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::MeshBlockerComponent, Vegetation::SpawnerComponent>()));
        EXPECT_FALSE((AreComponentsCompatible<Vegetation::MeshBlockerComponent, Vegetation::MeshBlockerComponent>()));
    }

    TEST_F(VegetationComponentTestsBasics, CreateEach)
    {
        CreateWith<Vegetation::AreaBlenderComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::BlockerComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::DescriptorListCombinerComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DescriptorListComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DescriptorWeightSelectorComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DistanceBetweenFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DistributionFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::LevelSettingsComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::MeshBlockerComponent, MockMeshServiceComponent>();
        CreateWith<Vegetation::PositionModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::RotationModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::ScaleModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::ShapeIntersectionFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SlopeAlignmentModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SpawnerComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::SurfaceAltitudeFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceMaskDepthFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceMaskFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceSlopeFilterComponent, MockVegetationAreaServiceComponent>();
    }

    TEST_F(VegetationComponentTestsBasics, LevelSettingsComponent)
    {
        MockSystemConfigurationRequestBus mockSystemConfigurationRequestBus;

        struct TestLevelSettingsComponent
            : public Vegetation::LevelSettingsComponent
        {
            const Vegetation::LevelSettingsConfig& Config() const
            {
                return m_configuration;
            }
        };

        // Provide a default configuration to the system component
        constexpr int defaultProcessTime = 7;
        Vegetation::InstanceSystemConfig defaultSystemConfig;
        defaultSystemConfig.m_maxInstanceProcessTimeMicroseconds = defaultProcessTime;

        mockSystemConfigurationRequestBus.UpdateSystemConfig(&defaultSystemConfig);
        const auto* instConfig = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
        ASSERT_TRUE(instConfig != nullptr);
        EXPECT_EQ(defaultProcessTime, instConfig->m_maxInstanceProcessTimeMicroseconds);

        {
            // Create a level settings component with a different config that should override the system component
            Vegetation::LevelSettingsComponent* component = nullptr;
            Vegetation::LevelSettingsConfig config;
            config.m_instanceSystemConfig.m_maxInstanceProcessTimeMicroseconds = 13;

            auto entity = CreateEntity(config, &component);

            const auto* instConfig2 = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
            ASSERT_TRUE(instConfig2 != nullptr);
            EXPECT_EQ(13, instConfig2->m_maxInstanceProcessTimeMicroseconds);

            TestLevelSettingsComponent* tester = static_cast<TestLevelSettingsComponent*>(component);
            EXPECT_EQ(13, tester->Config().m_instanceSystemConfig.m_maxInstanceProcessTimeMicroseconds);
        }

        // Entity should be out of scope now (destroyed), so the default settings should be restored
        instConfig = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
        EXPECT_EQ(defaultProcessTime, instConfig->m_maxInstanceProcessTimeMicroseconds);
    }

    TEST_F(VegetationComponentTestsBasics, Components_HaveMinMaxRanges)
    {
        ValidateHasMinMaxRanges<Vegetation::AreaSystemConfig>();
        ValidateHasMinMaxRanges<Vegetation::InstanceSystemConfig>();
        ValidateHasMinMaxRanges<Vegetation::AreaDebugConfig>();
    }
}

//////////////////////////////////////////////////////////////////////////

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
