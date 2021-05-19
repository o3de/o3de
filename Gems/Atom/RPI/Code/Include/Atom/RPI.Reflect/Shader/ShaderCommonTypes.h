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

#include <AzCore/RTTI/TypeInfo.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ
{
    namespace RPI
    {
        // Common bit positions for ShaderAsset2 and ShaderVariantAsset2 product SubIds.
        static constexpr uint32_t RhiIndexBitPosition = 30;
        static constexpr uint32_t RhiIndexNumBits = 32 - RhiIndexBitPosition;
        static constexpr uint32_t RhiIndexMaxValue = (1 << RhiIndexNumBits) - 1;

        static constexpr uint32_t SupervariantIndexBitPosition = 22;
        static constexpr uint32_t SupervariantIndexNumBits = RhiIndexBitPosition - SupervariantIndexBitPosition;
        static constexpr uint32_t SupervariantIndexMaxValue = (1 << SupervariantIndexNumBits) - 1;

        //! A wrapper around a supervariant index for type conformity.
        //! A supervariant index is required to find shader data from
        //! Shader2 and ShaderAsset2 related APIs.
        using SupervariantIndex = RHI::Handle<uint32_t, class ShaderAsset2>;
        static const SupervariantIndex DefaultSupervariantIndex(0);
        static const SupervariantIndex InvalidSupervariantIndex(aznumeric_cast<uint32_t>(-1));

        enum class ShaderStageType : uint32_t
        {
            Vertex,
            Geometry,
            TessellationControl,
            TessellationEvaluation,
            Fragment,
            Compute,
            RayTracing
        };

        const char* ToString(ShaderStageType shaderStageType);

        void ReflectShaderStageType(ReflectContext* context);

    } // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::ShaderStageType, "{A6408508-748B-4963-B618-E1E6ECA3629A}");
    
} // namespace AZ
