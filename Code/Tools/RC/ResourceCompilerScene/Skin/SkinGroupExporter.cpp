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
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Skin/SkinExportContexts.h>
#include <RC/ResourceCompilerScene/Skin/SkinGroupExporter.h>
#include <RC/ResourceCompilerScene/Skin/SkinUtils.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainer = AZ::SceneAPI::Containers;

        const AZStd::string SkinGroupExporter::s_fileExtension = "skin";

        SkinGroupExporter::SkinGroupExporter(IAssetWriter* writer, IConvertContext* convertContext)
            : m_assetWriter(writer)
            , m_convertContext(convertContext)
        {
            BindToCall(&SkinGroupExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult SkinGroupExporter::ProcessContext(SkinGroupExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(), context.m_outputDirectory, s_fileExtension);
            AZ_TraceContext("Skin filename", filename);
            if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Invalid file name for skin");
                return SceneEvents::ProcessingResult::Failure;
            }

            SceneEvents::ProcessingResultCombiner result;

            CContentCGF cgfContent(filename.c_str());
            ConfigureSkinContent(cgfContent);
            // Process mesh
            // For each selected mesh, find its skinned skeleton's root bone. Make sure the root bone is consistent through all selected skin meshes.
            const SceneContainer::SceneGraph& graph = context.m_scene.GetGraph();
            AZStd::vector<AZStd::string> targetNodes = SceneUtil::SceneGraphSelector::GenerateTargetNodes(graph,
                context.m_group.GetSceneNodeSelectionList(), SceneUtil::SceneGraphSelector::IsMesh);
            result += ProcessSkins(context, cgfContent, targetNodes);
            
            if (m_assetWriter)
            {
                if (m_assetWriter->WriteSKIN(&cgfContent, m_convertContext, true))
                {
                    static const AZ::Data::AssetType skinnedMeshAssetType("{C5D443E1-41FF-4263-8654-9438BC888CB7}"); // from MeshAsset.h
                    context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), skinnedMeshAssetType, 0, AZStd::nullopt);
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Writing Skin has failed.");
                    result += SceneEvents::ProcessingResult::Failure;
                }
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "No asset writer found. Unable to write skin to disk");
                result += SceneEvents::ProcessingResult::Failure;
            }
            return result.GetResult();
        }
    } // namespace RC
} // namespace AZ
