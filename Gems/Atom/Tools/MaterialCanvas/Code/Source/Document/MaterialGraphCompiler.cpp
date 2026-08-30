/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Graph/GraphTemplateFileDataCacheRequestBus.h>
#include <AtomToolsFramework/Graph/GraphUtil.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/regex.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Document/MaterialGraphCompiler.h>
#include <GraphModel/Model/Connection.h>

namespace MaterialCanvas
{
    void MaterialGraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialGraphCompiler, AtomToolsFramework::GraphCompiler>()
                ->Version(0)
                ;
        }
    }

    MaterialGraphCompiler::MaterialGraphCompiler(const AZ::Crc32& toolId)
        : AtomToolsFramework::GraphCompiler(toolId)
    {
    }

    MaterialGraphCompiler::~MaterialGraphCompiler()
    {
    }

    AZStd::string MaterialGraphCompiler::GetGraphPath() const
    {
        if (const auto& graphPath = AtomToolsFramework::GraphCompiler::GetGraphPath(); graphPath.ends_with(".materialgraph"))
        {
            return graphPath;
        }

        return AZStd::string::format("%s/Assets/Materials/Generated/untitled.materialgraph", AZ::Utils::GetProjectPath().c_str());
    }

    bool MaterialGraphCompiler::ShouldReportGeneratedFileStatus(const AZStd::string& generatedFile) const
    {
        // The viewport watches the asset catalog and reapplies the generated material when its product is ready. Waiting synchronously on
        // the source .material job can strand the compiler in Processing when the Asset Processor supersedes or retriggers that job while
        // the material type and shader pipeline settle. Keep the file in m_generatedFiles for the viewport, but do not use it as a gate.
        return AtomToolsFramework::GraphCompiler::ShouldReportGeneratedFileStatus(generatedFile) &&
            !generatedFile.ends_with(".material");
    }

    bool MaterialGraphCompiler::CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath)
    {
        if (!AtomToolsFramework::GraphCompiler::CompileGraph(graph, graphName, graphPath))
        {
            return false;
        }

        if (IsCancelRequested())
        {
            return FinishCompile(State::Canceled);
        }

        m_includePaths.clear();
        m_classDefinitions.clear();
        m_functionDefinitions.clear();
        m_configIdsVisited.clear();
        m_slotValueTable.clear();
        m_templateNodeCount = 0;
        m_templatePathsForCurrentNode.clear();
        m_templateFileDataVecForCurrentNode.clear();
        m_instructionNodesForCurrentNode.clear();
        m_wroteAnyGeneratedFile = false;
        m_onlyMaterialPropertyValuesChanged = true;
        m_productionOutputStale = false;
        m_materialPropertyValues.clear();
        BuildSlotValueTable();
        BuildDependencyTables();

        // Traverse all graph nodes and slots searching for settings to generate files from templates
        for (const auto& currentNode : GetAllNodesInExecutionOrder())
        {
            if (IsCancelRequested())
            {
                return FinishCompile(State::Canceled);
            }

            // Search this node for any template path settings that describe files that need to be generated from the graph.
            BuildTemplatePathsForCurrentNode(currentNode);

            // If no template files were specified for this node then skip additional processing and continue to the next one.
            if (m_templatePathsForCurrentNode.empty())
            {
                continue;
            }

            // Attempt to load all of the template files referenced by this node. All of the template data will be tokenized into individual
            // lines and stored in a container so then multiple passes can be made on each file, substituting tokens and filling in
            // details provided by the graph. None of the files generated from this node will be saved until they have all been processed.
            // Template files for material types will be processed in their own pass Because they require special handling and need to be
            // saved before material file templates to not trigger asset processor dependency errors.
            if (!LoadTemplatesForCurrentNode())
            {
                return FinishCompile(State::Failed);
            }

            if (IsCancelRequested())
            {
                return FinishCompile(State::Canceled);
            }

            // Perform an initial pass over all template files, injecting include files, class definitions, function definitions, simple
            // things that don't require much processing.
            PreprocessTemplatesForCurrentNode();

            // The next phase injects shader code instructions assembled by traversing the graph from each of the input slots on the current
            // node. The O3DE_GENERATED_INSTRUCTIONS_BEGIN marker will be followed by a list of input slot names corresponding to required
            // variables in the shader. Instructions will only be generated for the current node and nodes connected to the specified
            // inputs. This will allow multiple O3DE_GENERATED_INSTRUCTIONS blocks with different inputs to be specified in multiple
            // locations across multiple files from a single graph.

            // This will also keep track of nodes with instructions and data that contribute to the final shader code. The list of
            // contributing nodes will be used to exclude unused material inputs from generated SRGs and material types.
            BuildInstructionsForCurrentNode(currentNode);
            if (IsCancelRequested())
            {
                return FinishCompile(State::Canceled);
            }

            // At this point, all of the instructions have been generated for all of the template files used by this node. We now also have
            // a complete list of all nodes that contributed instructions to the final shader code across all of the files. Now, we can
            // safely generate the material SRG and material type that only contain variables referenced in the shaders. Without tracking
            // this, all variables would be included in the SRG and material type. The shader compiler would eliminate unused variables from
            // the compiled shader code. The material type would fail to build if it referenced any of the eliminated variables.

            // Note: The MaterialSrg has been replaced with the MaterialParameters - struct, which is generated automatically from the
            // MaterialProperties with a shaderInput connection.
            // BuildMaterialSrgForCurrentNode();

            // Write the output sets this compile owes. Everything above this point is output set independent and was done once: the
            // template data has been preprocessed in place and the instructions are already spliced into it, so writing a second set is
            // another pass over the same buffers rather than another traversal of the graph.
            for (const OutputSet outputSet : GetOutputSetsForThisCompile())
            {
                if (!ExportOutputSetForCurrentNode(currentNode, outputSet))
                {
                    return FinishCompile(State::Failed);
                }
            }

            // Preview output that is no longer being produced is not merely unused. The Asset Processor goes on building it, so a graph
            // that was edited once with preview output on would keep paying for preview shaders long after it was turned off.
            DeleteStalePreviewOutputForCurrentNode();

            // Whether Apply has anything left to do. Recorded here, while this node's template paths are still in hand.
            RecordProductionOutputStaleness();

            // Increment the template node counter in case we encounter another template node and need to uniquely identify it.
            ++m_templateNodeCount;
        }

        // Only wait on the Asset Processor when this compile produced something the viewport cannot resolve on its own.
        //
        // Nothing written means no job will ever be queued, so the status wait would poll paths whose jobs are all from a previous
        // compile. A change confined to material property values is delivered to the viewport below and applied as overrides on the live
        // material instance, so the preview is already correct before the Asset Processor has started. In both cases the wait is pure
        // latency, and in the second it is also the thing that makes the preview hostage to the Asset Processor finishing at all.
        if (m_wroteAnyGeneratedFile && !m_onlyMaterialPropertyValuesChanged)
        {
            if (!ReportGeneratedFileStatus())
            {
                return FinishCompile(State::Failed);
            }
        }
        else
        {
            AZ_TracePrintf_IfTrue(
                "MaterialGraphCompiler",
                IsCompileLoggingEnabled(),
                "Skipping asset status wait: %s\n",
                m_wroteAnyGeneratedFile ? "only material property values changed" : "no generated file changed");
        }

        // Send the current property values whatever path this compile took. A listener has to reapply them after anything that recreates
        // the material instance, so it needs the full set even when this particular compile changed none of them.
        return FinishCompile(
            State::Complete,
            [this]()
            {
                MaterialGraphCompilerNotificationBus::Event(
                    m_toolId,
                    &MaterialGraphCompilerNotificationBus::Events::OnMaterialPropertyValuesChanged,
                    GetGraphPath(),
                    m_materialPropertyValues);
            });
    }

    void MaterialGraphCompiler::BuildSlotValueTable()
    {
        // Build a table of all values for every slot in the graph.
        m_slotValueTable.clear();
        for (const auto& currentNode : GetAllNodesInExecutionOrder())
        {
            for (const auto& currentSlotPair : currentNode->GetSlots())
            {
                const auto& currentSlot = currentSlotPair.second;
                m_slotValueTable[currentSlot] = currentSlot->GetValue();
            }

            // If this is a dynamic node with slot data type groups, we will search for the largest vector or other data type and convert
            // all of the values in the group to the same type.
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                for (const auto& slotDataTypeGroup : nodeConfig.m_slotDataTypeGroups)
                {
                    // The slot data group string is separated by vertical bars and can be treated like a regular expression to compare
                    // against slot names. The largest vector size is recorded for each slot group.
                    const AZStd::regex slotDataTypeGroupRegex(slotDataTypeGroup, AZStd::regex::flag_type::icase);

                    // Some nodes might specify a minimum vector size needed to up convert slot values for azsl snippet or output slot
                    // requirements. Search all slots in the data type group for the minimum vector size setting to update the default
                    // minimum vector size.
                    unsigned int vectorSize = 0;
                    AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                        nodeConfig,
                        [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                        {
                            if (AZStd::regex_match(slotConfig.m_name, slotDataTypeGroupRegex))
                            {
                                const AZStd::string vectorSizeStr =
                                    AtomToolsFramework::GetSettingValueByName(slotConfig.m_settings, "materialPropertyMinVectorSize");
                                if (!vectorSizeStr.empty())
                                {
                                    vectorSize = AZStd::max(vectorSize, static_cast<unsigned int>(AZStd::stoi(vectorSizeStr)));
                                }
                            }
                        });

                    for (const auto& currentSlotPair : currentNode->GetSlots())
                    {
                        const auto& currentSlot = currentSlotPair.second;
                        if (currentSlot->GetSlotDirection() == GraphModel::SlotDirection::Input &&
                            AZStd::regex_match(currentSlot->GetName(), slotDataTypeGroupRegex))
                        {
                            const auto& currentSlotValue = GetValueFromSlotOrConnection(currentSlot);
                            vectorSize = AZStd::max(vectorSize, GetVectorSize(currentSlotValue));
                        }
                    }

                    // Once all of the container sizes have been recorded for each slot data group, iterate over all of these slot values
                    // and upgrade entries in the map to the bigger type.
                    for (const auto& currentSlotPair : currentNode->GetSlots())
                    {
                        const auto& currentSlot = currentSlotPair.second;
                        if (AZStd::regex_match(currentSlot->GetName(), slotDataTypeGroupRegex))
                        {
                            const auto& currentSlotValue = GetValueFromSlot(currentSlot);
                            m_slotValueTable[currentSlot] = ConvertToVector(currentSlotValue, vectorSize);
                        }
                    }
                }
            }
        }
    }

    void MaterialGraphCompiler::BuildDependencyTables()
    {
        if (!m_graph)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid graph object.");
            return;
        }

        // Collect the nodes that generate files, so that only nodes able to reach one of them contribute to the tables below.
        //
        // Include paths, class definitions and function definitions are injected verbatim into the generated shader code, and gathering
        // them from every node in the graph had two costs. The visible one was spurious rebuilds: dropping an unconnected Mix or Map
        // Range node onto the canvas changed the generated files and triggered a full compile, while an unconnected Material Input node
        // did not, because material inputs contribute properties rather than code. That asymmetry is what made the behaviour look
        // arbitrary. The quieter one is compile time, since anything injected is parsed by azslc and dxc whether or not it is reachable.
        //
        // This restores the invariant ShouldUseInstructionsFromInputNode already applies to instructions: a node counts when it is a
        // template node itself, or has a path to one. HasInputConnectionFromNode walks transitively, so an entire disconnected subgraph
        // is excluded rather than only the node on its end.
        // Iterate in execution order rather than over Graph::GetNodes() directly. That map is an unordered_map, and m_classDefinitions
        // and m_functionDefinitions are vectors appended to in iteration order, so hash order was leaking straight into the line order
        // of the generated O3DE_GENERATED_FUNCTIONS block. On this project that block runs to hundreds of lines; reshuffling it changes
        // the generated file, and a changed file is a full rebuild of every shader. m_includePaths escaped this only because it happens
        // to be a set.
        const auto nodesInExecutionOrder = GetAllNodesInExecutionOrder();

        AZStd::vector<GraphModel::ConstNodePtr> templateNodes;
        for (const auto& currentNode : nodesInExecutionOrder)
        {
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                bool hasTemplatePaths = false;
                AtomToolsFramework::VisitDynamicNodeSettings(
                    dynamicNode->GetConfig(),
                    [&hasTemplatePaths](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                    {
                        hasTemplatePaths = hasTemplatePaths || settings.contains("templatePaths");
                    });

                if (hasTemplatePaths)
                {
                    templateNodes.push_back(currentNode);
                }
            }
        }

        for (const auto& currentNode : nodesInExecutionOrder)
        {
            bool contributesToGeneratedFile = false;
            for (const auto& templateNode : templateNodes)
            {
                if (templateNode == currentNode || templateNode->HasInputConnectionFromNode(currentNode))
                {
                    contributesToGeneratedFile = true;
                    break;
                }
            }

            if (!contributesToGeneratedFile)
            {
                continue;
            }

            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                if (!m_configIdsVisited.contains(dynamicNode->GetConfig().m_id))
                {
                    m_configIdsVisited.insert(dynamicNode->GetConfig().m_id);
                    AtomToolsFramework::VisitDynamicNodeSettings(
                        dynamicNode->GetConfig(),
                        [&](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                        {
                            AtomToolsFramework::CollectDynamicNodeSettings(settings, "includePaths", m_includePaths);
                            AtomToolsFramework::CollectDynamicNodeSettings(settings, "classDefinitions", m_classDefinitions);
                            AtomToolsFramework::CollectDynamicNodeSettings(settings, "functionDefinitions", m_functionDefinitions);
                        });
                }
            }
        }
    }

    void MaterialGraphCompiler::BuildTemplatePathsForCurrentNode(const GraphModel::ConstNodePtr& currentNode)
    {
        m_templatePathsForCurrentNode.clear();
        if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
        {
            AtomToolsFramework::VisitDynamicNodeSettings(
                dynamicNode->GetConfig(),
                [&](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                {
                    AtomToolsFramework::CollectDynamicNodeSettings(settings, "templatePaths", m_templatePathsForCurrentNode);
                });
        }
    }

    bool MaterialGraphCompiler::LoadTemplatesForCurrentNode()
    {
        m_templateFileDataVecForCurrentNode.clear();

        for (const auto& templatePath : m_templatePathsForCurrentNode)
        {
            if (!templatePath.ends_with(".materialtype"))
            {
                // Load the unmodified, template source file data, which will be copied and used for insertions, substitutions, and
                // code generation.
                AtomToolsFramework::GraphTemplateFileData templateFileData;
                AtomToolsFramework::GraphTemplateFileDataCacheRequestBus::EventResult(
                    templateFileData,
                    m_toolId,
                    &AtomToolsFramework::GraphTemplateFileDataCacheRequestBus::Events::Load,
                    AtomToolsFramework::GetPathWithoutAlias(templatePath));

                if (!templateFileData.IsLoaded())
                {
                    m_templateFileDataVecForCurrentNode.clear();
                    return false;
                }

                m_templateFileDataVecForCurrentNode.emplace_back(AZStd::move(templateFileData));
            }
        }
        return true;
    }

    void MaterialGraphCompiler::DeleteExistingFilesForCurrentNode()
    {
        if (AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/ForceDeleteGeneratedFiles", false))
        {
            AZ::parallel_for_each(
                m_templateFileDataVecForCurrentNode.begin(),
                m_templateFileDataVecForCurrentNode.end(),
                [this](const auto& templateFileData)
                {
                    const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templateFileData.GetPath());
                    const auto& templateOutputPath = GetOutputPathFromTemplatePath(templateInputPath);

                    auto fileIO = AZ::IO::FileIOBase::GetInstance();
                    fileIO->Remove(templateOutputPath.c_str());
                });
        }
    }

    void MaterialGraphCompiler::ClearFingerprintsForCurrentNode()
    {
        if (AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/ForceClearAssetFingerprints", false))
        {
            for (const auto& templatePath : m_templatePathsForCurrentNode)
            {
                if (templatePath.ends_with(".material") || templatePath.ends_with(".materialtype"))
                {
                    const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
                    const auto& templateOutputPath = GetOutputPathFromTemplatePath(templateInputPath);
                    AzToolsFramework::AssetSystemRequestBus::Broadcast(
                        &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, templateOutputPath);
                }
            }
        }
    }

    void MaterialGraphCompiler::PreprocessTemplatesForCurrentNode()
    {
        AZ::parallel_for_each(
            m_templateFileDataVecForCurrentNode.begin(),
            m_templateFileDataVecForCurrentNode.end(),
            [&](auto& templateFileData)
            {
                // Substitute all references to the placeholder graph name with one generated from the document name
                templateFileData.ReplaceSymbol("MaterialGraphName", GetUniqueGraphName());

                // Inject include files found while traversing the graph into any include file blocks in the template.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_INCLUDES_BEGIN",
                    "O3DE_GENERATED_INCLUDES_END",
                    [&, this]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        // Include file paths will need to be converted to include statements.
                        AZStd::vector<AZStd::string> includeStatements;
                        includeStatements.reserve(m_includePaths.size());

                        for (const auto& path : m_includePaths)
                        {
                            bool relativePathFound = false;
                            AZStd::string relativePath;
                            AZStd::string relativePathFolder;

                            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                                relativePathFound,
                                &AzToolsFramework::AssetSystem::AssetSystemRequest::GenerateRelativeSourcePath,
                                AtomToolsFramework::GetPathWithoutAlias(path),
                                relativePath,
                                relativePathFolder);

                            if (relativePathFound)
                            {
                                includeStatements.push_back(AZStd::string::format("#include <%s>", relativePath.c_str()));
                            }
                        }
                        return includeStatements;
                    });

                // Inject class definitions found while traversing the graph.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_CLASSES_BEGIN",
                    "O3DE_GENERATED_CLASSES_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return m_classDefinitions;
                    });

                // Inject function definitions found while traversing the graph.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_FUNCTIONS_BEGIN",
                    "O3DE_GENERATED_FUNCTIONS_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return m_functionDefinitions;
                    });
            });
    }

    void MaterialGraphCompiler::BuildInstructionsForCurrentNode(const GraphModel::ConstNodePtr& currentNode)
    {
        if (!m_graph)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid graph object.");
            return;
        }

        m_instructionNodesForCurrentNode.clear();
        m_instructionNodesForCurrentNode.reserve(m_graph->GetNodeCount());

        AZ::parallel_for_each(
            m_templateFileDataVecForCurrentNode.begin(),
            m_templateFileDataVecForCurrentNode.end(),
            [&](auto& templateFileData)
            {
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_INSTRUCTIONS_BEGIN",
                    "O3DE_GENERATED_INSTRUCTIONS_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        AZStd::vector<AZStd::string> inputSlotNames;
                        AZ::StringFunc::Tokenize(blockHeader, inputSlotNames, ";:, \t\r\n\\/", false, false);

                        AZStd::vector<GraphModel::ConstNodePtr> instructionNodesForBlock;
                        instructionNodesForBlock.reserve(m_graph->GetNodeCount());
                        const auto& lines = GetInstructionsFromConnectedNodes(currentNode, inputSlotNames, instructionNodesForBlock);

                        // Adding all of the contributing notes from this blog to the set of all nodes for all blocks.
                        AZStd::scoped_lock lock(m_instructionNodesForCurrentNodeMutex);
                        m_instructionNodesForCurrentNode.insert(
                            m_instructionNodesForCurrentNode.end(), instructionNodesForBlock.begin(), instructionNodesForBlock.end());
                        return lines;
                    });
            });

        // All of the instruction nodes are gathered in temporary vectors and the results concatenated. The vector needs to be reduced
        // to only contain unique nodes and then resorted by depth.
        AZStd::sort(m_instructionNodesForCurrentNode.begin(), m_instructionNodesForCurrentNode.end());
        m_instructionNodesForCurrentNode.erase(
            AZStd::unique(m_instructionNodesForCurrentNode.begin(), m_instructionNodesForCurrentNode.end()),
            m_instructionNodesForCurrentNode.end());
        AtomToolsFramework::SortNodesInExecutionOrder(m_instructionNodesForCurrentNode);
    }

    void MaterialGraphCompiler::BuildMaterialSrgForCurrentNode()
    {
        AZ::parallel_for_each(
            m_templateFileDataVecForCurrentNode.begin(),
            m_templateFileDataVecForCurrentNode.end(),
            [&](auto& templateFileData)
            {
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_MATERIAL_SRG_BEGIN",
                    "O3DE_GENERATED_MATERIAL_SRG_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return GetMaterialPropertySrgMemberFromNodes(m_instructionNodesForCurrentNode);
                    });
            });
    }

    bool MaterialGraphCompiler::BuildMaterialTypeForCurrentNode(const GraphModel::ConstNodePtr& currentNode)
    {
        for (const auto& templatePath : m_templatePathsForCurrentNode)
        {
            if (!templatePath.ends_with(".materialtype"))
            {
                continue;
            }

            // Remove any aliases to resolve the absolute path to the template file
            const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
            const auto& templateOutputPath = GetOutputPathFromTemplatePath(templateInputPath);
            if (!BuildMaterialTypeFromTemplate(currentNode, m_instructionNodesForCurrentNode, templateInputPath, templateOutputPath))
            {
                return false;
            }

            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, templateOutputPath);
            m_generatedFiles.push_back(templateOutputPath);
        }
        return true;
    }

    bool MaterialGraphCompiler::ExportTemplatesMatchingRegex(const AZStd::string& pattern)
    {
        const AZStd::regex patternRegex(pattern, AZStd::regex::flag_type::icase);
        for (const auto& templateFileData : m_templateFileDataVecForCurrentNode)
        {
            if (IsCancelRequested())
            {
                return false;
            }

            if (AZStd::regex_match(templateFileData.GetPath(), patternRegex))
            {
                const auto& templateOutputPath = GetOutputPathFromTemplatePath(templateFileData.GetPath());
                bool wroteFile = false;
                if (!templateFileData.Save(templateOutputPath, &wroteFile))
                {
                    return false;
                }

                // Anything written here is shader code, a lua functor or a material, none of which can be reflected in the viewport
                // without rebuilding assets, so the fast path is off for this compile.
                if (wroteFile)
                {
                    m_wroteAnyGeneratedFile = true;
                    m_onlyMaterialPropertyValuesChanged = false;
                }

                AzFramework::AssetSystemRequestBus::Broadcast(
                    &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, templateOutputPath);
                m_generatedFiles.push_back(templateOutputPath);
            }
        }
        return true;
    }

    namespace
    {
        // Native separators and no trailing separator, so that one folder path can be tested as a prefix of another.
        void NormalizeFolderPathForComparison(AZStd::string& folderPath)
        {
            AZ::StringFunc::Path::Normalize(folderPath);
            while (!folderPath.empty() && (folderPath.back() == '/' || folderPath.back() == '\\'))
            {
                folderPath.pop_back();
            }
        }
    } // namespace

    bool MaterialGraphCompiler::IsPreviewOutputEnabled()
    {
        return AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/EnablePreviewOnlyMaterialPipeline", false);
    }

    bool MaterialGraphCompiler::IsInMemoryPreviewMaterialEnabled()
    {
        return IsPreviewOutputEnabled() &&
            AtomToolsFramework::GetSettingsValue<bool>("/O3DE/Atom/MaterialCanvas/EnableInMemoryPreviewMaterial", false);
    }

    bool MaterialGraphCompiler::IsPreviewOutputPath(AZStd::string_view path)
    {
        // Matched on the root folder name rather than on the full path. The same asset is named differently by the source tree, by the
        // cache and by the asset system, and those spellings agree on no prefix and on no separator, but all of them keep the folder.
        AZStd::string normalizedPath(path);
        AZ::StringFunc::Replace(normalizedPath, '\\', '/');

        const AZStd::string rootFolderToken = AZStd::string::format("/%s/", PreviewOutputRootFolderName);
        return AZ::StringFunc::Find(normalizedPath, rootFolderToken) != AZStd::string::npos;
    }

    AZStd::string MaterialGraphCompiler::GetPreviewOutputFolderForGraph() const
    {
        // GetProjectPath returns a FixedMaxPathString, which has no conversion to AZStd::string. GetGraphPath above goes through
        // c_str() for the same reason.
        AZStd::string projectFolder = AZ::Utils::GetProjectPath().c_str();
        NormalizeFolderPathForComparison(projectFolder);

        AZStd::string previewFolder;
        AZ::StringFunc::Path::Join(projectFolder.c_str(), PreviewOutputRootRelativePath, previewFolder);

        AZStd::string graphFolder = GetGraphPath();
        AZ::StringFunc::Path::StripFullName(graphFolder);
        NormalizeFolderPathForComparison(graphFolder);

        // Mirror the graph's own folder under the root, so that two graphs sharing a file name in different folders do not write over
        // each other. The separator test is what stops a project at C:/Foo from claiming a graph at C:/Foobar.
        if (graphFolder.size() > projectFolder.size() && AZ::StringFunc::StartsWith(graphFolder, projectFolder, false) &&
            (graphFolder[projectFolder.size()] == '/' || graphFolder[projectFolder.size()] == '\\'))
        {
            AZStd::string mirroredFolder;
            AZ::StringFunc::Path::Join(previewFolder.c_str(), graphFolder.substr(projectFolder.size() + 1).c_str(), mirroredFolder);
            previewFolder = AZStd::move(mirroredFolder);
        }

        // A graph from outside the project has nothing to mirror and lands in the root itself. Two such graphs with the same name open
        // at once would collide, which is a narrow enough case to accept rather than to hash a path over.
        return previewFolder;
    }

    void MaterialGraphCompiler::RecordProductionOutputStaleness()
    {
        // Production output is stale when preview output is newer than it, or when it does not exist at all.
        //
        // Comparing modification times rather than hashing the graph gets the awkward cases right for nothing. Every writer in this file
        // leaves a file alone when its content has not changed, so a compile that changes nothing moves no timestamp and reports no
        // staleness, and moving a node around the graph view does not either. A save writes preview before production, so the two come
        // out of a save in the right order.
        if (!IsPreviewOutputEnabled())
        {
            return;
        }

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO || m_productionOutputStale)
        {
            return;
        }

        for (const auto& templatePath : m_templatePathsForCurrentNode)
        {
            const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
            const auto& previewOutputPath = GetOutputPathFromTemplatePath(templateInputPath, OutputSet::Preview);

            // A preview file that was never written says nothing about the production one.
            if (!fileIO->Exists(previewOutputPath.c_str()))
            {
                continue;
            }

            const auto& productionOutputPath = GetOutputPathFromTemplatePath(templateInputPath, OutputSet::Production);
            if (!fileIO->Exists(productionOutputPath.c_str()) ||
                fileIO->ModificationTime(previewOutputPath.c_str()) > fileIO->ModificationTime(productionOutputPath.c_str()))
            {
                m_productionOutputStale = true;
                return;
            }
        }
    }

    AZStd::vector<MaterialGraphCompiler::OutputSet> MaterialGraphCompiler::GetOutputSetsForThisCompile() const
    {
        // With the preview off there is one set and it is the production one, which is exactly the behaviour this tool had before the
        // preview pipeline existed.
        if (!IsPreviewOutputEnabled())
        {
            return { OutputSet::Production };
        }

        // An edit writes the preview alone. This is the point of the split: the material type the engine loads keeps describing the last
        // saved state of the graph, so a level rendering this material is unaffected by the fact that someone has its graph open.
        if (!IsProductionOutputRequested())
        {
            return { OutputSet::Preview };
        }

        // A save owes both. Preview first, so the viewport can resolve its material while the production shaders are still building.
        return { OutputSet::Preview, OutputSet::Production };
    }

    bool MaterialGraphCompiler::IsViewportOutputSet() const
    {
        return m_currentOutputSet == (IsPreviewOutputEnabled() ? OutputSet::Preview : OutputSet::Production);
    }

    bool MaterialGraphCompiler::EnsureOutputFolderExists() const
    {
        // The production set is written beside the graph, in a folder that exists because the graph is in it.
        if (m_currentOutputSet != OutputSet::Preview)
        {
            return true;
        }

        const AZStd::string previewFolder = GetPreviewOutputFolderForGraph();

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO && !fileIO->Exists(previewFolder.c_str()) && !fileIO->CreatePath(previewFolder.c_str()))
        {
            AZ_Error("MaterialGraphCompiler", false, "Preview output folder could not be created: '%s'.", previewFolder.c_str());
            return false;
        }
        return true;
    }

    bool MaterialGraphCompiler::ExportOutputSetForCurrentNode(const GraphModel::ConstNodePtr& currentNode, OutputSet outputSet)
    {
        m_currentOutputSet = outputSet;

        if (!EnsureOutputFolderExists())
        {
            return false;
        }

        // Force delete prior versions of files to be generated if settings are configured to do so.
        DeleteExistingFilesForCurrentNode();

        // Reset the asset processor fingerprint for material and material type files to force them to recompile even if nothing
        // changed. Source material files, generated by material canvas graph templates, never change after they're first generated.
        // Material type and shader source files change constantly based on the configuration of the graph. The AP is not
        // reprocessing or triggering asset notifications for unmodified material assets even though the dependencies are changing.
        // AssetSystemRequestBus::Events::ClearFingerprintForAsset is rarely used but specifically documented to resolve this problem.
        // The consequence is that reflecting some changes in the preview may take more time because all generated assets will be
        // reprocessed including material types where only a material input might have changed.
        ClearFingerprintsForCurrentNode();

        // Save all of the generated files except for materials and material types. Generated material type files must be saved after
        // generated shader files to prevent AP errors because of missing dependencies.
        if (!ExportTemplatesMatchingRegex(".*\\.lua\\b") ||
            !ExportTemplatesMatchingRegex(".*\\.azsli\\b") ||
            !ExportTemplatesMatchingRegex(".*\\.azsl\\b") ||
            !ExportTemplatesMatchingRegex(".*\\.shader\\b"))
        {
            return false;
        }

        // Process material type template files, injecting properties from material input nodes.
        if (!BuildMaterialTypeForCurrentNode(currentNode))
        {
            return false;
        }

        // After the material types have been processed and saved, save the materials that reference them. This is a real build
        // rather than a template copy because the material now carries the graph's material input values.
        if (!BuildMaterialForCurrentNode())
        {
            return false;
        }

        return true;
    }

    void MaterialGraphCompiler::DeleteStalePreviewOutputForCurrentNode()
    {
        if (IsPreviewOutputEnabled())
        {
            return;
        }

        // Deliberately not gated on ForceDeleteGeneratedFiles, unlike DeleteExistingFilesForCurrentNode. These are not previous versions
        // of something this compile is about to write, they are the entire output of a mode that is now off.
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return;
        }

        for (const auto& templatePath : m_templatePathsForCurrentNode)
        {
            const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
            const auto& previewOutputPath = GetOutputPathFromTemplatePath(templateInputPath, OutputSet::Preview);
            if (fileIO->Exists(previewOutputPath.c_str()))
            {
                AZ_TracePrintf_IfTrue(
                    "MaterialGraphCompiler", IsCompileLoggingEnabled(), "Removing stale preview output: %s\n", previewOutputPath.c_str());
                fileIO->Remove(previewOutputPath.c_str());
            }
        }

        // The folder itself is left alone. Removing it would mean deciding whether anything else in it belongs to someone, and an empty
        // folder costs nothing.
    }

    AZStd::string MaterialGraphCompiler::GetOutputPathFromTemplatePath(const AZStd::string& templateInputPath) const
    {
        return GetOutputPathFromTemplatePath(templateInputPath, m_currentOutputSet);
    }

    AZStd::string MaterialGraphCompiler::GetOutputPathFromTemplatePath(
        const AZStd::string& templateInputPath, OutputSet outputSet) const
    {
        AZStd::string templateInputFileName;
        AZ::StringFunc::Path::GetFullFileName(templateInputPath.c_str(), templateInputFileName);

        AZStd::string templateOutputPath = GetGraphPath();
        AZ::StringFunc::Path::ReplaceFullName(templateOutputPath, templateInputFileName.c_str());

        // The preview set keeps the file names and changes only the folder. Everything a generated file references by name -- the
        // material's material type, the material type's generated azsli -- therefore resolves within whichever set is being written,
        // with no path rewriting, and the two sets land in different intermediate asset folders so their shaders cannot collide.
        if (outputSet == OutputSet::Preview)
        {
            AZ::StringFunc::Path::Join(GetPreviewOutputFolderForGraph().c_str(), templateInputFileName.c_str(), templateOutputPath);
        }

        AZ::StringFunc::Replace(templateOutputPath, "MaterialGraphName", GetUniqueGraphName().c_str());
        return templateOutputPath;
    }

    unsigned int MaterialGraphCompiler::GetVectorSize(const AZStd::any& slotValue) const
    {
        if (slotValue.is<AZ::Color>())
        {
            return 4;
        }
        if (slotValue.is<AZ::Vector4>())
        {
            return 4;
        }
        if (slotValue.is<AZ::Vector3>())
        {
            return 3;
        }
        if (slotValue.is<AZ::Vector2>())
        {
            return 2;
        }
        if (slotValue.is<bool>() || slotValue.is<int>() || slotValue.is<unsigned int>() || slotValue.is<float>())
        {
            return 1;
        }
        return 0;
    }

    AZStd::any MaterialGraphCompiler::ConvertToScalar(const AZStd::any& slotValue) const
    {
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::any(v->GetR());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        return slotValue;
    }

    template<typename T>
    AZStd::any MaterialGraphCompiler::ConvertToVector(const AZStd::any& slotValue) const
    {
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::any(T(v->GetAsVector4()));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        return slotValue;
    }

    AZStd::any MaterialGraphCompiler::ConvertToVector(const AZStd::any& slotValue, unsigned int score) const
    {
        switch (score)
        {
        case 4:
            // Skipping color to vector conversions so that they export as the correct type with the material type.
            return slotValue.is<AZ::Color>() ? slotValue : ConvertToVector<AZ::Vector4>(slotValue);
        case 3:
            // Skipping color to vector conversions so that they export as the correct type with the material type.
            return slotValue.is<AZ::Color>() ? slotValue : ConvertToVector<AZ::Vector3>(slotValue);
        case 2:
            return ConvertToVector<AZ::Vector2>(slotValue);
        case 1:
            return ConvertToScalar(slotValue);
        default:
            return slotValue;
        }
    }

    AZStd::any MaterialGraphCompiler::GetValueFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        const auto& slotItr = m_slotValueTable.find(slot);
        return slotItr != m_slotValueTable.end() ? slotItr->second : slot->GetValue();
    }

    AZStd::any MaterialGraphCompiler::GetValueFromSlotOrConnection(GraphModel::ConstSlotPtr slot) const
    {
         for (const auto& connection : slot->GetConnections())
        {
             auto sourceSlot = connection->GetSourceSlot();
             auto targetSlot = connection->GetTargetSlot();
             if (targetSlot == slot)
             {
                return GetValueFromSlotOrConnection(sourceSlot);
            }
        }

        return GetValueFromSlot(slot);
    }

    AZStd::string MaterialGraphCompiler::GetAzslTypeFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        const auto& slotValue = GetValueFromSlot(slot);
        const auto& slotDataType = slot->GetGraphContext()->GetDataTypeForValue(slotValue);
        const auto& slotDataTypeName = slotDataType ? slotDataType->GetDisplayName() : AZStd::string{};

        if (AZ::StringFunc::Equal(slotDataTypeName, "color"))
        {
            return "float4";
        }

        return slotDataTypeName;
    }

    AZStd::string MaterialGraphCompiler::GetAzslValueFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        const auto& slotValue = GetValueFromSlot(slot);

        // This code and some of these rules will be refactored and generalized after splitting this class into a document and builder or
        // compiler class. Once that is done, it will be easier to register types, conversions, substitutions with the system.
        for (const auto& connection : slot->GetConnections())
        {
            auto sourceSlot = connection->GetSourceSlot();
            auto targetSlot = connection->GetTargetSlot();
            if (targetSlot == slot)
            {
                // If there is an incoming connection to this slot, the name of the source slot from the incoming connection will be used as
                // part of the value for the slot. It must be cast to the correct vector type for generated code. These conversions will be
                // extended once the code generator is separated from the document class.
                const auto& sourceSlotValue = GetValueFromSlot(sourceSlot);
                const auto& sourceSlotSymbolName = GetSymbolNameFromSlot(sourceSlot);
                if (slotValue.is<AZ::Vector2>())
                {
                    if (sourceSlotValue.is<AZ::Vector3>() ||
                        sourceSlotValue.is<AZ::Vector4>() ||
                        sourceSlotValue.is<AZ::Color>())
                    {
                        return AZStd::string::format("(float2)%s", sourceSlotSymbolName.c_str());
                    }
                }
                if (slotValue.is<AZ::Vector3>())
                {
                    if (sourceSlotValue.is<AZ::Vector2>())
                    {
                        return AZStd::string::format("float3(%s, 0)", sourceSlotSymbolName.c_str());
                    }
                    if (sourceSlotValue.is<AZ::Vector4>() ||
                        sourceSlotValue.is<AZ::Color>())
                    {
                        return AZStd::string::format("(float3)%s", sourceSlotSymbolName.c_str());
                    }
                }
                if (slotValue.is<AZ::Vector4>() ||
                    slotValue.is<AZ::Color>())
                {
                    if (sourceSlotValue.is<AZ::Vector2>())
                    {
                        return AZStd::string::format("float4(%s, 0, 1)", sourceSlotSymbolName.c_str());
                    }
                    if (sourceSlotValue.is<AZ::Vector3>())
                    {
                        return AZStd::string::format("float4(%s, 1)", sourceSlotSymbolName.c_str());
                    }
                }

                // A scalar target fed by a vector source. The table above only widens, so this case fell through to a bare assignment:
                // "float inAlpha = node33_color_ramp_input_outColor;". That is legal HLSL and takes the first component, but DXC reports
                // it as "implicit truncation of vector type [-Wconversion]" on every color or vector wired into a float slot. Writing the
                // swizzle changes nothing about what the shader computes; it just says which component was taken, which is worth stating
                // outright since a colour reaching a scalar slot is as often a mis-wire as it is deliberate.
                if (slotValue.is<bool>() || slotValue.is<int>() || slotValue.is<unsigned int>() || slotValue.is<float>())
                {
                    if (sourceSlotValue.is<AZ::Vector2>() ||
                        sourceSlotValue.is<AZ::Vector3>() ||
                        sourceSlotValue.is<AZ::Vector4>() ||
                        sourceSlotValue.is<AZ::Color>())
                    {
                        return AZStd::string::format("%s.x", sourceSlotSymbolName.c_str());
                    }
                }

                return sourceSlotSymbolName;
            }
        }

        // If the slot's embedded value is being used then generate shader code to represent it. More generic options will be explored to
        // clean this code up, possibly storing numeric values in a two-dimensional floating point array with the layout corresponding to
        // most vector and matrix types.
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g, %g}", v->GetR(), v->GetG(), v->GetB(), v->GetA());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g, %g}", v->GetX(), v->GetY(), v->GetZ(), v->GetW());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g}", v->GetX(), v->GetY(), v->GetZ());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::string::format("{%g, %g}", v->GetX(), v->GetY());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector2, 2>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(),
                value[1].GetX(), value[1].GetY());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector3, 3>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector4, 3>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(), value[0].GetW(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(), value[1].GetW(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ(), value[2].GetW());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector4, 4>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(), value[0].GetW(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(), value[1].GetW(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ(), value[2].GetW(),
                value[3].GetX(), value[3].GetY(), value[3].GetZ(), value[3].GetW());
        }
        if (auto v = AZStd::any_cast<const float>(&slotValue))
        {
            return AZStd::string::format("%g", *v);
        }
        if (auto v = AZStd::any_cast<const int>(&slotValue))
        {
            return AZStd::string::format("%i", *v);
        }
        if (auto v = AZStd::any_cast<const unsigned int>(&slotValue))
        {
            return AZStd::string::format("%u", *v);
        }
        if (auto v = AZStd::any_cast<const bool>(&slotValue))
        {
            return AZStd::string::format("%u", *v ? 1 : 0);
        }
        if (auto v = AZStd::any_cast<const AZStd::string>(&slotValue))
        {
            return *v;
        }
        return AZStd::string();
    }

    AZStd::string MaterialGraphCompiler::GetAzslSrgMemberFromSlot(
        GraphModel::ConstNodePtr node, const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig) const
    {
        if (const auto& slot = node->GetSlot(slotConfig.m_name))
        {
            const auto& slotValue = GetValueFromSlot(slot);
            if (auto v = AZStd::any_cast<const AZ::RHI::SamplerState>(&slotValue))
            {
                // The fields commented out below either cause errors or are not recognized by the shader compiler.
                AZStd::string srgMember;
                srgMember += AZStd::string::format("Sampler SLOTNAME\n");
                srgMember += AZStd::string::format("{\n");
                srgMember += AZStd::string::format("MaxAnisotropy = %u;\n", AZStd::max<uint32_t>(v->m_anisotropyMax, 1));
                //srgMember += AZStd::string::format("AnisotropyEnable = %u;\n", AZStd::clamp<uint32_t>(v->m_anisotropyEnable, 0, 1);
                srgMember += AZStd::string::format("MinFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMin).data());
                srgMember += AZStd::string::format("MagFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMag).data());
                srgMember += AZStd::string::format("MipFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMip).data());
                srgMember += AZStd::string::format("ReductionType = %s;\n", AZ::RHI::ReductionTypeNamespace::ToString(v->m_reductionType).data());
                //srgMember += AZStd::string::format("ComparisonFunc = %s;\n", AZ::RHI::ComparisonFuncNamespace::ToString(v->m_comparisonFunc).data());
                srgMember += AZStd::string::format("AddressU = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressU).data());
                srgMember += AZStd::string::format("AddressV = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressV).data());
                srgMember += AZStd::string::format("AddressW = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressW).data());
                srgMember += AZStd::string::format("MinLOD = %f;\n", AZStd::max(v->m_mipLodMin, 0.0f));
                srgMember += AZStd::string::format("MaxLOD = %f;\n", AZStd::max(v->m_mipLodMax, 0.0f));
                srgMember += AZStd::string::format("MipLODBias = %f;\n", AZStd::max(v->m_mipLodBias, 0.0f));
                srgMember += AZStd::string::format("BorderColor = %s;\n", AZ::RHI::BorderColorNamespace::ToString(v->m_borderColor).data());
                srgMember += "};\n";
                return srgMember;
            }

            if (AZStd::any_cast<const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(&slotValue))
            {
                return AZStd::string::format("Texture2D SLOTNAME;\n");
            }

            return AZStd::string::format("SLOTTYPE SLOTNAME;\n");
        }

        return AZStd::string();
    }

    AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> MaterialGraphCompiler::GetSubstitutionSymbolsFromNode(
        GraphModel::ConstNodePtr node) const
    {
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> substitutionSymbols;

        // Reserving space for the number of elements added in this function. 
        substitutionSymbols.reserve(node->GetSlots().size() * 4 + 1);
        substitutionSymbols.emplace_back("NODEID", GetSymbolNameFromNode(node));

        for (const auto& slotPair : node->GetSlots())
        {
            const auto& slot = slotPair.second;

            // These substitutions will allow accessing the slot ID, type, value from anywhere in the node's shader code.
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTTYPE\\(%s\\)", slot->GetName().c_str()), GetAzslTypeFromSlot(slot));
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTVALUE\\(%s\\)", slot->GetName().c_str()), GetAzslValueFromSlot(slot));
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTNAME\\(%s\\)", slot->GetName().c_str()), GetSymbolNameFromSlot(slot));

            // This expression will allow direct substitution of node variable names in node configurations with the decorated symbol name.
            // It will match whole words only. No additional decoration should be required on the node configuration side. However, support
            // for the older slot type, name, value substitutions are still supported as a convenience.
            substitutionSymbols.emplace_back(AZStd::string::format("\\b%s\\b", slot->GetName().c_str()), GetSymbolNameFromSlot(slot));
        }

        return substitutionSymbols;
    }

    AZStd::vector<AZStd::string> MaterialGraphCompiler::GetInstructionsFromSlot(
        GraphModel::ConstNodePtr node,
        const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const
    {
        AZStd::vector<AZStd::string> instructionsForSlot;

        auto slot = node->GetSlot(slotConfig.m_name);
        if (slot && (slot->GetSlotDirection() != GraphModel::SlotDirection::Output || !slot->GetConnections().empty()))
        {
            AtomToolsFramework::CollectDynamicNodeSettings(slotConfig.m_settings, "instructions", instructionsForSlot);

            AtomToolsFramework::ReplaceSymbolsInContainer(substitutionSymbols, instructionsForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTNAME", GetSymbolNameFromSlot(slot), instructionsForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTTYPE", GetAzslTypeFromSlot(slot), instructionsForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTVALUE", GetAzslValueFromSlot(slot), instructionsForSlot);
        }

        return instructionsForSlot;
    }

    bool MaterialGraphCompiler::ShouldUseInstructionsFromInputNode(
        GraphModel::ConstNodePtr outputNode, GraphModel::ConstNodePtr inputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        if (inputNode == outputNode)
        {
            return true;
        }

        for (const auto& inputSlotName : inputSlotNames)
        {
            if (const auto slot = outputNode->GetSlot(inputSlotName))
            {
                if (slot->GetSlotDirection() == GraphModel::SlotDirection::Input)
                {
                    for (const auto& connection : slot->GetConnections())
                    {
                        AZ_Assert(connection->GetSourceNode() != outputNode, "This should never be the source node on an input connection.");
                        AZ_Assert(connection->GetTargetNode() == outputNode, "This should always be the target node on an input connection.");
                        if (connection->GetSourceNode() == inputNode || connection->GetSourceNode()->HasInputConnectionFromNode(inputNode))
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialGraphCompiler::GetAllNodesInExecutionOrder() const
    {
        AZStd::vector<GraphModel::ConstNodePtr> nodes;

        if (!m_graph)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid graph object.");
            return nodes;
        }

        nodes.reserve(m_graph->GetNodes().size());
        for (const auto& nodePair : m_graph->GetNodes())
        {
            nodes.push_back(nodePair.second);
        }

        AtomToolsFramework::SortNodesInExecutionOrder(nodes);
        return nodes;
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialGraphCompiler::GetInstructionNodesInExecutionOrder(
        GraphModel::ConstNodePtr outputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        AZStd::vector<GraphModel::ConstNodePtr> nodes = GetAllNodesInExecutionOrder();
        AZStd::erase_if(nodes, [this, &outputNode, &inputSlotNames](const auto& node) {
            return !ShouldUseInstructionsFromInputNode(outputNode, node, inputSlotNames);
        });
        return nodes;
    }

    AZStd::vector<AZStd::string> MaterialGraphCompiler::GetInstructionsFromConnectedNodes(
        GraphModel::ConstNodePtr outputNode,
        const AZStd::vector<AZStd::string>& inputSlotNames,
        AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const
    {
        AZStd::vector<AZStd::string> instructions;

        for (const auto& inputNode : GetInstructionNodesInExecutionOrder(outputNode, inputSlotNames))
        {
            // Build a list of all nodes that will contribute instructions for the output node
            if (AZStd::find(instructionNodes.begin(), instructionNodes.end(), inputNode) == instructionNodes.end())
            {
                instructionNodes.push_back(inputNode);
            }

            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get());
            if (dynamicNode)
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                const auto& substitutionSymbols = GetSubstitutionSymbolsFromNode(inputNode);

                // Instructions are gathered separately for all of the slot categories because they need to be added in a specific order.

                // Gather and perform substitutions on instructions embedded directly in the node.
                AZStd::vector<AZStd::string> instructionsForNode;
                AtomToolsFramework::CollectDynamicNodeSettings(nodeConfig.m_settings, "instructions", instructionsForNode);
                AtomToolsFramework::ReplaceSymbolsInContainer(substitutionSymbols, instructionsForNode);

                // Gather and perform substitutions on instructions contained in property slots.
                AZStd::vector<AZStd::string> instructionsForPropertySlots;
                for (const auto& slotConfig : nodeConfig.m_propertySlots)
                {
                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForPropertySlots.insert(instructionsForPropertySlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                // Gather and perform substitutions on instructions contained in input slots.
                AZStd::vector<AZStd::string> instructionsForInputSlots;
                for (const auto& slotConfig : nodeConfig.m_inputSlots)
                {
                    // If this is the output node, only gather instructions for requested input slots.
                    if (inputNode == outputNode &&
                        AZStd::find(inputSlotNames.begin(), inputSlotNames.end(), slotConfig.m_name) == inputSlotNames.end())
                    {
                        continue;
                    }

                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForInputSlots.insert(instructionsForInputSlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                // Gather and perform substitutions on instructions contained in output slots.
                AZStd::vector<AZStd::string> instructionsForOutputSlots;
                for (const auto& slotConfig : nodeConfig.m_outputSlots)
                {
                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForOutputSlots.insert(instructionsForOutputSlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                instructions.insert(instructions.end(), instructionsForPropertySlots.begin(), instructionsForPropertySlots.end());
                instructions.insert(instructions.end(), instructionsForInputSlots.begin(), instructionsForInputSlots.end());
                instructions.insert(instructions.end(), instructionsForNode.begin(), instructionsForNode.end());
                instructions.insert(instructions.end(), instructionsForOutputSlots.begin(), instructionsForOutputSlots.end());
            }
        }

        return instructions;
    }

    AZStd::string MaterialGraphCompiler::GetSymbolNameFromNode(GraphModel::ConstNodePtr node) const
    {
        return AtomToolsFramework::GetSymbolNameFromText(AZStd::string::format("node%u_%s", node->GetId(), node->GetTitle()));
    }

    AZStd::string MaterialGraphCompiler::GetSymbolNameFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        bool allowNameSubstitution = true;
        if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(slot->GetParentNode().get()))
        {
            const auto& nodeConfig = dynamicNode->GetConfig();
            AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                nodeConfig,
                [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                {
                    if (slot->GetName() == slotConfig.m_name)
                    {
                        allowNameSubstitution = slotConfig.m_allowNameSubstitution;
                    }
                });
        }

        if (!allowNameSubstitution)
        {
            return slot->GetName();
        }

        if (slot->SupportsExtendability())
        {
            return AZStd::string::format(
                "%s_%s_%d", GetSymbolNameFromNode(slot->GetParentNode()).c_str(), slot->GetName().c_str(), slot->GetSlotSubId());
        }

        return AZStd::string::format("%s_%s", GetSymbolNameFromNode(slot->GetParentNode()).c_str(), slot->GetName().c_str());
    }

    AZStd::vector<AZStd::string> MaterialGraphCompiler::GetMaterialPropertySrgMemberFromSlot(
        GraphModel::ConstNodePtr node,
        const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const
    {
        AZStd::vector<AZStd::string> materialPropertySrgMemberForSlot;

        if (auto slot = node->GetSlot(slotConfig.m_name))
        {
            AtomToolsFramework::CollectDynamicNodeSettings(slotConfig.m_settings, "materialPropertySrgMember", materialPropertySrgMemberForSlot);

            AtomToolsFramework::ReplaceSymbolsInContainer(substitutionSymbols, materialPropertySrgMemberForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("STANDARD_SRG_MEMBER", GetAzslSrgMemberFromSlot(node, slotConfig), materialPropertySrgMemberForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTNAME", GetSymbolNameFromSlot(slot), materialPropertySrgMemberForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTTYPE", GetAzslTypeFromSlot(slot), materialPropertySrgMemberForSlot);
            AtomToolsFramework::ReplaceSymbolsInContainer("SLOTVALUE", GetAzslValueFromSlot(slot), materialPropertySrgMemberForSlot);
        }

        return materialPropertySrgMemberForSlot;
    }

    AZStd::vector<AZStd::string> MaterialGraphCompiler::GetMaterialPropertySrgMemberFromNodes(
        const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const
    {
        if (!m_graph)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid graph object.");
            return {};
        }

        AZStd::vector<AZStd::string> materialPropertySrgMember;

        for (const auto& inputNode : instructionNodes)
        {
            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get());
            if (dynamicNode)
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                const auto& substitutionSymbols = GetSubstitutionSymbolsFromNode(inputNode);

                AZStd::vector<AZStd::string> materialPropertySrgMembersForNode;
                AtomToolsFramework::CollectDynamicNodeSettings(
                    nodeConfig.m_settings, "materialPropertySrgMember", materialPropertySrgMembersForNode);
                AtomToolsFramework::ReplaceSymbolsInContainer(substitutionSymbols, materialPropertySrgMembersForNode);

                AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                    nodeConfig,
                    [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                    {
                        const auto& materialPropertySrgMemberForSlot =
                            GetMaterialPropertySrgMemberFromSlot(inputNode, slotConfig, substitutionSymbols);
                        materialPropertySrgMembersForNode.insert(
                            materialPropertySrgMembersForNode.end(),
                            materialPropertySrgMemberForSlot.begin(),
                            materialPropertySrgMemberForSlot.end());
                    });

                materialPropertySrgMember.insert(
                    materialPropertySrgMember.end(), materialPropertySrgMembersForNode.begin(), materialPropertySrgMembersForNode.end());
            }
        }

        return materialPropertySrgMember;
    }

    bool MaterialGraphCompiler::BuildMaterialTypeFromTemplate(
        GraphModel::ConstNodePtr templateNode,
        const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes,
        const AZStd::string& templateInputPath,
        const AZStd::string& templateOutputPath)
    {
        using namespace AtomToolsFramework;

        if (!m_graph)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid graph object.");
            return false;
        }

        if (!templateNode)
        {
            AZ_Error("MaterialGraphCompiler", false, "Attempting to generate data from invalid template node.");
            return false;
        }

        // Load the material type template file, which is the same format as MaterialTypeSourceData with a different extension
        auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(templateInputPath);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialGraphCompiler", false, "Material type template could not be loaded: '%s'.", templateInputPath.c_str());
            return false;
        }

        // Copy the material type source data from the template and begin populating it.
        AZ::RPI::MaterialTypeSourceData materialTypeSourceData = materialTypeOutcome.TakeValue();

        // If the node providing all the template information has a description then assign it to the material type source data.
        materialTypeSourceData.m_description = GetStringValueFromSlot(templateNode->GetSlot("inDescription"));

        // The preview set, and only the preview set, is built through the preview pipeline alone.
        //
        // This replaces the settings registry file the tool used to copy into the project, which swapped
        // /O3DE/Atom/RPI/MaterialPipelineFiles out for the preview pipeline and therefore rebuilt every material type in the project
        // through it, whether or not it had anything to do with Material Canvas, and needed the Asset Processor restarted to put back.
        // Declaring the pipeline on the material type instead confines the choice to the graphs actually being edited, and takes effect
        // on the next compile rather than the next restart.
        //
        // Note that the preview pipeline masquerades as MainPipeline: its shaders carry that material pipeline tag, because the Material
        // Canvas viewport renders through the shared MainRenderPipeline asset and a render pipeline matches exactly one tag, so a preview
        // tag of its own would leave every other material in that viewport with no shaders. The consequence is that a preview material
        // type renders anywhere a production one would, at preview fidelity and without saying so, which is precisely why the two are
        // separate files rather than one file with a switch.
        if (m_currentOutputSet == OutputSet::Preview)
        {
            // Matched against the material pipeline file stem by MaterialTypeBuilder, which resolves it against the project's default
            // pipelines plus anything registered under /O3DE/Atom/RPI/OptInMaterialPipelineFiles. The MaterialCanvas gem registers its
            // preview pipeline there permanently, so nothing has to be copied anywhere for this name to resolve.
            materialTypeSourceData.m_buildSettings["materialPipelines"] = AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialCanvas/PreviewMaterialPipelineName", "MaterialCanvasPreview");
        }
        else
        {
            // Optionally narrow which pipelines the production set is built through.
            //
            // Left empty, the material type declares nothing and MaterialTypeBuilder builds it through every pipeline the project
            // registers, which by default is MainPipeline and LowEndPipeline. Those two produce near identical shaders -- measured at
            // 13,201 and 13,181 preprocessed lines for the same transparent Standard PBR material, 1,287 ms and 1,286 ms of azslc -- so
            // a project that does not ship a low end target pays for a second full set of shaders on every save and never loads them.
            //
            // Set this to "MainPipeline" to build only that one. Unrecognised names are reported by MaterialTypeBuilder and the default
            // list is used instead, so a typo costs a warning rather than a material that does not render.
            const AZStd::string productionMaterialPipelines = AtomToolsFramework::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialCanvas/ProductionMaterialPipelines", "");
            if (!productionMaterialPipelines.empty())
            {
                materialTypeSourceData.m_buildSettings["materialPipelines"] = productionMaterialPipelines;
            }
        }

        // Opacity mode is structural for a generated graph material: it selects the exact forward, cutout, blended, or tinted-transparent
        // shader group in the material pipeline scripts. Material types that do not provide this build setting retain the legacy Dynamic
        // fallback and build every group.
        const AZStd::string opacityMode = GetStringValueFromSlot(templateNode->GetSlot("inOpacityMode"));
        if (!opacityMode.empty())
        {
            materialTypeSourceData.m_buildSettings["opacityMode"] = opacityMode;
        }

        // Position-offset instructions run in vertex stages. Record whether the graph actually feeds this input so the pipeline scripts
        // can omit the extra depth, shadow, and motion-vector vertex passes for graphs that leave the position unchanged.
        const auto positionOffsetSlot = templateNode->GetSlot("inPositionOffset");
        if (positionOffsetSlot && !positionOffsetSlot->GetConnections().empty())
        {
            materialTypeSourceData.m_buildSettings["positionOffset"] = "Connected";
        }

        // Search the graph for nodes defining material input properties that should be added to the material type and material SRG
        for (const auto& inputNode : instructionNodes)
        {
            // Search for all slots with settings indicating that material type properties should be generated. The settings can correspond
            // to shader inputs, shader options, and other material property values that may or may not have matching entries in the
            // material SRG.
            AZStd::vector<AZStd::pair<GraphModel::ConstSlotPtr, DynamicNodeSlotConfig>> materialPropertyValueSlots;
            if (auto dynamicNode = azrtti_cast<const DynamicNode*>(inputNode.get()))
            {
                VisitDynamicNodeSlotConfigs(
                    dynamicNode->GetConfig(),
                    [&](const DynamicNodeSlotConfig& slotConfig)
                    {
                        if (slotConfig.m_settings.contains("materialPropertyName") ||
                            slotConfig.m_settings.contains("materialPropertyDisplayName") ||
                            slotConfig.m_settings.contains("materialPropertyConnectionType") ||
                            slotConfig.m_settings.contains("materialPropertyConnectionName") ||
                            slotConfig.m_settings.contains("materialPropertyGroupName") ||
                            slotConfig.m_settings.contains("materialPropertyGroup"))
                        {
                            const auto materialPropertyValueSlot = inputNode->GetSlot(slotConfig.m_name);
                            materialPropertyValueSlots.emplace_back(materialPropertyValueSlot, slotConfig);
                        }
                    });
            }

            // Register all the properties that were parsed out of the slots with the material type.
            for (const auto& [materialPropertyValueSlot, materialPropertyValueSlotConfig] : materialPropertyValueSlots)
            {
                // Sampler states are currently not configurable and will not be added added to the material type, just the material SRG.
                if (!materialPropertyValueSlot || materialPropertyValueSlot->GetValue().empty())
                {
                    continue;
                }

                const auto& materialPropertyValueSlotSymbolName = GetSymbolNameFromSlot(materialPropertyValueSlot);

                // If the property represents a shader option, the connection name will be defined in a static setting. Otherwise, it will
                // be the slot symbol name which is the same as the variable name added to the SRG and referenced in code.
                const auto& materialPropertyConnectionName = GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyConnectionName"),
                    materialPropertyValueSlotSymbolName
                    });

                // The material property connection type determines if the connection represents a shader option, shader input, internal
                // value, or just a placeholder property.
                const auto& materialPropertyConnectionType = GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyConnectionType")
                    });

                // While this might change, material properties representing shader inputs generally have their name, display name,
                // description, and other details spread across multiple, user configurable slots on the same node. Shader options don't
                // need a user configurable name or description because they refer to a predefined option name that will always be used the
                // same way. Several shader options can be exposed on the same node. Because of that, shader options must specify their
                // connection name and copy the name and description directly from the slot instead of having the users enter one.
                const auto& materialPropertyUseSlotConfig = !AZ::StringFunc::Equal(materialPropertyConnectionType, "ShaderInput");

                // The material property name must be unique relative to its group. Material property names are used to read and write
                // property values through the material system API. These will be stored with default values in the material type and
                // overridden values per material. In material canvas, rather than overwhelming the user with Learning and managing the
                // differences between IDs, names, and display names, we will generate the values for symbol and display names Based on a
                // single user specified material input name, slot settings, or the symbol name generated from the node and slot IDs.

                // Find the most appropriate name to use for this property, prioritizing static settings for shader options first.
                const auto& materialPropertyName = GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyName"),
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyDisplayName"),
                    materialPropertyUseSlotConfig ? materialPropertyValueSlotConfig.m_displayName : GetStringValueFromSlot(inputNode->GetSlot("inDisplayName")),
                    materialPropertyUseSlotConfig ? materialPropertyValueSlotConfig.m_name : GetStringValueFromSlot(inputNode->GetSlot("inName")),
                    materialPropertyValueSlotSymbolName
                    });

                // The symbol name used to uniquely identify the property in its group will be generated by transforming the above name to
                // lowercase and replacing all non word characters with underscores.
                const auto& materialPropertySymbolName = GetSymbolNameFromText(materialPropertyName);

                // The display name slot was removed from the original, experimental material output nodes but we are handling it for
                // backwards compatibility. The display name will otherwise be generated by sanitizing and camel casing the property name.
                const auto& materialPropertyDisplayName = GetDisplayNameFromText(GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyDisplayName"),
                    materialPropertyUseSlotConfig ? materialPropertyValueSlotConfig.m_displayName : GetStringValueFromSlot(inputNode->GetSlot("inDisplayName")),
                    materialPropertyName
                    }));

                if (materialPropertyName.empty() || materialPropertySymbolName.empty() || materialPropertyDisplayName.empty())
                {
                    AZ_Error(
                        "MaterialGraphCompiler",
                        false,
                        "Material property name could not be resolved for slot '%s' and template '%s'.",
                        materialPropertyValueSlotSymbolName.c_str(),
                        templateOutputPath.c_str());
                    return false;
                }

                // The group name can be specified in a static setting for shader options or configured for material inputs. Properties that
                // do not explicitly define a group will fall back to the general group.
                const auto& materialPropertyGroupName = GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyGroup"),
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyGroupName"),
                    GetStringValueFromSlot(inputNode->GetSlot("inGroup")),
                    "general"
                    });

                // Sanitize the symbol and display names for the group to force casing, spacing, and eliminate any potential erroneous input.
                const auto& materialPropertyGroupSymbolName = GetSymbolNameFromText(materialPropertyGroupName);
                const auto& materialPropertyGroupDisplayName = GetDisplayNameFromText(materialPropertyGroupName);
                if (materialPropertyGroupName.empty() || materialPropertyGroupDisplayName.empty())
                {
                    AZ_Error(
                        "MaterialGraphCompiler",
                        false,
                        "Material property group could not be resolved for slot '%s' and template '%s'.",
                        materialPropertyValueSlotSymbolName.c_str(),
                        templateOutputPath.c_str());
                    return false;
                }


                // The property description can also be read from static settings for shader options or a user configurable slot
                // for material inputs. If no description is specified, it will fall back to using the material property display name.
                const auto& materialPropertyDescription = GetFirstNonEmptyString({
                    GetSettingValueByName(materialPropertyValueSlotConfig.m_settings, "materialPropertyDescription"),
                    materialPropertyUseSlotConfig ? materialPropertyValueSlotConfig.m_description : GetStringValueFromSlot(inputNode->GetSlot("inDescription")),
                    materialPropertyDisplayName
                    });

                // Find or create a property group with the specified name
                auto propertyGroup = materialTypeSourceData.FindPropertyGroup(materialPropertyGroupSymbolName);
                if (!propertyGroup)
                {
                    // Add the property group to the material type if it was not already registered
                    propertyGroup = materialTypeSourceData.AddPropertyGroup(materialPropertyGroupSymbolName);
                    if (!propertyGroup)
                    {
                        AZ_Error(
                            "MaterialGraphCompiler",
                            false,
                            "Material property group '%s' could not be added for slot '%s' and template '%s'.",
                            materialPropertyGroupSymbolName.c_str(),
                            materialPropertyValueSlotSymbolName.c_str(),
                            templateOutputPath.c_str());
                        return false;
                    }

                    // The unmodified text value will be used as the display name and description for now
                    propertyGroup->SetDisplayName(materialPropertyGroupDisplayName);
                    propertyGroup->SetDescription(materialPropertyGroupDisplayName);
                }

                // Force material properties to be added with a unique names to prevent collisions that can occur if duplicating
                unsigned int uniqueNameIndex = 0;
                AZStd::string materialPropertySymbolNameUnique = materialPropertySymbolName;
                AZStd::string materialPropertyDisplayNameUnique = materialPropertyDisplayName;
                const auto& existingProperties = propertyGroup->GetProperties();
                while (AZStd::find_if(
                           existingProperties.begin(),
                           existingProperties.end(),
                           [&materialPropertySymbolNameUnique](const auto& existingProperty)
                           {
                               return materialPropertySymbolNameUnique == existingProperty->GetName();
                           }) != existingProperties.end())
                {
                    ++uniqueNameIndex;
                    materialPropertySymbolNameUnique = AZStd::string::format("%s_%u", materialPropertySymbolName.c_str(), uniqueNameIndex);
                    materialPropertyDisplayNameUnique = AZStd::string::format("%s (%u)", materialPropertyDisplayName.c_str(), uniqueNameIndex);
                }

                if (uniqueNameIndex > 0)
                {
                    AZ_Warning(
                        "MaterialGraphCompiler",
                        false,
                        "Material property '%s' Was exported with a unique name '%s' in group '%s' for slot '%s' and template '%s'.",
                        materialPropertySymbolName.c_str(),
                        materialPropertySymbolNameUnique.c_str(),
                        materialPropertyGroupSymbolName.c_str(),
                        materialPropertyValueSlotSymbolName.c_str(),
                        templateOutputPath.c_str());
                }

                auto property = propertyGroup->AddProperty(materialPropertySymbolNameUnique);
                if (!property)
                {
                    AZ_Error(
                        "MaterialGraphCompiler",
                        false,
                        "Material property '%s' could not be added to group '%s' for slot '%s' and template '%s'.",
                        materialPropertySymbolNameUnique.c_str(),
                        materialPropertyGroupSymbolName.c_str(),
                        materialPropertyValueSlotSymbolName.c_str(),
                        templateOutputPath.c_str());
                    return false;
                }

                // Lastly, the property value is read from the slot.
                const auto& materialPropertyValue = GetValueFromSlot(materialPropertyValueSlot);

                // The complete property ID is a combination of the group name and the property name.
                const AZ::Name materialPropertyId(materialPropertyGroupSymbolName + "." + materialPropertySymbolNameUnique);

                property->m_displayName = materialPropertyDisplayNameUnique;
                property->m_description = materialPropertyDescription;
                property->m_enumValues = materialPropertyValueSlotConfig.m_enumValues;
                property->m_value = AZ::RPI::MaterialPropertyValue::FromAny(materialPropertyValue);

                // The property definition requires an explicit type enum that's converted from the actual data type.
                property->m_dataType = GetMaterialPropertyDataTypeFromValue(property->m_value, !property->m_enumValues.empty());

                // Images and enums need additional conversion prior to being saved.
                ConvertToExportFormat(templateOutputPath, materialPropertyId, *property, property->m_value);

                // Keep the value the graph actually describes, then replace it in the material type with a placeholder of the same type.
                //
                // Property defaults are part of the material type's content, so leaving the graph's value here made every value edit
                // change the material type: its hash changed, MaterialTypeBuilder's pipeline stage re-ran, and the fingerprint
                // propagated to every shader job. Editing a single float rebuilt every shader for a change that touches no shader code.
                // The values are written into the generated .material instead, which only re-runs the material builder, and are handed
                // to the viewport over MaterialGraphCompilerNotificationBus so the preview updates without waiting for even that.
                //
                // The placeholder has to keep the type, because the type enum was derived from the value just above and the generated
                // MaterialParameters struct is built from the property layout.
                // A value only moves if the material type can be left with a placeholder of the same type and the generated material can
                // express the real one. Those turn out to be the same question, so the reset reports whether it happened and the value is
                // recorded only then. Tying the two halves together is deliberate: recording a value that never left the material type
                // would write it into both files, and blanking one the material cannot carry would lose it outright.
                //
                // Enums never move. An enum's value is a name drawn from m_enumValues and that set has no blank member, so the placeholder
                // is not a value of the property's own type. MaterialTypeAssetCreator rejects it twice, once for the type ("is a Enum
                // type, can only accept UInt value, input value is Invalid") and once for the lookup ("Enum value '' couldn't be found").
                // Leaving the real value in the material type costs one rebuild when the enum is edited, which for the enums that occur
                // here -- opacity mode above all -- is a structural change to the material that should rebuild anyway.
                //
                // Sampler states never move either, for the other half of the reason. A material stores its property values as bare JSON
                // with no type context, and MaterialPropertyValue's serializer has to infer the alternative from the shape of the value;
                // a sampler is an object, so it comes back as a color and MaterialAssetCreator reports "Type mismatch. Expected
                // SamplerState but was Vector4".
                //
                // The value moves out of every set that is built, but it is recorded once. Recording it per set would send the viewport
                // each value as many times as there are sets, and the sets agree on the values by construction. The viewport's own set is
                // the one that records, and it is built first, so the material written for either set has the full list to draw on.
                const AZ::RPI::MaterialPropertyValue materialPropertyValueBeforeReset = property->m_value;
                if (property->m_enumValues.empty() && ResetMaterialPropertyValueToTypeDefault(property->m_value))
                {
                    if (IsViewportOutputSet())
                    {
                        m_materialPropertyValues.emplace_back(materialPropertyId, materialPropertyValueBeforeReset);
                    }
                }

                // This property connects to the material SRG member with the same name. Shader options are not yet supported.
                if (!materialPropertyConnectionName.empty())
                {
                    if (AZ::StringFunc::Equal(materialPropertyConnectionType, "ShaderInput"))
                    {
                        property->m_outputConnections.emplace_back(
                            AZ::RPI::MaterialPropertyOutputType::ShaderInput, materialPropertyConnectionName);
                    }
                    else if (AZ::StringFunc::Equal(materialPropertyConnectionType, "ShaderOption"))
                    {
                        property->m_outputConnections.emplace_back(
                            AZ::RPI::MaterialPropertyOutputType::ShaderOption, materialPropertyConnectionName);
                    }
                    else if (AZ::StringFunc::Equal(materialPropertyConnectionType, "InternalProperty"))
                    {
                        property->m_outputConnections.emplace_back(
                            AZ::RPI::MaterialPropertyOutputType::InternalProperty, materialPropertyConnectionName);
                    }
                }
            }
        }

        // Sorting groups and properties in the source data layout to force consistent ordering of the generated material type.
        materialTypeSourceData.SortProperties();

        // The file is written to an in memory buffer before saving to facilitate string substitutions.
        AZStd::string templateOutputText;
        if (!AZ::RPI::JsonUtils::SaveObjectToString(templateOutputText, materialTypeSourceData))
        {
            AZ_Error("MaterialGraphCompiler", false, "Material type template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        // Substitute the material graph name and any other Material Canvas specific tokens
        AZ::StringFunc::Replace(templateOutputText, "MaterialGraphName", GetUniqueGraphName().c_str());

        // Compare against what is already on disk before writing. Rewriting a file with identical content still invalidates the Asset
        // Processor's file state cache and forces it to rehash the source and walk every job that depends on it, so an unchanged material
        // type is worth detecting even though it would not ultimately rebuild anything.
        if (const auto existingText = AZ::Utils::ReadFile(templateOutputPath); existingText.IsSuccess())
        {
            if (existingText.GetValue() == templateOutputText)
            {
                AZ_TracePrintf_IfTrue(
                    "MaterialGraphCompiler",
                    IsCompileLoggingEnabled(),
                    "Generated material type is unchanged, skipping write: %s\n",
                    templateOutputPath.c_str());
                return true;
            }

            if (MaterialTypeTextsDifferOnlyByPropertyValues(existingText.GetValue(), templateOutputText))
            {
                // Only default values moved. The file is still written so that the source on disk always describes the current graph,
                // but the viewport does not have to wait for the result: CompileGraph skips the Asset Processor status wait for this
                // compile and the values collected above are applied directly to the material instance instead.
                AZ_TracePrintf_IfTrue(
                    "MaterialGraphCompiler",
                    IsCompileLoggingEnabled(),
                    "Generated material type differs only by property values: %s\n",
                    templateOutputPath.c_str());
            }
            else
            {
                m_onlyMaterialPropertyValuesChanged = false;
            }
        }
        else
        {
            // No file to compare against, so this is the first time the material type has been generated.
            m_onlyMaterialPropertyValuesChanged = false;
        }

        AZ_TracePrintf_IfTrue(
            "MaterialGraphCompiler", IsCompileLoggingEnabled(), "Saving generated file: %s\n", templateOutputPath.c_str());

        // The material type is complete and can be saved to disk.
        if (IsCancelRequested())
        {
            return false;
        }

        const auto writeOutcome = AZ::Utils::WriteFile(templateOutputText, templateOutputPath);
        if (!writeOutcome)
        {
            AZ_Error("MaterialGraphCompiler", false, "Material type template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        m_wroteAnyGeneratedFile = true;
        return true;
    }

    namespace
    {
        //! Deep comparison of two parsed material types that treats every "defaultValue" member as a wildcard. Everything else, including
        //! the set of property groups, the property names, their declared types and their shader connections, has to match exactly.
        bool JsonEqualIgnoringPropertyValues(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
        {
            static constexpr const char* DefaultValueMemberName = "defaultValue";

            if (lhs.GetType() != rhs.GetType())
            {
                return false;
            }

            if (lhs.IsObject())
            {
                const auto countComparedMembers = [](const rapidjson::Value& value)
                {
                    size_t count = 0;
                    for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
                    {
                        count += (azstricmp(member->name.GetString(), DefaultValueMemberName) != 0) ? 1 : 0;
                    }
                    return count;
                };

                // Counting first catches a member that exists on one side only, which FindMember below cannot see from the other
                // direction.
                if (countComparedMembers(lhs) != countComparedMembers(rhs))
                {
                    return false;
                }

                for (auto member = lhs.MemberBegin(); member != lhs.MemberEnd(); ++member)
                {
                    if (azstricmp(member->name.GetString(), DefaultValueMemberName) == 0)
                    {
                        continue;
                    }

                    const auto other = rhs.FindMember(member->name);
                    if (other == rhs.MemberEnd() || !JsonEqualIgnoringPropertyValues(member->value, other->value))
                    {
                        return false;
                    }
                }

                return true;
            }

            if (lhs.IsArray())
            {
                if (lhs.Size() != rhs.Size())
                {
                    return false;
                }

                for (rapidjson::SizeType index = 0; index < lhs.Size(); ++index)
                {
                    if (!JsonEqualIgnoringPropertyValues(lhs[index], rhs[index]))
                    {
                        return false;
                    }
                }

                return true;
            }

            return lhs == rhs;
        }
    } // namespace

    bool MaterialGraphCompiler::MaterialTypeTextsDifferOnlyByPropertyValues(
        const AZStd::string& existingText, const AZStd::string& newText)
    {
        const auto existingDocument = AZ::JsonSerializationUtils::ReadJsonString(existingText);
        const auto newDocument = AZ::JsonSerializationUtils::ReadJsonString(newText);

        // If either side will not parse, report a structural change. Writing the file and waiting for the Asset Processor is always
        // correct, only slow. Taking the fast path on a comparison that could not be made would show a preview that does not match the
        // graph.
        if (!existingDocument.IsSuccess() || !newDocument.IsSuccess())
        {
            return false;
        }

        return JsonEqualIgnoringPropertyValues(existingDocument.GetValue(), newDocument.GetValue());
    }

    bool MaterialGraphCompiler::ResetMaterialPropertyValueToTypeDefault(AZ::RPI::MaterialPropertyValue& value)
    {
        // Assigning through MaterialPropertyValue's templated operator= keeps the variant on the same alternative, so the property's
        // declared type and the generated parameter struct are unaffected. Anything not listed is left alone and reported as not reset,
        // which is also the answer to whether the generated material can carry the real value: MaterialPropertyValue's remaining
        // alternatives are RHI::SamplerState and Data::Instance<Image>, and neither survives a round trip through a .material.
        if (value.Is<bool>())                                              { value = false; }
        else if (value.Is<int32_t>())                                      { value = static_cast<int32_t>(0); }
        else if (value.Is<uint32_t>())                                     { value = static_cast<uint32_t>(0); }
        else if (value.Is<float>())                                        { value = 0.0f; }
        else if (value.Is<AZ::Vector2>())                                  { value = AZ::Vector2::CreateZero(); }
        else if (value.Is<AZ::Vector3>())                                  { value = AZ::Vector3::CreateZero(); }
        else if (value.Is<AZ::Vector4>())                                  { value = AZ::Vector4::CreateZero(); }
        else if (value.Is<AZ::Color>())                                    { value = AZ::Color(0.0f, 0.0f, 0.0f, 1.0f); }
        else if (value.Is<AZStd::string>())                                { value = AZStd::string{}; }
        else if (value.Is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())         { value = AZ::Data::Asset<AZ::RPI::ImageAsset>{}; }
        else                                                               { return false; }
        return true;
    }

    bool MaterialGraphCompiler::BuildMaterialForCurrentNode()
    {
        for (const auto& templatePath : m_templatePathsForCurrentNode)
        {
            if (IsCancelRequested())
            {
                return false;
            }

            // ".materialtype" does not end with ".material", so this selects only the material templates.
            if (!templatePath.ends_with(".material"))
            {
                continue;
            }

            const auto& templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
            const auto& templateOutputPath = GetOutputPathFromTemplatePath(templateInputPath);

            // The preview material is built in the viewport from the material type, so writing this file produces a source the
            // Asset Processor dutifully builds and nothing ever loads. Measured at 268 ms of Asset Processor time for 28 ms of
            // builder work, every edit. Not writing it removes the job outright rather than merely not waiting for it.
            //
            // An existing one is removed, because a file left behind from before this was enabled keeps its job alive forever.
            if (m_currentOutputSet == OutputSet::Preview && IsInMemoryPreviewMaterialEnabled())
            {
                if (auto fileIO = AZ::IO::FileIOBase::GetInstance(); fileIO && fileIO->Exists(templateOutputPath.c_str()))
                {
                    AZ_TracePrintf_IfTrue(
                        "MaterialGraphCompiler",
                        IsCompileLoggingEnabled(),
                        "Removing preview material, which the viewport now builds itself: %s\n",
                        templateOutputPath.c_str());
                    fileIO->Remove(templateOutputPath.c_str());
                }
                continue;
            }

            if (!BuildMaterialFromTemplate(templateInputPath, templateOutputPath))
            {
                return false;
            }

            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, templateOutputPath);
            m_generatedFiles.push_back(templateOutputPath);
        }
        return true;
    }

    bool MaterialGraphCompiler::BuildMaterialFromTemplate(
        const AZStd::string& templateInputPath, const AZStd::string& templateOutputPath)
    {
        AZ::RPI::MaterialSourceData materialSourceData;
        if (!AZ::RPI::JsonUtils::LoadObjectFromFile(templateInputPath, materialSourceData))
        {
            AZ_Error("MaterialGraphCompiler", false, "Material template could not be loaded: '%s'.", templateInputPath.c_str());
            return false;
        }

        for (const auto& [propertyId, propertyValue] : m_materialPropertyValues)
        {
            materialSourceData.SetPropertyValue(propertyId, propertyValue);
        }

        AZStd::string templateOutputText;
        if (!AZ::RPI::JsonUtils::SaveObjectToString(templateOutputText, materialSourceData))
        {
            AZ_Error("MaterialGraphCompiler", false, "Material template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        AZ::StringFunc::Replace(templateOutputText, "MaterialGraphName", GetUniqueGraphName().c_str());

        // Unchanged content is left alone for the same reason as everywhere else: rewriting it forces the Asset Processor to rehash the
        // source and walk everything that depends on it.
        if (const auto existingText = AZ::Utils::ReadFile(templateOutputPath);
            existingText.IsSuccess() && existingText.GetValue() == templateOutputText)
        {
            return true;
        }

        if (IsCancelRequested())
        {
            return false;
        }

        if (!AZ::Utils::WriteFile(templateOutputText, templateOutputPath))
        {
            AZ_Error("MaterialGraphCompiler", false, "Material template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        // Deliberately does not clear m_onlyMaterialPropertyValuesChanged. Writing the material re-runs the material builder and nothing
        // else, and the viewport already has the values, so the compile still skips the asset status wait.
        m_wroteAnyGeneratedFile = true;
        return true;
    }

    AZStd::string MaterialGraphCompiler::GetUniqueGraphName() const
    {
        return m_templateNodeCount <= 0 ? m_graphName : AZStd::string::format("%s_%03i", m_graphName.c_str(), m_templateNodeCount);
    }
} // namespace MaterialCanvas
