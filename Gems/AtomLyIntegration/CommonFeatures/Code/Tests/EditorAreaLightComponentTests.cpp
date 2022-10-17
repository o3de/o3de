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

    TEST_F(EditorAreaLightComponentFixture, CheckCapsuleAreaLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler physxCookFailed("Unable to find a AZ::Render::CapsuleLightFeatureProcessorInterface on the scene.");

        {
            auto entity = AZStd::make_unique<AZ::Entity>();
            entity->Init();

            AZ::Render::AreaLightComponentConfig areaLightComponentConfig;
            areaLightComponentConfig.m_attenuationRadiusMode = AZ::Render::LightAttenuationRadiusMode::Explicit;
            areaLightComponentConfig.m_attenuationRadius = 10.0f;
            areaLightComponentConfig.m_lightType = AZ::Render::AreaLightComponentConfig::LightType::Capsule;

            entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            entity->CreateComponent(LmbrCentral::CapsuleShapeComponentTypeId);
            entity->AddComponent(aznew AZ::Render::EditorAreaLightComponent(areaLightComponentConfig));
            entity->Activate();

            LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
                entity->GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, 10.0f);
            LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
                entity->GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, 2.0f);

            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            AzFramework::BoundsRequestBus::EventResult(aabb, entity->GetId(), &AzFramework::BoundsRequestBus::Events::GetLocalBounds);

            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f), AZ::Vector3(10.0f))));
        }

        {
            auto entity = AZStd::make_unique<AZ::Entity>();
            entity->Init();

            AZ::Render::AreaLightComponentConfig areaLightComponentConfig;
            areaLightComponentConfig.m_attenuationRadiusMode = AZ::Render::LightAttenuationRadiusMode::Explicit;
            areaLightComponentConfig.m_attenuationRadius = 5.0f;
            areaLightComponentConfig.m_lightType = AZ::Render::AreaLightComponentConfig::LightType::Capsule;

            entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            entity->CreateComponent(LmbrCentral::CapsuleShapeComponentTypeId);
            entity->AddComponent(aznew AZ::Render::EditorAreaLightComponent(areaLightComponentConfig));
            entity->Activate();

            LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
                entity->GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, 30.0f);
            LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
                entity->GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, 2.0f);

            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            AzFramework::BoundsRequestBus::EventResult(aabb, entity->GetId(), &AzFramework::BoundsRequestBus::Events::GetLocalBounds);

            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-5.0f, -5.0f, -15.0f), AZ::Vector3(5.0f, 5.0f, 15.0f))));
        }
    }
} // namespace UnitTest
