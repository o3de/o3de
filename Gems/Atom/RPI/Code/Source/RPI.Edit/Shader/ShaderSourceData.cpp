/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RHI.Edit/Utils.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderSourceData>()
                    ->Version(6) // Introduction of "AddBuildArguments" & "RemoveBuildArguments"
                    ->Field("Source", &ShaderSourceData::m_source)
                    ->Field("DrawList", &ShaderSourceData::m_drawListName)
                    ->Field("DepthStencilState", &ShaderSourceData::m_depthStencilState)
                    ->Field("RasterState", &ShaderSourceData::m_rasterState)
                    ->Field("BlendState", &ShaderSourceData::m_blendState)
                    ->Field("ProgramSettings", &ShaderSourceData::m_programSettings)
                    ->Field("RemoveBuildArguments", &ShaderSourceData::m_removeBuildArguments)
                    ->Field("AddBuildArguments", &ShaderSourceData::m_addBuildArguments)
                    ->Field("Definitions", &ShaderSourceData::m_definitions)
                    ->Field("DisabledRHIBackends", &ShaderSourceData::m_disabledRhiBackends)
                    ->Field("Supervariants", &ShaderSourceData::m_supervariants)
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

                serializeContext->Class<ShaderSourceData::SupervariantInfo>()
                    ->Version(2) // Introduction of "AddBuildArguments" & "RemoveBuildArguments"
                    ->Field("Name", &SupervariantInfo::m_name)
                    ->Field("RemoveBuildArguments", &SupervariantInfo::m_removeBuildArguments)
                    ->Field("AddBuildArguments", &SupervariantInfo::m_addBuildArguments)
                    ->Field("Definitions", &SupervariantInfo::m_definitions)
                    ;
            }
        }

        bool ShaderSourceData::IsRhiBackendDisabled(const AZ::Name& rhiName) const
        {
            return AZStd::any_of(AZ_BEGIN_END(m_disabledRhiBackends), [&](const AZStd::string& currentRhiName)
                {
                    return currentRhiName == rhiName.GetStringView();
                });
        }

    } // namespace RPI
} // namespace AZ
