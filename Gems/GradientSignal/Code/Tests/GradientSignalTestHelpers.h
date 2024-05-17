/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzTest/AzTest.h>

#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace UnitTest
{
    //! Helper method to build a AZ::RHI::ImageSubresourceLayout
    //! @param width The width of the image
    //! @param height The height of the image
    //! @param pixelSize Number of bytes per pixel
    //! @return The AZ::RHI::ImageSubresourceLayout that has been filled out appropriately
    AZ::RHI::DeviceImageSubresourceLayout BuildSubImageLayout(AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize);

    //! Build a deterministic random set of image pixel data
    //! @param width Width of the image
    //! @param height Height of the image
    //! @param pixelSize Number of bytes per pixel
    //! @param seed The random seed for generating the data
    //! @return A vector of bytes for the image data
    AZStd::vector<uint8_t> BuildBasicImageData(AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZ::s32 seed);

    //! Build a mip chain asset that contains the basic image data from BuildBasicImageData
    //! @param mipLevels Number of mip levels in the chain
    //! @param width The width of the image
    //! @param height The height of the image
    //! @param pixelSize The number of bytes per pixel
    //! @param data The raw pixel data
    //! @return A mip chain asset with the specified basic image data
    AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> BuildBasicMipChainAsset(
        AZ::u16 mipLevels, AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZStd::span<const uint8_t> data);

    //! Construct an array of image data where all the pixels are 0 except for one at the given coordinate
    //! @param width Width of the image
    //! @param height Height of the image
    //! @param pixelSize Number of bytes per pixel
    //! @param pixelX The X coordinate of the pixel to set to a specific value
    //! @param pixelY The Y coordinate of the pixel to set to a specific value
    //! @param setPixelValues The values to set the one specific pixel to (one value per pixel channel, determined by pixelSize)
    //! @return A vector of bytes for the image data
    AZStd::vector<uint8_t> BuildSpecificPixelImageData(
        AZ::u32 width, AZ::u32 height, AZ::u32 pixelSize, AZ::u32 pixelX, AZ::u32 pixelY, AZStd::span<const AZ::u8> setPixelValues);

    //! Given a set of raw pixel data, create an AZ::RPI::StreamingImageAsset
    //! @param width The width of the image
    //! @param height The height of the image
    //! @param format The format of the pixel data
    //! @param data The raw pixel data
    //! \return The AZ::RPI::StreamingImageAsset in a loaded ready state
    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateImageAssetFromPixelData(
        AZ::u32 width, AZ::u32 height, AZ::RHI::Format format, AZStd::span<const uint8_t> data);

    //! Creates a deterministically random set of pixel data as an AZ::RPI::StreamingImageAsset.
    //! \param width The width of the AZ::RPI::StreamingImageAsset
    //! \param height The height of the AZ::RPI::StreamingImageAsset
    //! \param seed The random seed to use for generating the random data
    //! \return The AZ::RPI::StreamingImageAsset in a loaded ready state
    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateImageAsset(AZ::u32 width, AZ::u32 height, AZ::s32 seed);

    //! Creates an AZ::RPI::StreamingImageAsset where all the pixels are 0 except for the one pixel at the given coordinates, which is set to a specific value based on the component.
    //! \param width The width of the AZ::RPI::StreamingImageAsset
    //! \param height The height of the AZ::RPI::StreamingImageAsset
    //! \param pixelX The X coordinate of the pixel to set to a specific value
    //! \param pixelY The Y coordinate of the pixel to set to a specific value
    //! @param setPixelValues The values to set the one specific pixel to (one value per pixel channel, determined by pixelSize)
    //! \return The AZ::RPI::StreamingImageAsset in a loaded ready state
    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> CreateSpecificPixelImageAsset(
        AZ::u32 width, AZ::u32 height, AZ::u32 pixelX, AZ::u32 pixelY, AZStd::span<const AZ::u8> setPixelValues);

    //! Converts a set of pixel coordinates in an image to a world space value that represents the center of the pixel.
    //! \param pixelX The X coordinate of the pixel to convert.
    //! \param pixelY The Y coordinate of the pixel to convert.
    //! \param bounds The world space bounds of the image.
    //! \param width The width of the image in pixels.
    //! \param height The height of the image in pixels.
    //! @return The world space center of the requested pixel.
    AZ::Vector3 PixelCoordinatesToWorldSpace(uint32_t pixelX, uint32_t pixelY, const AZ::Aabb& bounds, uint32_t width, uint32_t height);

    class GradientSignalTestHelpers
    {
    public:
        static void CompareGetValueAndGetValues(AZ::EntityId gradientEntityId, float queryMin, float queryMax);

#ifdef HAVE_BENCHMARK
        // We use an enum to list out the different types of GetValue() benchmarks to run so that way we can condense our test cases
        // to just take the value in as a benchmark argument and switch on it. Otherwise, we would need to write a different benchmark
        // function for each test case for each gradient.
        enum GetValuePermutation : int64_t
        {
            EBUS_GET_VALUE,
            EBUS_GET_VALUES,
            SAMPLER_GET_VALUE,
            SAMPLER_GET_VALUES,
        };

        static void FillQueryPositions(AZStd::vector<AZ::Vector3>& positions, float height, float width);

        static void RunEBusGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunEBusGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunSamplerGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunSamplerGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunGetValueOrGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId);

// Because there's no good way to label different enums in the output results (they just appear as integer values), we work around it by
// registering one set of benchmark runs for each enum value and use ArgNames() to give it a friendly name in the results.
#ifndef GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F
#define GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(Fixture, Func)                                                                    \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUE, 1024 })                                                  \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUE, 2048 })                                                  \
        ->ArgNames({ "EbusGetValue", "size" })                                                                                            \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUES, 1024 })                                                 \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUES, 2048 })                                                 \
        ->ArgNames({ "EbusGetValues", "size" })                                                                                           \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUE, 1024 })                                               \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUE, 2048 })                                               \
        ->ArgNames({ "SamplerGetValue", "size" })                                                                                         \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUES, 1024 })                                              \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUES, 2048 })                                              \
        ->ArgNames({ "SamplerGetValues", "size" })                                                                                        \
        ->Unit(::benchmark::kMillisecond);
#endif

#endif
    };


}
