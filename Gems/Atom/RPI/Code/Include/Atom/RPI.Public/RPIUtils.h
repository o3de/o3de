/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// RPIUtils is for dumping common functionality that is used in several places across the RPI
 
#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI/DispatchItem.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/optional.h>

namespace AZ
{
    namespace RPI
    {
        class Shader;

        // If the RPI system using null renderer
        bool IsNullRenderer();

        //! Get the asset ID for a given shader file path
        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath, bool isCritical = false);

        //! Finds a shader asset for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Asset<ShaderAsset> FindShaderAsset(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "");

        //! Finds a shader asset for the given shader file path
        Data::Asset<ShaderAsset> FindShaderAsset(const AZStd::string& shaderFilePath);
        Data::Asset<ShaderAsset> FindCriticalShaderAsset(const AZStd::string& shaderFilePath);

        //! Loads a shader for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Instance<Shader> LoadShader(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "", const AZStd::string& supervariantName = "");

        //! Loads a shader for the given shader file path
        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath, const AZStd::string& supervariantName = "");
        Data::Instance<Shader> LoadCriticalShader(const AZStd::string& shaderFilePath, const AZStd::string& supervariantName = "");

        //! Loads a streaming image asset for the given file path
        Data::Instance<RPI::StreamingImage> LoadStreamingTexture(AZStd::string_view path);

        //! Looks for a three arguments attribute named @attributeName in the given shader asset.
        //! Assigns the value to each non-null output variables.
        //! @param shaderAsset
        //! @param attributeName
        //! @param numThreadsX Can be NULL. If not NULL it takes the value of the 1st argument of the attribute. Becomes 1 on error.
        //! @param numThreadsY Can be NULL. If not NULL it takes the value of the 2nd argument of the attribute. Becomes 1 on error.
        //! @param numThreadsZ Can be NULL. If not NULL it takes the value of the 3rd argument of the attribute. Becomes 1 on error.
        //! @returns An Outcome instance with error message in case of error.
        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& attributeName, uint16_t* numThreadsX, uint16_t* numThreadsY, uint16_t* numThreadsZ);

        //! Same as above, but assumes the name of the attribute to be 'numthreads'.
        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, uint16_t* numThreadsX, uint16_t* numThreadsY, uint16_t* numThreadsZ);

        //! Same as above. Provided as a convenience when all arguments of the 'numthreads' attributes should be assigned to RHI::DispatchDirect::m_threadsPerGroup* variables.
        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, RHI::DispatchDirect& dispatchDirect);

        //! Returns true/false if the specified format is supported by the image data pixel retrieval APIs
        //!     GetImageDataPixelValue, GetSubImagePixelValue, and GetSubImagePixelValues
        bool IsImageDataPixelAPISupported(AZ::RHI::Format format);

        //! Get single image pixel value from raw image data
        //! This assumes the imageData is not empty
        template<typename T>
        T GetImageDataPixelValue(AZStd::span<const uint8_t> imageData, const AZ::RHI::ImageDescriptor& imageDescriptor,
            uint32_t x, uint32_t y, uint32_t componentIndex = 0);

        //! Get single image pixel value for specified mip and slice
        template<typename T>
        T GetSubImagePixelValue(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset, uint32_t x, uint32_t y,
            uint32_t componentIndex = 0, uint32_t mip = 0, uint32_t slice = 0);

        //! Process a region of image pixel values (float) for specified mip and slice
        //! The visitor function `callback` is invoked on each pixel specified by the region
        //! NOTE: The topLeft coordinate is inclusive, whereas the bottomRight is exclusive
        bool GetSubImagePixelValues(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32&y, const float& value)> callback,
            uint32_t componentIndex = 0, uint32_t mip = 0, uint32_t slice = 0);

        //! Process a region of image pixel values (uint) for specified mip and slice
        //! The visitor function `callback` is invoked on each pixel specified by the region
        //! NOTE: The topLeft coordinate is inclusive, whereas the bottomRight is exclusive
        bool GetSubImagePixelValuesUint(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32& y, const AZ::u32& value)> callback,
            uint32_t componentIndex = 0, uint32_t mip = 0, uint32_t slice = 0);

        //! Process a region of image pixel values (int) for specified mip and slice
        //! The visitor function `callback` is invoked on each pixel specified by the region
        //! NOTE: The topLeft coordinate is inclusive, whereas the bottomRight is exclusive
        bool GetSubImagePixelValuesInt(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32& y, const AZ::s32& value)> callback,
            uint32_t componentIndex = 0, uint32_t mip = 0, uint32_t slice = 0);

        //! Loads a render pipeline asset and returns its descriptor.
        //! @param pipelineAssetPath Path to the render pipeline asset.
        //! @param nameSuffix Optional parameter to specify a suffix for the name of the pipeline returned.
        //! @returns An optional with a render pipeline descriptor if there was not errors.
        AZStd::optional<RenderPipelineDescriptor> GetRenderPipelineDescriptorFromAsset(const AZStd::string& pipelineAssetPath, AZStd::string_view nameSuffix = "");

    }   // namespace RPI
}   // namespace AZ

