/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/RPISystem.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/System/SceneDescriptor.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <CoreLights/EditorAreaLightComponent.h>

#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

namespace UnitTest
{
    class EditorAreaLightComponentFixture : public ::testing::Test
    {
    };

    static AZStd::unique_ptr<AZ::Entity> CreateEditorAreaLightEntity(
        const AZ::Render::AreaLightComponentConfig::LightType lightType, const AZ::TypeId& shapeTypeId, const float attenuationRadius)
    {
        auto entity = AZStd::make_unique<AZ::Entity>();
        entity->Init();

        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        entity->CreateComponent(shapeTypeId);

        AZ::Render::AreaLightComponentConfig areaLightComponentConfig;
        areaLightComponentConfig.m_lightType = lightType;
        areaLightComponentConfig.m_attenuationRadiusMode = AZ::Render::LightAttenuationRadiusMode::Explicit;
        areaLightComponentConfig.m_attenuationRadius = attenuationRadius;

        entity->AddComponent(aznew AZ::Render::EditorAreaLightComponent(areaLightComponentConfig));
        entity->Activate();

        return entity;
    }

    static void SetCapsuleShapeHeightAndRadius(const AZ::EntityId entityId, const float height, const float radius)
    {
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entityId, &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, height);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entityId, &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaCapsuleLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler physxCookFailed("Unable to find a AZ::Render::CapsuleLightFeatureProcessorInterface on the scene.");

        // capsule shape contained within attenuation radius (effectively a sphere)
        {
            auto entity = CreateEditorAreaLightEntity(
                AZ::Render::AreaLightComponentConfig::LightType::Capsule, LmbrCentral::EditorCapsuleShapeComponentTypeId, 10.0f);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 10.0f, 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f), AZ::Vector3(10.0f))));
        }

        // capsule shape contained within attenuation radius with capsule height contributing to overall height
        {
            auto entity = CreateEditorAreaLightEntity(
                AZ::Render::AreaLightComponentConfig::LightType::Capsule, LmbrCentral::EditorCapsuleShapeComponentTypeId, 10.0f);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 40.0f, 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f, -10.0f, -20.0f), AZ::Vector3(10.0f, 10.0f, 20.0f))));
        }

        // attenuation radius contained within capsule shape
        {
            auto entity = CreateEditorAreaLightEntity(
                AZ::Render::AreaLightComponentConfig::LightType::Capsule, LmbrCentral::EditorCapsuleShapeComponentTypeId, 10.0f);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 40.0f, 15.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f, -15.0f, -20.0f), AZ::Vector3(15.0f, 15.0f, 20.0f))));
        }

        // attenuation radius contained within capsule shape (now effectively a sphere)
        {
            auto entity = CreateEditorAreaLightEntity(
                AZ::Render::AreaLightComponentConfig::LightType::Capsule, LmbrCentral::EditorCapsuleShapeComponentTypeId, 10.0f);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 50.0f, 25.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-25.0f), AZ::Vector3(25.0f))));
        }
    }
} // namespace UnitTest
