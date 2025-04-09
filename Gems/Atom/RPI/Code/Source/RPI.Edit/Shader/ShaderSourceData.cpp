/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderSourceData::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderSourceData>()
                    ->Version(8) // Added m_keepTempFolder flag.
                    ->Field("Source", &ShaderSourceData::m_source)
                    ->Field("DrawList", &ShaderSourceData::m_drawListName)
                    ->Field("DepthStencilState", &ShaderSourceData::m_depthStencilState)
                    ->Field("RasterState", &ShaderSourceData::m_rasterState)
                    ->Field("BlendState", &ShaderSourceData::m_blendState)
                    ->Field("GlobalTargetBlendState", &ShaderSourceData::m_globalTargetBlendState)
                    ->Field("TargetBlendStates", &ShaderSourceData::m_targetBlendStates)
                    ->Field("ProgramSettings", &ShaderSourceData::m_programSettings)
                    ->Field("RemoveBuildArguments", &ShaderSourceData::m_removeBuildArguments)
                    ->Field("AddBuildArguments", &ShaderSourceData::m_addBuildArguments)
                    ->Field("Definitions", &ShaderSourceData::m_definitions)
                    ->Field("ShaderOptions", &ShaderSourceData::m_shaderOptionValues)
                    ->Field("DisabledRHIBackends", &ShaderSourceData::m_disabledRhiBackends)
                    ->Field("Supervariants", &ShaderSourceData::m_supervariants)
                    ->Field("KeepTempFolder", &ShaderSourceData::m_keepTempFolder)
                    ;

                serializeContext->Class<ProgramSettings>()
                    ->Version(1)
                    ->Field("EntryPoints", &ProgramSettings::m_entryPoints)
                    ;

                serializeContext->Class<EntryPoint>()
                    ->Version(1)
                    ->Field("Name", &EntryPoint::m_name)
                    ->Field("Type", &EntryPoint::m_type)
                    ;

                serializeContext->Class<SupervariantInfo>()
                    ->Version(2) // Introduction of "AddBuildArguments" & "RemoveBuildArguments"
                    ->Field("Name", &SupervariantInfo::m_name)
                    ->Field("RemoveBuildArguments", &SupervariantInfo::m_removeBuildArguments)
                    ->Field("AddBuildArguments", &SupervariantInfo::m_addBuildArguments)
                    ->Field("Definitions", &SupervariantInfo::m_definitions)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ShaderSourceData>("ShaderSourceData", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_source, "Source", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_drawListName, "Draw List", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_depthStencilState, "Depth Stencil State", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_rasterState, "Raster State", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_blendState, "Blend State", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_globalTargetBlendState, "Global Target Blend State", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_targetBlendStates, "Target Blend States", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_programSettings, "Program Settings", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_removeBuildArguments, "Remove Build Arguments", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_addBuildArguments, "Add Build Arguments", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_definitions, "Definitions", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_shaderOptionValues, "Shader Options", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_disabledRhiBackends, "Disabled RHI Backends", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_supervariants, "Super Variants", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderSourceData::m_keepTempFolder, "Keep Temp Folder", "Preserves the Temp folder for successful shader compilations.")
                        ;

                    editContext->Class<ProgramSettings>("ShaderSourceData::ProgramSettings", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ProgramSettings::m_entryPoints, "Entry Points", "")
                        ;

                    editContext->Class<EntryPoint>("ShaderSourceData::EntryPoint", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EntryPoint::m_name, "Name", "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EntryPoint::m_type, "Type", "")
                            ->EnumAttribute(ShaderStageType::Vertex, ToString(ShaderStageType::Vertex))
                            ->EnumAttribute(ShaderStageType::Geometry, ToString(ShaderStageType::Geometry))
                            ->EnumAttribute(ShaderStageType::TessellationControl, ToString(ShaderStageType::TessellationControl))
                            ->EnumAttribute(ShaderStageType::TessellationEvaluation, ToString(ShaderStageType::TessellationEvaluation))
                            ->EnumAttribute(ShaderStageType::Fragment, ToString(ShaderStageType::Fragment))
                            ->EnumAttribute(ShaderStageType::Compute, ToString(ShaderStageType::Compute))
                            ->EnumAttribute(ShaderStageType::RayTracing, ToString(ShaderStageType::RayTracing))
                        ;

                    editContext->Class<SupervariantInfo>("ShaderSourceData::SupervariantInfo", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SupervariantInfo::m_name, "Name", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SupervariantInfo::m_removeBuildArguments, "Remove Build Arguments", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SupervariantInfo::m_addBuildArguments, "Add Build Arguments", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SupervariantInfo::m_definitions, "Definitions", "")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                // declare EntryPoint before things that use it.
                behaviorContext->Class<ShaderSourceData::EntryPoint>("ShaderSourceData::EntryPoint")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const ShaderSourceData::EntryPoint&>()
                    ->Property("name", BehaviorValueProperty(&EntryPoint::m_name))
                    ->Property("type", BehaviorValueProperty(&EntryPoint::m_type))
                    ;
                
                // Declare SupervariantInfo (which uses EntryPoint) before things that use it.
                behaviorContext->Class<ShaderSourceData::SupervariantInfo>("ShaderSourceData::SupervariantInfo")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const ShaderSourceData::SupervariantInfo&>()
                    ->Property("name", BehaviorValueProperty(&SupervariantInfo::m_name))
                    ->Property("removeBuildArguments", BehaviorValueProperty(&SupervariantInfo::m_removeBuildArguments))
                    ->Property("addBuildArguments", BehaviorValueProperty(&SupervariantInfo::m_addBuildArguments))
                    ->Property("definitions", BehaviorValueProperty(&SupervariantInfo::m_definitions))
                    ;
                
                // ShaderSourceData uses SuperVariant, so SuperVarient is defined above, before it.
                behaviorContext->Class<ShaderSourceData>("ShaderSourceData")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const ShaderSourceData&>()
                    ->Property("source", BehaviorValueProperty(&ShaderSourceData::m_source))
                    ->Property("drawListName", BehaviorValueProperty(&ShaderSourceData::m_drawListName))
                    ->Property("depthStencilState", BehaviorValueProperty(&ShaderSourceData::m_depthStencilState))
                    ->Property("rasterState", BehaviorValueProperty(&ShaderSourceData::m_rasterState))
                    ->Property("blendState", BehaviorValueProperty(&ShaderSourceData::m_blendState))
                    ->Property("globalTargetBlendState", BehaviorValueProperty(&ShaderSourceData::m_globalTargetBlendState))
                    ->Property("targetBlendStates", BehaviorValueProperty(&ShaderSourceData::m_targetBlendStates))
                    ->Property("programSettings", BehaviorValueProperty(&ShaderSourceData::m_programSettings))
                    ->Property("removeBuildArguments", BehaviorValueProperty(&ShaderSourceData::m_removeBuildArguments))
                    ->Property("addBuildArguments", BehaviorValueProperty(&ShaderSourceData::m_addBuildArguments))
                    ->Property("definitions", BehaviorValueProperty(&ShaderSourceData::m_definitions))
                    ->Property("shaderOptions", BehaviorValueProperty(&ShaderSourceData::m_shaderOptionValues))
                    ->Property("disabledRhiBackends", BehaviorValueProperty(&ShaderSourceData::m_disabledRhiBackends))
                    ->Property("superVariants", BehaviorValueProperty(&ShaderSourceData::m_supervariants))
                    ->Property("keepTempFolder", BehaviorValueProperty(&ShaderSourceData::m_keepTempFolder))
                    ->Method("IsRhiBackendDisabled", &ShaderSourceData::IsRhiBackendDisabled)
                    ;

                behaviorContext->Class<ShaderSourceData::ProgramSettings>("ShaderSourceData::ProgramSettings")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const ShaderSourceData::ProgramSettings&>()
                    ->Property("entryPoints", BehaviorValueProperty(&ProgramSettings::m_entryPoints))
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
