/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>
#include <Tests/GradientSignalTestHelpers.h>

#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Atom/RPI.Public/RPISystem.h>
#include <Common/RHI/Factory.h>
#include <Common/RHI/Stubs.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <AzFramework/Components/TransformComponent.h>
#include <GradientSignal/Components/GradientSurfaceDataComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>

// Base gradient components
#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/ShapeAreaFalloffGradientComponent.h>

// Gradient modifier components
#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/InvertGradientComponent.h>
#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <GradientSignal/Components/MixedGradientComponent.h>
#include <GradientSignal/Components/PosterizeGradientComponent.h>
#include <GradientSignal/Components/ReferenceGradientComponent.h>
#include <GradientSignal/Components/SmoothStepGradientComponent.h>
#include <GradientSignal/Components/ThresholdGradientComponent.h>

// Gradient surface data components
#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <GradientSignal/Components/SurfaceSlopeGradientComponent.h>

namespace UnitTest
{
    void GradientSignalTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "LmbrCentral", "SurfaceData" });

        AddComponentDescriptors({
            AzFramework::TransformComponent::CreateDescriptor(),

            GradientSignal::ConstantGradientComponent::CreateDescriptor(),
            GradientSignal::DitherGradientComponent::CreateDescriptor(),
            GradientSignal::GradientSurfaceDataComponent::CreateDescriptor(),
            GradientSignal::GradientTransformComponent::CreateDescriptor(),
            GradientSignal::ImageGradientComponent::CreateDescriptor(),
            GradientSignal::InvertGradientComponent::CreateDescriptor(),
            GradientSignal::LevelsGradientComponent::CreateDescriptor(),
            GradientSignal::MixedGradientComponent::CreateDescriptor(),
            GradientSignal::PerlinGradientComponent::CreateDescriptor(),
            GradientSignal::PosterizeGradientComponent::CreateDescriptor(),
            GradientSignal::RandomGradientComponent::CreateDescriptor(),
            GradientSignal::ReferenceGradientComponent::CreateDescriptor(),
            GradientSignal::ShapeAreaFalloffGradientComponent::CreateDescriptor(),
            GradientSignal::SmoothStepGradientComponent::CreateDescriptor(),
            GradientSignal::SurfaceAltitudeGradientComponent::CreateDescriptor(),
            GradientSignal::SurfaceMaskGradientComponent::CreateDescriptor(),
            GradientSignal::SurfaceSlopeGradientComponent::CreateDescriptor(),
            GradientSignal::ThresholdGradientComponent::CreateDescriptor(),

            MockShapeComponent::CreateDescriptor(),
            MockSurfaceProviderComponent::CreateDescriptor(),
            MockGradientSignal::CreateDescriptor()
        });
    }

    void GradientSignalTestEnvironment::PostCreateApplication()
    {
        // Ebus usage will allocate a global context on first usage. If that first usage occurs in a DLL, then the context will be
        // invalid on subsequent unit test runs if using gtest_repeat. However, if we force the ebus to create their global context in
        // the main test DLL (this one), the context will remain active throughout repeated runs. By creating them in
        // PostCreateApplication(), they will be created before the DLLs get loaded and any system components from those DLLs run, so we
        // can guarantee this will be the first usage.

        // These ebuses need their contexts created here before any of the dependent DLLs get loaded:
        AZ::AssetTypeInfoBus::GetOrCreateContext();
        SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext();
        SurfaceData::SurfaceDataProviderRequestBus::GetOrCreateContext();
        SurfaceData::SurfaceDataModifierRequestBus::GetOrCreateContext();
        LmbrCentral::ShapeComponentRequestsBus::GetOrCreateContext();
    }

    GradientSignalBaseFixture::GradientSignalBaseFixture()
    {
        // Even though this is an empty function, it needs to appear here so that the destruction code for the unique pointers
        // is created here instead of inline wherever the GradientSignalTest class is used. This lets us keep the #includes for
        // these Atom classes private to this file instead of exposing them outward through the header and requiring everything
        // using this class to include the Atom libs as well.
    }

    GradientSignalBaseFixture::~GradientSignalBaseFixture()
    {
        // This also needs to appear here, even though it's an empty function.
    }

    void GradientSignalBaseFixture::SetupCoreSystems()
    {
        // Create a stub RHI for use by Atom
        m_rhiFactory.reset(aznew UnitTest::StubRHI::Factory());

        // Create the Atom RPISystem
        AZ::RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<AZ::RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);

        AZ::RPI::ImageSystemDescriptor imageSystemDescriptor;
        m_imageSystem = AZStd::make_unique<AZ::RPI::ImageSystem>();
        m_imageSystem->Init(imageSystemDescriptor);
    }

    void GradientSignalBaseFixture::TearDownCoreSystems()
    {
        m_imageSystem->Shutdown();
        m_rpiSystem->Shutdown();

        m_imageSystem = nullptr;
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::CreateTestEntity(float shapeHalfBounds)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(shapeHalfBounds * 2.0f));
        auto boxComponent = testEntity->CreateComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId);
        boxComponent->SetConfiguration(boxConfig);

        // Create a transform that locates our gradient in the center of our desired Shape.
        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::CreateTestSphereEntity(float shapeRadius)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::SphereShapeConfig sphereConfig(shapeRadius);
        auto sphereComponent = testEntity->CreateComponent(LmbrCentral::SphereShapeComponentTypeId);
        sphereComponent->SetConfiguration(sphereConfig);

        // Create a transform that locates our gradient in the center of our desired Shape.
        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeRadius)));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestConstantGradient(float shapeHalfBounds, float value)
    {
        // Create a Constant Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ConstantGradientConfig config;
        config.m_value = value;
        entity->CreateComponent<GradientSignal::ConstantGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestImageGradient(float shapeHalfBounds)
    {
        // Create an Image Gradient Component with arbitrary sizes and parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ImageGradientConfig config;
        const uint32_t imageSize = 4096;
        const int32_t imageSeed = 12345;
        config.m_imageAsset = UnitTest::CreateImageAsset(imageSize, imageSize, imageSeed);
        config.m_tiling = AZ::Vector2::CreateOne();
        entity->CreateComponent<GradientSignal::ImageGradientComponent>(config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestPerlinGradient(float shapeHalfBounds)
    {
        // Create a Perlin Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::PerlinGradientConfig config;
        config.m_amplitude = 1.0f;
        config.m_frequency = 1.1f;
        config.m_octave = 4;
        config.m_randomSeed = 12345;
        entity->CreateComponent<GradientSignal::PerlinGradientComponent>(config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestRandomGradient(float shapeHalfBounds)
    {
        // Create a Random Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = 12345;
        entity->CreateComponent<GradientSignal::RandomGradientComponent>(config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestShapeAreaFalloffGradient(float shapeHalfBounds)
    {
        // Create a Shape Area Falloff Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ShapeAreaFalloffGradientConfig config;
        config.m_shapeEntityId = entity->GetId();
        config.m_falloffWidth = 16.0f;
        config.m_falloffType = GradientSignal::FalloffType::InnerOuter;
        entity->CreateComponent<GradientSignal::ShapeAreaFalloffGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestDitherGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Dither Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::DitherGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_useSystemPointsPerUnit = false;
        // Use a number other than 1.0f for pointsPerUnit to ensure the dither math is getting exercised properly.
        config.m_pointsPerUnit = 0.25f;     
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        entity->CreateComponent<GradientSignal::DitherGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestInvertGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create an Invert Gradient Component.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::InvertGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        entity->CreateComponent<GradientSignal::InvertGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestLevelsGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Levels Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::LevelsGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_inputMin = 0.1f;
        config.m_inputMid = 0.3f;
        config.m_inputMax = 0.9f;
        config.m_outputMin = 0.0f;
        config.m_outputMax = 1.0f;
        entity->CreateComponent<GradientSignal::LevelsGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestMixedGradient(
        float shapeHalfBounds, const AZ::EntityId& baseGradientId, const AZ::EntityId& mixedGradientId)
    {
        // Create a Mixed Gradient Component that mixes two input gradients together in arbitrary ways.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::MixedGradientConfig config;

        GradientSignal::MixedGradientLayer layer;
        layer.m_enabled = true;

        layer.m_operation = GradientSignal::MixedGradientLayer::MixingOperation::Initialize;
        layer.m_gradientSampler.m_gradientId = baseGradientId;
        layer.m_gradientSampler.m_opacity = 1.0f;
        config.m_layers.push_back(layer);

        layer.m_operation = GradientSignal::MixedGradientLayer::MixingOperation::Overlay;
        layer.m_gradientSampler.m_gradientId = mixedGradientId;
        layer.m_gradientSampler.m_opacity = 0.75f;
        config.m_layers.push_back(layer);

        entity->CreateComponent<GradientSignal::MixedGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestPosterizeGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Posterize Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::PosterizeGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_mode = GradientSignal::PosterizeGradientConfig::ModeType::Ps;
        config.m_bands = 5;
        entity->CreateComponent<GradientSignal::PosterizeGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestReferenceGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Reference Gradient Component.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ReferenceGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_gradientSampler.m_ownerEntityId = entity->GetId();
        entity->CreateComponent<GradientSignal::ReferenceGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSmoothStepGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Smooth Step Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::SmoothStepGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_smoothStep.m_falloffMidpoint = 0.75f;
        config.m_smoothStep.m_falloffRange = 0.125f;
        config.m_smoothStep.m_falloffStrength = 0.25f;
        entity->CreateComponent<GradientSignal::SmoothStepGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestThresholdGradient(
        float shapeHalfBounds, const AZ::EntityId& inputGradientId)
    {
        // Create a Threshold Gradient Component with arbitrary parameters.
        auto entity = CreateTestEntity(shapeHalfBounds);
        GradientSignal::ThresholdGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_threshold = 0.75f;
        entity->CreateComponent<GradientSignal::ThresholdGradientComponent>(config);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceAltitudeGradient(float shapeHalfBounds)
    {
        // Create a Surface Altitude Gradient Component with arbitrary parameters.
        auto entity = CreateTestSphereEntity(shapeHalfBounds);
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_altitudeMin = -5.0f;
        config.m_altitudeMax = 15.0f + (shapeHalfBounds * 2.0f);
        entity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(config);

        // Create a SurfaceDataShape component to provide surface points from this component.
        SurfaceData::SurfaceDataShapeConfig shapeConfig;
        shapeConfig.m_providerTags.emplace_back("test_mask");
        auto surfaceShapeComponent = entity->CreateComponent(azrtti_typeid<SurfaceData::SurfaceDataShapeComponent>());
        surfaceShapeComponent->SetConfiguration(shapeConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceMaskGradient(float shapeHalfBounds)
    {
        // Create a Surface Mask Gradient Component with arbitrary parameters.
        auto entity = CreateTestSphereEntity(shapeHalfBounds);
        GradientSignal::SurfaceMaskGradientConfig config;
        config.m_surfaceTagList.push_back(AZ_CRC_CE("test_mask"));
        entity->CreateComponent<GradientSignal::SurfaceMaskGradientComponent>(config);

        // Create a SurfaceDataShape component to provide surface points from this component.
        SurfaceData::SurfaceDataShapeConfig shapeConfig;
        shapeConfig.m_providerTags.emplace_back("test_mask");
        auto surfaceShapeComponent = entity->CreateComponent(azrtti_typeid<SurfaceData::SurfaceDataShapeComponent>());
        surfaceShapeComponent->SetConfiguration(shapeConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::BuildTestSurfaceSlopeGradient(float shapeHalfBounds)
    {
        // Create a Surface Slope Gradient Component with arbitrary parameters.
        auto entity = CreateTestSphereEntity(shapeHalfBounds);
        GradientSignal::SurfaceSlopeGradientConfig config;
        config.m_slopeMin = 5.0f;
        config.m_slopeMax = 50.0f;
        config.m_rampType = GradientSignal::SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP;
        config.m_smoothStep.m_falloffMidpoint = 0.75f;
        config.m_smoothStep.m_falloffRange = 0.125f;
        config.m_smoothStep.m_falloffStrength = 0.25f;
        entity->CreateComponent<GradientSignal::SurfaceSlopeGradientComponent>(config);

        // Create a SurfaceDataShape component to provide surface points from this component.
        SurfaceData::SurfaceDataShapeConfig shapeConfig;
        shapeConfig.m_providerTags.emplace_back("test_mask");
        auto surfaceShapeComponent = entity->CreateComponent(azrtti_typeid<SurfaceData::SurfaceDataShapeComponent>());
        surfaceShapeComponent->SetConfiguration(shapeConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    void GradientSignalTest::SetUp()
    {
        SetupCoreSystems();
    }

    void GradientSignalTest::TearDown()
    {
        TearDownCoreSystems();
    }

    void GradientSignalTest::TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId)
    {
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientEntityId;

        TestFixedDataSampler(expectedOutput, size, gradientSampler);
    }

    void GradientSignalTest::TestFixedDataSampler(
        const AZStd::vector<float>& expectedOutput, int size, GradientSignal::GradientSampler& gradientSampler)
    {
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f);

                const int index = y * size + x;
                float actualValue = gradientSampler.GetValue(params);
                float expectedValue = expectedOutput[index];

                EXPECT_NEAR(actualValue, expectedValue, 0.01f);
            }
        }
    }

}

