/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>

namespace AZ
{
    namespace RPI
    {
        const char* ToString(ShaderStageType shaderStageType)
        {
            switch (shaderStageType)
            {
            case ShaderStageType::Vertex: return "Vertex";
            case ShaderStageType::Geometry: return "Geometry";
            case ShaderStageType::TessellationControl: return "TessellationControl";
            case ShaderStageType::TessellationEvaluation: return "TessellationEvaluation";
            case ShaderStageType::Fragment: return "Fragment";
            case ShaderStageType::Compute: return "Compute";
            case ShaderStageType::RayTracing: return "RayTracing";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        void ReflectShaderStageType(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Enum<ShaderStageType>()
                    ->Value(ToString(ShaderStageType::Vertex), ShaderStageType::Vertex)
                    ->Value(ToString(ShaderStageType::Geometry), ShaderStageType::Geometry)
                    ->Value(ToString(ShaderStageType::TessellationControl), ShaderStageType::TessellationControl)
                    ->Value(ToString(ShaderStageType::TessellationEvaluation), ShaderStageType::TessellationEvaluation)
                    ->Value(ToString(ShaderStageType::Fragment), ShaderStageType::Fragment)
                    ->Value(ToString(ShaderStageType::Compute), ShaderStageType::Compute)
                    ->Value(ToString(ShaderStageType::RayTracing), ShaderStageType::RayTracing)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
