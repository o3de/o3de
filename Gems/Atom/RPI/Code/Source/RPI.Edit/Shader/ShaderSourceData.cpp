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

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>

namespace AZ
{
    namespace RPI
    {
        const char* ShaderSourceData::Extension = "shader";

        void ShaderSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderSourceData>()
                    ->Version(2)
                    ->Field("Source", &ShaderSourceData::m_source)
                    ->Field("DrawList", &ShaderSourceData::m_drawListName)
                    ->Field("DepthStencilState", &ShaderSourceData::m_depthStencilState)
                    ->Field("RasterState", &ShaderSourceData::m_rasterState)
                    ->Field("BlendState", &ShaderSourceData::m_blendState)
                    ->Field("ProgramSettings", &ShaderSourceData::m_programSettings)
                    ->Field("CompilerHints", &ShaderSourceData::m_compiler)
                    ->Field("ShaderVariantHints", &ShaderSourceData::m_shaderOptionGroupHints)
                    ;

                serializeContext->Class<ShaderSourceData::Variant>()
                    ->Version(1)
                    ->Field("Options", &Variant::m_options)
                    ;

                serializeContext->Class<ShaderSourceData::ProgramSettings>()
                    ->Version(1)
                    ->Field("EntryPoints", &ProgramSettings::m_entryPoints)
                    ;

                serializeContext->Class<ShaderSourceData::EntryPoint>()
                    ->Version(1)
                    ->Field("Name", &EntryPoint::m_name)
                    ->Field("Type", &EntryPoint::m_type)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
