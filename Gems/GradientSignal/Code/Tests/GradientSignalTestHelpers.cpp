/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestHelpers.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <AzCore/Math/Aabb.h>
#include <GradientSignal/GradientSampler.h>

namespace UnitTest
{
    AZ::RHI::DeviceImageSubresourceLayout BuildSubImageLayout(AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize)
    {
        AZ::RHI::DeviceImageSubresourceLayout layout;
        layout.m_size = AZ::RHI::Size{ width, height, 1 };
        layout.m_rowCount = width;
        layout.m_bytesPerRow = width * pixelSize;
        layout.m_bytesPerImage = width * height * pixelSize;
        return layout;
    }

    AZStd::vector<uint8_t> BuildBasicImageData(AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZ::s32 seed)
    {
        const size_t imageSize = width * height * pixelSize;

        AZStd::vector<uint8_t> image;
        image.reserve(imageSize);

        size_t value = 0;
        AZStd::hash_combine(value, seed);

        for (AZ::u32 x = 0; x < width; ++x)
        {
            for (AZ::u32 y = 0; y < height; ++y)
            {
                AZStd::hash_combine(value, x);
                AZStd::hash_combine(value, y);
                image.push_back(static_cast<AZ::u8>(value));
            }
        }

        EXPECT_EQ(image.size(), imageSize);
        return image;
    }

    AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> BuildBasicMipChainAsset(
        AZ::u16 mipLevels, AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZStd::span<const uint8_t> data)
    {
        using namespace AZ;

        RPI::ImageMipChainAssetCreator assetCreator;

        const uint16_t arraySize = 1;
        assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), mipLevels, arraySize);

        RHI::DeviceImageSubresourceLayout layout = BuildSubImageLayout(width, height, pixelSize);

        assetCreator.BeginMip(layout);
        assetCreator.AddSubImage(data.data(), data.size());
        assetCreator.EndMip();

        Data::Asset<RPI::ImageMipChainAsset> asset;
        EXPECT_TRUE(assetCreator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);

        return asset;
    }

    AZStd::vector<uint8_t> BuildSpecificPixelImageData(
        AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZ::u32 pixelX, AZ::u32 pixelY, AZStd::span<const AZ::u8> setPixelValues)
    {
        AZ_Assert(setPixelValues.size() == pixelSize, "Wrong number of pixel channel values passed in");
        const size_t imageSize = width * height * pixelSize;

        AZStd::vector<uint8_t> image;
        image.reserve(imageSize);

        // Image data should be stored inverted on the y axis relative to our engine, so loop backwards through y.
        for (int y = static_cast<int>(height) - 1; y >= 0; --y)
        {
            for (AZ::u32 x = 0; x < width; ++x)
            {
                for (AZ::u32 component = 0; component < pixelSize; ++component)
                {
                    if ((x == static_cast<int>(pixelX)) && (y == static_cast<int>(pixelY)))
                    {
                        image.push_back(setPixelValues[component]);
                    }
                    else
                    {
                        image.push_back(0);
                    }
                }
            }
        }

        EXPECT_EQ(image.size(), imageSize);
        return image;
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateImageAssetFromPixelData(
        AZ::u32 width, AZ::u32 height, AZ::RHI::Format format, AZStd::span<const uint8_t> data)
    {
        auto randomAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
        auto imageAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::RPI::StreamingImageAsset>(
            randomAssetId, AZ::Data::AssetLoadBehavior::Default);

        const AZ::u32 mipCountTotal = 1;
        const AZ::u32 pixelSize = AZ::RHI::GetFormatComponentCount(format);

        AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> mipChain = BuildBasicMipChainAsset(mipCountTotal, width, height, pixelSize, data);

        AZ::RPI::StreamingImageAssetCreator assetCreator;
        assetCreator.Begin(randomAssetId);

        AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create2D(AZ::RHI::ImageBindFlags::ShaderRead, width, height, format);

        assetCreator.SetImageDescriptor(imageDesc);
        assetCreator.AddMipChainAsset(*mipChain.Get());

        EXPECT_TRUE(assetCreator.End(imageAsset));
        EXPECT_TRUE(imageAsset.IsReady());
        EXPECT_NE(imageAsset.Get(), nullptr);

        return imageAsset;
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateImageAsset(AZ::u32 width, AZ::u32 height, AZ::s32 seed)
    {
        const auto format = AZ::RHI::Format::R8_UNORM;
        const AZ::u32 pixelSize = AZ::RHI::GetFormatComponentCount(format);

        AZStd::vector<uint8_t> data = BuildBasicImageData(width, height, pixelSize, seed);
        return CreateImageAssetFromPixelData(width, height, format, data);
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateSpecificPixelImageAsset(
        AZ::u32 width, AZ::u32 height, AZ::u32 pixelX, AZ::u32 pixelY, AZStd::span<const AZ::u8> setPixelValues)
    {
        const auto format = AZ::RHI::Format::R8G8B8A8_UNORM;
        const AZ::u32 pixelSize = AZ::RHI::GetFormatComponentCount(format);

        AZStd::vector<uint8_t> data = BuildSpecificPixelImageData(width, height, pixelSize, pixelX, pixelY, setPixelValues);
        return CreateImageAssetFromPixelData(width, height, format, data);
    }

    AZ::Vector3 PixelCoordinatesToWorldSpace(uint32_t pixelX, uint32_t pixelY, const AZ::Aabb& bounds, uint32_t width, uint32_t height)
    {
        AZ::Vector2 pixelSize(bounds.GetXExtent() / aznumeric_cast<float>(width), bounds.GetYExtent() / aznumeric_cast<float>(height));

        // Return the center point of the pixel in world space.
        // Note that Y gets flipped because of the way images map into world space. (0,0) is the lower left corner in world space,
        // but the upper left corner in image space.
        return AZ::Vector3(
            bounds.GetMin().GetX() + ((pixelX + 0.5f) * pixelSize.GetX()),
            bounds.GetMin().GetY() + ((height - (pixelY + 0.5f)) * pixelSize.GetY()),
            0.0f);
    }

    void GradientSignalTestHelpers::CompareGetValueAndGetValues(AZ::EntityId gradientEntityId, float queryMin, float queryMax)
    {
        // Create a gradient sampler and run through a series of points to see if they match expectations.

        const AZ::Aabb queryRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(queryMin), AZ::Vector3(queryMax));
        const AZ::Vector2 stepSize(1.0f, 1.0f);

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientEntityId;

        const size_t numSamplesX = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetX() / stepSize.GetX()));
        const size_t numSamplesY = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetY() / stepSize.GetY()));

        // Build up the list of positions to query.
        AZStd::vector<AZ::Vector3> positions(numSamplesX * numSamplesY);
        size_t index = 0;
        for (size_t yIndex = 0; yIndex < numSamplesY; yIndex++)
        {
            float y = queryRegion.GetMin().GetY() + (stepSize.GetY() * yIndex);
            for (size_t xIndex = 0; xIndex < numSamplesX; xIndex++)
            {
                float x = queryRegion.GetMin().GetX() + (stepSize.GetX() * xIndex);
                positions[index++] = AZ::Vector3(x, y, 0.0f);
            }
        }

        // Get the results from GetValues
        AZStd::vector<float> results(numSamplesX * numSamplesY);
        gradientSampler.GetValues(positions, results);

        // For each position, call GetValue and verify that the values match.
        for (size_t positionIndex = 0; positionIndex < positions.size(); positionIndex++)
        {
            GradientSignal::GradientSampleParams params;
            params.m_position = positions[positionIndex];
            float value = gradientSampler.GetValue(params);

            // We use ASSERT_NEAR instead of EXPECT_NEAR because if one value doesn't match, they probably all won't, so there's no
            // reason to keep running and printing failures for every value.
            ASSERT_NEAR(value, results[positionIndex], 0.000001f);
        }
    }

#ifdef HAVE_BENCHMARK

    void GradientSignalTestHelpers::FillQueryPositions(AZStd::vector<AZ::Vector3>& positions, float height, float width)
    {
        size_t index = 0;
        for (float y = 0.0f; y < height; y += 1.0f)
        {
            for (float x = 0.0f; x < width; x += 1.0f)
            {
                positions[index++] = AZ::Vector3(x, y, 0.0f);
            }
        }
    }

    void GradientSignalTestHelpers::RunEBusGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

        GradientSignal::GradientSampleParams params;

        // Get the height and width ranges for querying from our benchmark parameters
        const float height = aznumeric_cast<float>(queryRange);
        const float width = aznumeric_cast<float>(queryRange);

        // Call GetValue() on the EBus for every height and width in our ranges.
        for ([[maybe_unused]] auto _ : state)
        {
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    float value = 0.0f;
                    params.m_position = AZ::Vector3(x, y, 0.0f);
                    GradientSignal::GradientRequestBus::EventResult(
                        value, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
                    benchmark::DoNotOptimize(value);
                }
            }
        }
    }

    void GradientSignalTestHelpers::RunEBusGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Get the height and width ranges for querying from our benchmark parameters
        float height = aznumeric_cast<float>(queryRange);
        float width = aznumeric_cast<float>(queryRange);
        int64_t totalQueryPoints = queryRange * queryRange;

        // Call GetValues() for every height and width in our ranges.
        for ([[maybe_unused]] auto _ : state)
        {
            // Set up our vector of query positions. This is done inside the benchmark timing since we're counting the work to create
            // each query position in the single GetValue() call benchmarks, and will make the timing more directly comparable.
            AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
            FillQueryPositions(positions, height, width);

            // Query and get the results.
            AZStd::vector<float> results(totalQueryPoints);
            GradientSignal::GradientRequestBus::Event(
                gradientId, &GradientSignal::GradientRequestBus::Events::GetValues, positions, results);
            benchmark::DoNotOptimize(results);
        }
    }

    void GradientSignalTestHelpers::RunSamplerGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Create a gradient sampler to use for querying our gradient.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientId;

        // Get the height and width ranges for querying from our benchmark parameters
        const float height = aznumeric_cast<float>(queryRange);
        const float width = aznumeric_cast<float>(queryRange);

        // Call GetValue() through the GradientSampler for every height and width in our ranges.
        for ([[maybe_unused]] auto _ : state)
        {
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    GradientSignal::GradientSampleParams params;
                    params.m_position = AZ::Vector3(x, y, 0.0f);
                    float value = gradientSampler.GetValue(params);
                    benchmark::DoNotOptimize(value);
                }
            }
        }
    }

    void GradientSignalTestHelpers::RunSamplerGetValuesBenchmark(
        benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Create a gradient sampler to use for querying our gradient.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientId;

        // Get the height and width ranges for querying from our benchmark parameters
        const float height = aznumeric_cast<float>(queryRange);
        const float width = aznumeric_cast<float>(queryRange);
        const int64_t totalQueryPoints = queryRange * queryRange;

        // Call GetValues() through the GradientSampler for every height and width in our ranges.
        for ([[maybe_unused]] auto _ : state)
        {
            // Set up our vector of query positions. This is done inside the benchmark timing since we're counting the work to create
            // each query position in the single GetValue() call benchmarks, and will make the timing more directly comparable.
            AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
            FillQueryPositions(positions, height, width);

            // Query and get the results.
            AZStd::vector<float> results(totalQueryPoints);
            gradientSampler.GetValues(positions, results);
            benchmark::DoNotOptimize(results);
        }
    }

    void GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId)
    {
        switch (state.range(0))
        {
        case GetValuePermutation::EBUS_GET_VALUE:
            RunEBusGetValueBenchmark(state, gradientId, state.range(1));
            break;
        case GetValuePermutation::EBUS_GET_VALUES:
            RunEBusGetValuesBenchmark(state, gradientId, state.range(1));
            break;
        case GetValuePermutation::SAMPLER_GET_VALUE:
            RunSamplerGetValueBenchmark(state, gradientId, state.range(1));
            break;
        case GetValuePermutation::SAMPLER_GET_VALUES:
            RunSamplerGetValuesBenchmark(state, gradientId, state.range(1));
            break;
        default:
            AZ_Assert(false, "Benchmark permutation type not supported.");
        }
    }
#endif
}


