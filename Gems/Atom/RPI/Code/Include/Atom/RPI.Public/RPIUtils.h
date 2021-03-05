/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

// RPIUtils is for dumping common functionality that is used in several places across the RPI
 
#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace AZ
{
    namespace RPI
    {
        class Shader;

        //! Get the asset ID for a given shader file path
        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath);

        //! Finds a shader asset for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Asset<ShaderAsset> FindShaderAsset(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "");

        //! Finds a shader asset for the given shader file path
        Data::Asset<ShaderAsset> FindShaderAsset(const AZStd::string& shaderFilePath);

        //! Loads a shader for the given shader asset ID. Optional shaderFilePath param for debugging.
        Data::Instance<Shader> LoadShader(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath = "");

        //! Loads a shader for the given shader file path
        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath);

        //! Loads a streaming image asset for the given file path
        Data::Instance<RPI::StreamingImage> LoadStreamingTexture(AZStd::string_view path);
    }   // namespace RPI
}   // namespace AZ

