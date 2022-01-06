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

namespace AZ
{
    namespace RPI
    {
        class Shader;

        //! Get the asset ID for a given shader file path
        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath, bool isCritical = false);

        //! Finds a shader asset for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Asset<ShaderAsset> FindShaderAsset(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "");

        //! Finds a shader asset for the given shader file path
        Data::Asset<ShaderAsset> FindShaderAsset(const AZStd::string& shaderFilePath);
        Data::Asset<ShaderAsset> FindCriticalShaderAsset(const AZStd::string& shaderFilePath);

        //! Loads a shader for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Instance<Shader> LoadShader(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "");

        //! Loads a shader for the given shader file path
        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath);
        Data::Instance<Shader> LoadCriticalShader(const AZStd::string& shaderFilePath);

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
        
    }   // namespace RPI
}   // namespace AZ

