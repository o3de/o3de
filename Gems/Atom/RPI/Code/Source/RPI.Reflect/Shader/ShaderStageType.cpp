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
