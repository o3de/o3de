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
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <CoreLights/EditorAreaLightComponent.h>

#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace UnitTest
{
    using EditorAreaLightComponentFixture = ::testing::Test;

    AZ::Render::AreaLightComponentConfig CreateAreaLightComponentConfig(
        const AZ::Render::AreaLightComponentConfig::LightType lightType,
        const float attenuationRadius,
        const bool enableShutters = false,
        const float innerShutterAngleDegrees = 0.0f,
        const float outerShutterAngleDegrees = 0.0f)
    {
        AZ::Render::AreaLightComponentConfig areaLightComponentConfig;
        areaLightComponentConfig.m_lightType = lightType;
        areaLightComponentConfig.m_attenuationRadiusMode = AZ::Render::LightAttenuationRadiusMode::Explicit;
        areaLightComponentConfig.m_attenuationRadius = attenuationRadius;
        areaLightComponentConfig.m_enableShutters = enableShutters;
        areaLightComponentConfig.m_innerShutterAngleDegrees = innerShutterAngleDegrees;
        areaLightComponentConfig.m_outerShutterAngleDegrees = outerShutterAngleDegrees;
        return areaLightComponentConfig;
    }

    static AZStd::unique_ptr<AZ::Entity> CreateEditorAreaLightEntity(
        const AZ::Render::AreaLightComponentConfig& areaLightComponentConfig,
        const AZStd::optional<AZ::TypeId>& shapeTypeId = AZStd::nullopt,
        const AZ::Vector3& shapeOffset = AZ::Vector3::CreateZero())
    {
        auto entity = AZStd::make_unique<AZ::Entity>();
        entity->Init();

        entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        if (shapeTypeId.has_value())
        {
            entity->CreateComponent(shapeTypeId.value());
        }
        entity->AddComponent(aznew AZ::Render::EditorAreaLightComponent(areaLightComponentConfig));
        entity->Activate();

        LmbrCentral::ShapeComponentRequestsBus::Event(
            entity->GetId(), &LmbrCentral::ShapeComponentRequests::SetTranslationOffset, shapeOffset);

        return entity;
    }

    static void SetCapsuleShapeHeightAndRadius(const AZ::EntityId entityId, const float height, const float radius)
    {
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entityId, &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, height);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entityId, &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    static void SetDiskShapeRadius(const AZ::EntityId entityId, const float radius)
    {
        LmbrCentral::DiskShapeComponentRequestBus::Event(entityId, &LmbrCentral::DiskShapeComponentRequestBus::Events::SetRadius, radius);
    }

    static void SetQuadShapeWidthAndHeight(const AZ::EntityId entityId, const float width, const float height)
    {
        LmbrCentral::QuadShapeComponentRequestBus::Event(entityId, &LmbrCentral::QuadShapeComponentRequestBus::Events::SetQuadWidth, width);
        LmbrCentral::QuadShapeComponentRequestBus::Event(
            entityId, &LmbrCentral::QuadShapeComponentRequestBus::Events::SetQuadHeight, height);
    }

    static void SetPolygonShapeVertices(const AZ::EntityId entityId, const AZStd::vector<AZ::Vector2>& vertices)
    {
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(
            entityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::SetVertices, vertices);
    }

    static void SetSphereShapeRadius(const AZ::EntityId entityId, const float radius)
    {
        LmbrCentral::SphereShapeComponentRequestsBus::Event(
            entityId, &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaCapsuleLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::CapsuleLightFeatureProcessorInterface on the scene.");

        // capsule shape contained within attenuation radius (effectively a sphere)
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 10.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 10.0f, 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f), AZ::Vector3(10.0f))));
        }

        // capsule shape contained within attenuation radius with capsule height contributing to overall height
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 10.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 40.0f, 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f, -10.0f, -20.0f), AZ::Vector3(10.0f, 10.0f, 20.0f))));
        }

        // attenuation radius contained within capsule shape
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 10.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 40.0f, 15.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f, -15.0f, -20.0f), AZ::Vector3(15.0f, 15.0f, 20.0f))));
        }

        // attenuation radius contained within capsule shape (now effectively a sphere)
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 10.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 50.0f, 25.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-25.0f), AZ::Vector3(25.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaCapsuleLightWithShapeTranslationOffsetBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::CapsuleLightFeatureProcessorInterface on the scene.");

        // capsule shape contained within attenuation radius (effectively a sphere)
        {
            const AZ::Vector3 shapeTranslationOffset(4.0f, 7.0f, 2.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 15.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId, shapeTranslationOffset);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 12.0f, 1.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-11.0f, -8.0f, -13.0f), AZ::Vector3(19.0f, 22.0f, 17.0f))));
        }

        // capsule shape contained within attenuation radius with capsule height contributing to overall height
        {
            const AZ::Vector3 shapeTranslationOffset(6.0f, -11.0f, 13.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 5.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId, shapeTranslationOffset);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 30.0f, 4.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(1.0f, -16.0f, -2.0f), AZ::Vector3(11.0f, -6.0f, 28.0f))));
        }

        // attenuation radius contained within capsule shape
        {
            const AZ::Vector3 shapeTranslationOffset(-7.0f, -7.0f, 4.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 5.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId, shapeTranslationOffset);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 50.0f, 12.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-19.0f, -19.0f, -21.0f), AZ::Vector3(5.0f, 5.0f, 29.0f))));
        }

        // attenuation radius contained within capsule shape (now effectively a sphere)
        {
            const AZ::Vector3 shapeTranslationOffset(8.0f, -13.0f, 9.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 8.0f),
                LmbrCentral::EditorCapsuleShapeComponentTypeId, shapeTranslationOffset);

            SetCapsuleShapeHeightAndRadius(entity->GetId(), 30.0f, 20.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-12.0f, -33.0f, -11.0f), AZ::Vector3(28.0f, 7.0f, 29.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaCapsuleLightSurfaceArea)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::CapsuleLightFeatureProcessorInterface on the scene.");

        auto entity = CreateEditorAreaLightEntity(
            CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Capsule, 10.0f),
            LmbrCentral::EditorCapsuleShapeComponentTypeId);

        // note: radius will be 10.0 after scale is applied
        SetCapsuleShapeHeightAndRadius(entity->GetId(), 20.0f, 5.0f);
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalUniformScale, 2.0f);

        // 4.0f * Pi * radius * radius - both caps make a sphere
        // 2.0f * Pi * radius * innerHeight - cylindrical area of capsule
        float lightDelegateSurfaceArea = 0.0f;
        AZ::Render::AreaLightRequestBus::EventResult(
            lightDelegateSurfaceArea, entity->GetId(), &AZ::Render::AreaLightRequestBus::Events::GetSurfaceArea);

        using ::testing::FloatNear;
        EXPECT_THAT(lightDelegateSurfaceArea, FloatNear(2513.27412287f, AZ::Constants::FloatEpsilon));
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaSpotDiskLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::DiskLightFeatureProcessorInterface on the scene.");

        // inner angle smaller and taller
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SpotDisk, 10.0f, true, 15.0f, 30.0f),
                LmbrCentral::EditorDiskShapeComponentTypeId);

            SetDiskShapeRadius(entity->GetId(), 1.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-6.0f, -6.0f, 0.0f), AZ::Vector3(6.0f, 6.0f, 9.65926f))));
        }

        // inner angle smaller and taller with larger base radius
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SpotDisk, 10.0f, true, 15.0f, 30.0f),
                LmbrCentral::EditorDiskShapeComponentTypeId);

            SetDiskShapeRadius(entity->GetId(), 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-7.0f, -7.0f, 0.0f), AZ::Vector3(7.0f, 7.0f, 9.65926f))));
        }

        // inner angle larger and clamped to outer angle
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SpotDisk, 10.0f, true, 40.0f, 30.0f),
                LmbrCentral::EditorDiskShapeComponentTypeId);

            SetDiskShapeRadius(entity->GetId(), 1.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-6.0f, -6.0f, 0.0f), AZ::Vector3(6.0f, 6.0f, 8.66025f))));
        }

        // inner and outer angle the same, wide angle
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SpotDisk, 5.0f, true, 60.0f, 60.0f),
                LmbrCentral::EditorDiskShapeComponentTypeId);

            SetDiskShapeRadius(entity->GetId(), 2.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(
                aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-6.33013f, -6.33013f, 0.0f), AZ::Vector3(6.33013f, 6.33013f, 2.5f))));
        }

        // inner and outer angles disabled
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SpotDisk, 8.0f),
                LmbrCentral::EditorDiskShapeComponentTypeId);

            SetDiskShapeRadius(entity->GetId(), 5.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(
                aabb,
                IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-8.38095f, -8.38095f, 0.0f), AZ::Vector3(8.38095f, 8.38095f, 7.25046f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaSimpleSpotLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound(
            "Unable to find a AZ::Render::SimpleSpotLightFeatureProcessorInterface on the scene.");

        // inner angle smaller and taller
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SimpleSpot, 10.0f, true, 15.0f, 30.0f));

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-5.0f, -5.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 9.65926f))));
        }

        // inner angle larger and clamped to outer angle
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SimpleSpot, 10.0f, true, 40.0f, 30.0f));

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-5.0f, -5.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 8.66025f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaQuadLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::QuadLightFeatureProcessorInterface on the scene.");

        // quad contained within attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Quad, 15.0f),
                LmbrCentral::EditorQuadShapeComponentTypeId);

            SetQuadShapeWidthAndHeight(entity->GetId(), 5.0f, 5.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f), AZ::Vector3(15.0f))));
        }

        // quad larger than attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Quad, 15.0f),
                LmbrCentral::EditorQuadShapeComponentTypeId);

            SetQuadShapeWidthAndHeight(entity->GetId(), 50.0f, 50.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-25.0f, -25.0f, -15.0f), AZ::Vector3(25.0f, 25.0f, 15.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaPolygonLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::PolygonLightFeatureProcessorInterface on the scene.");

        // polygon contained within attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Polygon, 15.0f),
                LmbrCentral::EditorPolygonPrismShapeComponentTypeId);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f), AZ::Vector3(15.0f))));
        }

        // polygon outside attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Polygon, 15.0f),
                LmbrCentral::EditorPolygonPrismShapeComponentTypeId);

            SetPolygonShapeVertices(
                entity->GetId(),
                AZStd::vector<AZ::Vector2>{
                    AZ::Vector2(-50.0f, -50.0f), AZ::Vector2(50.0f, -50.0f), AZ::Vector2(50.0f, 50.0f), AZ::Vector2(-50.0f, 50.0f) });

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-50.0f, -50.0f, -15.0f), AZ::Vector3(50.0f, 50.0f, 15.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaPointSphereLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::PointLightFeatureProcessorInterface on the scene.");

        // sphere shape contained within attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Sphere, 15.0f),
                LmbrCentral::EditorSphereShapeComponentTypeId);

            SetSphereShapeRadius(entity->GetId(), 5.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f), AZ::Vector3(15.0f))));
        }

        // sphere shape contained within attenuation sphere
        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Sphere, 15.0f),
                LmbrCentral::EditorSphereShapeComponentTypeId);

            SetSphereShapeRadius(entity->GetId(), 30.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-30.0f), AZ::Vector3(30.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaPointSphereLightWithShapeTranslationOffsetBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::PointLightFeatureProcessorInterface on the scene.");

        // sphere shape contained within attenuation sphere
        {
            const AZ::Vector3 shapeTranslationOffset(5.0f, 8.0f, -7.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Sphere, 20.0f),
                LmbrCentral::EditorSphereShapeComponentTypeId, shapeTranslationOffset);

            SetSphereShapeRadius(entity->GetId(), 8.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f, -12.0f, -27.0f), AZ::Vector3(25.0f, 28.0f, 13.0f))));
        }

        // sphere shape contained within attenuation sphere
        {
            const AZ::Vector3 shapeTranslationOffset(-7.0f, -2.0f, 6.0f);
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Sphere, 12.0f),
                LmbrCentral::EditorSphereShapeComponentTypeId, shapeTranslationOffset);

            SetSphereShapeRadius(entity->GetId(), 25.0f);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-32.0f, -27.0f, -19.0f), AZ::Vector3(18.0f, 23.0f, 31.0f))));
        }
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaPointSphereLightSurfaceArea)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a AZ::Render::PointLightFeatureProcessorInterface on the scene.");

        auto entity = CreateEditorAreaLightEntity(
            CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::Sphere, 10.0f),
            LmbrCentral::EditorSphereShapeComponentTypeId);

        // note: radius will be 10.0 after scale is applied
        SetSphereShapeRadius(entity->GetId(), 5.0f);
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalUniformScale, 2.0f);

        // 4.0f * Pi * radius * radius
        float lightDelegateSurfaceArea = 0.0f;
        AZ::Render::AreaLightRequestBus::EventResult(
            lightDelegateSurfaceArea, entity->GetId(), &AZ::Render::AreaLightRequestBus::Events::GetSurfaceArea);

        using ::testing::FloatNear;
        EXPECT_THAT(lightDelegateSurfaceArea, FloatNear(1256.6370614f, AZ::Constants::FloatEpsilon));
    }

    TEST_F(EditorAreaLightComponentFixture, CheckEditorAreaSimplePointLightBounds)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound(
            "Unable to find a AZ::Render::SimplePointLightFeatureProcessorInterface on the scene.");

        {
            auto entity = CreateEditorAreaLightEntity(
                CreateAreaLightComponentConfig(AZ::Render::AreaLightComponentConfig::LightType::SimplePoint, 15.0f),
                LmbrCentral::EditorSphereShapeComponentTypeId);

            const AZ::Aabb aabb = AzFramework::CalculateEntityLocalBoundsUnion(entity.get());
            EXPECT_THAT(aabb, IsClose(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-15.0f), AZ::Vector3(15.0f))));
        }
    }
} // namespace UnitTest
