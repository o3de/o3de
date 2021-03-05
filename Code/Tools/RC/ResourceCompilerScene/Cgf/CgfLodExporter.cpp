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
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfLodExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;

        const AZStd::string CgfLodExporter::s_fileExtension = "cgf";

        CgfLodExporter::CgfLodExporter(IAssetWriter* writer)
            : m_assetWriter(writer)
        {
            BindToCall(&CgfLodExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult CgfLodExporter::ProcessContext(CgfGroupExportContext& context) const
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

            // Create the base CGF name
            AZStd::string baseCGFfilename = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(), context.m_outputDirectory, s_fileExtension);

            SceneEvents::ProcessingResultCombiner result;
            
            for (size_t index = 0; index < lodRule->GetLodCount(); ++index)
            {
                AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(
                    context.m_group.GetName() + "_LOD" + AZStd::to_string(static_cast<int>(index + 1)), context.m_outputDirectory, s_fileExtension);
                AZ_TraceContext("CGF Lod File Name", filename);
                if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to write CGF Lod file. Filename is empty or target folder does not exist.");
                    result += SceneEvents::ProcessingResult::Failure;
                    break;
                }

                CContentCGF cgfContent(filename.c_str());
                ConfigureCgfContent(cgfContent);

                const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
                AZStd::vector<AZStd::string> targetNodes = SceneUtil::SceneGraphSelector::GenerateTargetNodes(graph,
                    lodRule->GetSceneNodeSelectionList(index), SceneUtil::SceneGraphSelector::IsMesh);

                result += ProcessMeshes(context, cgfContent, targetNodes);
                if (cgfContent.GetNodeCount() == 0)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Empty LoD Detected at level %d.", index);
                    result += SceneEvents::ProcessingResult::Failure;
                    break;
                }

                if (m_assetWriter)
                {
                    if (m_assetWriter->WriteCGF(&cgfContent))
                    {
                        static const AZ::Data::AssetType staticMeshLodsAssetType("{9AAE4926-CB6A-4C60-9948-A1A22F51DB23}");
                        // Using the same guid as the parent group/cgf as this needs to be a lod of that cgf.
                        // Setting the lod to index+1 as 0 means the base mesh and 1-6 are lod levels 0-5.
                        AZ::SceneAPI::Events::ExportProduct& lodProduct = context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), staticMeshLodsAssetType, index + 1, AZStd::nullopt);

                        // Add this LOD as a dependency to the base CGF
                        context.m_products.AddDependencyToProduct(baseCGFfilename, lodProduct);
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to write CGF LoD file at level %d.", index);
                        result += SceneEvents::ProcessingResult::Failure;
                        break;
                    }
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "No asset writer found. Unable to write cgf to disk");
                    result += SceneEvents::ProcessingResult::Failure;
                    break;
                }
            }

            return result.GetResult();
        }
    } // namespace RC
} // namespace AZ
