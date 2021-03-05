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
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfGroupExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
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

        const AZStd::string CgfGroupExporter::s_fileExtension = "cgf";

        CgfGroupExporter::CgfGroupExporter(IAssetWriter* writer)
            : m_assetWriter(writer)
        {
            BindToCall(&CgfGroupExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult CgfGroupExporter::ProcessContext(CgfGroupExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(), context.m_outputDirectory, s_fileExtension);
            AZ_TraceContext("CGF File Name", filename);
            if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to write CGF file. Filename is empty or target folder does not exist.");
                return SceneEvents::ProcessingResult::Failure;
            }

            SceneEvents::ProcessingResultCombiner result;
            
            CContentCGF cgfContent(filename.c_str());
            ConfigureCgfContent(cgfContent);
            
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            AZStd::vector<AZStd::string> targetNodes = SceneUtil::SceneGraphSelector::GenerateTargetNodes(graph,
                context.m_group.GetSceneNodeSelectionList(), SceneUtil::SceneGraphSelector::IsMesh);
            
            result += ProcessMeshes(context, cgfContent, targetNodes);
            
            if (m_assetWriter && cgfContent.GetNodeCount() > 0)
            {
                if (m_assetWriter->WriteCGF(&cgfContent))
                {
                    static const Data::AssetType staticMeshAssetType("{C2869E3B-DDA0-4E01-8FE3-6770D788866B}"); // from MeshAsset.h
                    AZ::SceneAPI::Events::ExportProduct& exportProduct = context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), staticMeshAssetType, 0, AZStd::nullopt);

                    // If the mesh group has a material rule, then add the material path dependency
                    if (context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IMaterialRule>())
                    {
                        // All CGFs are assumed to have a single material with their same name in their same folder.
                        // Add just the material file name as a path dependency for now.
                        // Note that at this point, the .mtl file may or may not exist, which is fine.
                        AZStd::string materialName;
                        AzFramework::StringFunc::Path::GetFullFileName(context.m_scene.GetSourceFilename().c_str(), materialName);
                        AzFramework::StringFunc::Path::ReplaceExtension(materialName, ".mtl");
                        exportProduct.m_legacyPathDependencies.push_back(materialName);
                    }
                }
                else
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to write CGF file.");
                    result += SceneEvents::ProcessingResult::Failure;
                }
            }
            else
            {
                if (!m_assetWriter)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "No asset writer found. Unable to write cgf to disk");
                }   
                if (cgfContent.GetNodeCount() == 0 )
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Empty Cgf file. Cgf not written to disk." );
                }   
                result += SceneEvents::ProcessingResult::Failure;
            }
            return result.GetResult();
        }
    } // namespace RC
} // namespace AZ
