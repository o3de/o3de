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

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>
#include <RC/ResourceCompilerScene/Skin/SkinExportContexts.h>
#include <RC/ResourceCompilerScene/Skin/SkinLodExporter.h>
#include <RC/ResourceCompilerScene/Skin/SkinUtils.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainer = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        const AZStd::string SkinLodExporter::s_fileExtension = "skin";

        SkinLodExporter::SkinLodExporter(IAssetWriter* writer, IConvertContext* convertContext)
            : m_assetWriter(writer)
            , m_convertContext(convertContext)
        {
            BindToCall(&SkinLodExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult SkinLodExporter::ProcessContext(SkinGroupExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::shared_ptr<const SceneDataTypes::ILodRule> lodRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::ILodRule>();
            if (!lodRule)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            SceneEvents::ProcessingResultCombiner result;

            for (size_t index = 0; index < lodRule->GetLodCount(); ++index)
            {
                AZ_TraceContext("Skin lod level", static_cast<uint64_t>(index));

                AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(
                    context.m_group.GetName() + "_LOD" + AZStd::to_string(static_cast<int>(index + 1)), context.m_outputDirectory, s_fileExtension);

                AZ_TraceContext("Skin lod filename", filename);

                if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Invalid file name for skin");
                    result += SceneEvents::ProcessingResult::Failure;
                    break;
                }

                CContentCGF cgfContent(filename.c_str());
                ConfigureSkinContent(cgfContent);
                // Process mesh
                // For each selected mesh, find its skinned skeleton's root bone. Make sure the root bone is consistent through all selected skin meshes.
                const SceneContainer::SceneGraph& graph = context.m_scene.GetGraph();
                AZStd::vector<AZStd::string> targetNodes = SceneUtil::SceneGraphSelector::GenerateTargetNodes(graph,
                    lodRule->GetSceneNodeSelectionList(index), SceneUtil::SceneGraphSelector::IsMesh);
                result += ProcessSkins(context, cgfContent, targetNodes);

                if (m_assetWriter)
                {
                    if (m_assetWriter->WriteSKIN(&cgfContent, m_convertContext, false))
                    {
                        static const AZ::Data::AssetType skinnedMeshLodsAssetType("{58E5824F-C27B-46FD-AD48-865BA41B7A51}");
                        // Using the same guid as the parent group/cgf as this needs to be a lod of that cgf.
                        // Setting the lod to index+1 as 0 means the base mesh and 1-6 are lod levels 0-5.
                        context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), skinnedMeshLodsAssetType, index + 1, AZStd::nullopt);
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Writing Skin has failed.");
                        result += SceneEvents::ProcessingResult::Failure;
                        break;
                    }
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "No asset writer found. Unable to write skin to disk");
                    result += SceneEvents::ProcessingResult::Failure;
                    break;
                }
            }
            return result.GetResult();
        }
    } // namespace RC
} // namespace AZ
