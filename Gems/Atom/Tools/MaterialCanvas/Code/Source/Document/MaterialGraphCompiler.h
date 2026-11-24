/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphTemplateFileDataCacheRequestBus.h>
#include <Document/MaterialGraphCompilerNotificationBus.h>
#include <GraphModel/Model/Node.h>

namespace MaterialCanvas
{
    //! MaterialGraphCompiler traverses a material graph, searching for and splicing shader code snippets, variable values and definitions,
    //! and other information into complete, functional material types, materials, and shaders. Currently, the resulting files will be
    //! generated an output into the same folder location has the source graph.
    class MaterialGraphCompiler : public AtomToolsFramework::GraphCompiler
    {
    public:
        AZ_RTTI(MaterialGraphCompiler, "{570E3923-48C4-4B91-BC44-3145BE771E9B}", AtomToolsFramework::GraphCompiler);
        AZ_CLASS_ALLOCATOR(MaterialGraphCompiler, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(MaterialGraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        MaterialGraphCompiler() = default;
        MaterialGraphCompiler(const AZ::Crc32& toolId);
        virtual ~MaterialGraphCompiler();

        // AtomToolsFramework::GraphCompiler overrides...
        AZStd::string GetGraphPath() const override;
        bool CompileGraph(GraphModel::GraphPtr graph, const AZStd::string& graphName, const AZStd::string& graphPath) override;
        bool ShouldReportGeneratedFileStatus(const AZStd::string& generatedFile) const override;

        //! Project-relative root for all preview output, mirroring each graph's folder; one root to ignore, bundle-exclude or delete.
        static constexpr const char* PreviewOutputRootFolderName = "MaterialCanvasPreview";
        static constexpr const char* PreviewOutputRootRelativePath = "Assets/MaterialCanvasPreview";

        //! Whether @path is inside the preview output root: the single "is this a preview asset" predicate.
        static bool IsPreviewOutputPath(AZStd::string_view path);

        //! Whether graphs also generate a reduced preview output set; public so the viewport picks the same material.
        static bool IsPreviewOutputEnabled();

        //! Whether the viewport builds the preview material itself; if so the preview .material isn't written and its job never runs.
        static bool IsInMemoryPreviewMaterialEnabled();

        //! Whether production output is older than its graph as of the last compile; false when there is no preview to compare with.
        bool IsProductionOutputStale() const override
        {
            return m_productionOutputStale;
        }

        //! Output sets per graph: Production for the engine, Preview (MaterialCanvasPreview, reduced fidelity) for the viewport only.
        enum class OutputSet
        {
            Production,
            Preview
        };

    private:
        //! Output sets this compile owes, in write order; Preview first so the viewport resolves while production shaders build.
        AZStd::vector<OutputSet> GetOutputSetsForThisCompile() const;

        //! Writes one output set for the current node. Everything before this point in the compile is set independent and is done once.
        bool ExportOutputSetForCurrentNode(const GraphModel::ConstNodePtr& currentNode, OutputSet outputSet);

        //! True while building the set the viewport displays, the only one whose property values are collected.
        bool IsViewportOutputSet() const;

        //! Creates the preview output folder if this compile writes into it; no-op for the production set.
        bool EnsureOutputFolderExists() const;

        //! Removes old preview output once preview output is off, so the Asset Processor stops building it.
        void DeleteStalePreviewOutputForCurrentNode();

        //! This graph's preview folder: the preview root with the graph's folder mirrored so same-named graphs don't collide.
        AZStd::string GetPreviewOutputFolderForGraph() const;

        //! Compares the preview and production output of the current node and records whether production has fallen behind.
        void RecordProductionOutputStaleness();

        void BuildSlotValueTable();
        void BuildDependencyTables();
        void BuildTemplatePathsForCurrentNode(const GraphModel::ConstNodePtr& currentNode);
        bool LoadTemplatesForCurrentNode();
        void DeleteExistingFilesForCurrentNode();
        void ClearFingerprintsForCurrentNode();
        void PreprocessTemplatesForCurrentNode();
        void BuildInstructionsForCurrentNode(const GraphModel::ConstNodePtr& currentNode);
        //! @deprecated Material SRG members are created by the Material-Pipeline.
        void BuildMaterialSrgForCurrentNode();
        bool BuildMaterialTypeForCurrentNode(const GraphModel::ConstNodePtr& currentNode);
        bool ExportTemplatesMatchingRegex(const AZStd::string& pattern);

        //! True when the two fully substituted material type texts differ only in property default values.
        static bool MaterialTypeTextsDifferOnlyByPropertyValues(
            const AZStd::string& existingText, const AZStd::string& newText);

        //! Replaces @value with a same-type placeholder; returns false (untouched) for samplers and images. Callers must exclude enums.
        static bool ResetMaterialPropertyValueToTypeDefault(AZ::RPI::MaterialPropertyValue& value);

        //! Generates the .material files for the current template node, carrying the graph's material input values as property overrides.
        bool BuildMaterialForCurrentNode();
        bool BuildMaterialFromTemplate(const AZStd::string& templateInputPath, const AZStd::string& templateOutputPath);

        // Convert the template file path into a save file path based on the document name, for the output set currently being written.
        AZStd::string GetOutputPathFromTemplatePath(const AZStd::string& templatePath) const;

        // As above, for a named output set rather than the current one.
        AZStd::string GetOutputPathFromTemplatePath(const AZStd::string& templatePath, OutputSet outputSet) const;

        // Functions assisting with conversions between different vector and scalar types. Functions like these will eventually be moved out
        // of the document class so that they can be registered more flexibly and extensively.
        unsigned int GetVectorSize(const AZStd::any& slotValue) const;
        AZStd::any ConvertToScalar(const AZStd::any& slotValue) const;

        template<typename T>
        AZStd::any ConvertToVector(const AZStd::any& slotValue) const;
        AZStd::any ConvertToVector(const AZStd::any& slotValue, unsigned int score) const;

        // Returns the value of the slot or the slots incoming connection if present.
        AZStd::any GetValueFromSlot(GraphModel::ConstSlotPtr slot) const;

        // Returns the value for the corresponding slot or the slot providing its input, if connected.
        AZStd::any GetValueFromSlotOrConnection(GraphModel::ConstSlotPtr slot) const;

        // Convert special slot type names, like color, into one compatible with AZSL shader code.
        AZStd::string GetAzslTypeFromSlot(GraphModel::ConstSlotPtr slot) const;

        // Convert a stored slot value into a string representation that can be injected into AZSL shader code.
        AZStd::string GetAzslValueFromSlot(GraphModel::ConstSlotPtr slot) const;

        // Generate AZSL to insert/substitute members in the material SRG definition. The code for most data types is relatively small and
        // can be entered manually but SamplerState and other data types with several members need additional Handling transform the data
        // into the required format.
        AZStd::string GetAzslSrgMemberFromSlot(
            GraphModel::ConstNodePtr node, const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig) const;

        // Creates a table of strings to search for and the values to replace them with for a specific node.
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> GetSubstitutionSymbolsFromNode(GraphModel::ConstNodePtr node) const;

        // Collect instructions from a slot and perform substitutions based on node and slot types, names, values, and connections.
        AZStd::vector<AZStd::string> GetInstructionsFromSlot(
            GraphModel::ConstNodePtr node,
            const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
            const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const;

        // Determine if instructions contained on an input node should be used as part of code generation based on node connections.
        bool ShouldUseInstructionsFromInputNode(
            GraphModel::ConstNodePtr outputNode,
            GraphModel::ConstNodePtr inputNode,
            const AZStd::vector<AZStd::string>& inputSlotNames) const;

        // Build a list of all graph nodes sorted in execution order based on depth
        AZStd::vector<GraphModel::ConstNodePtr> GetAllNodesInExecutionOrder() const;

        // Build a list of all graph nodes That feed into specific slots an output node, sorted in execution order based on depth
        AZStd::vector<GraphModel::ConstNodePtr> GetInstructionNodesInExecutionOrder(
            GraphModel::ConstNodePtr outputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const;

        // Generate AZSL instructions for an output node by evaluating all of the sorted graph nodes for connections to input slots
        AZStd::vector<AZStd::string> GetInstructionsFromConnectedNodes(
            GraphModel::ConstNodePtr outputNode,
            const AZStd::vector<AZStd::string>& inputSlotNames,
            AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const;

        // Create a unique string identifier, from a node title and ID, that can be used for a file name or symbol in code
        AZStd::string GetSymbolNameFromNode(GraphModel::ConstNodePtr node) const;

        // Create a unique string identifier, from the node symbol name and slot title, that can be used as a variable name in code
        AZStd::string GetSymbolNameFromSlot(GraphModel::ConstSlotPtr slot) const;

        // Convert a material input node into AZSL lines of variables that can be injected into the material SRG
        AZStd::vector<AZStd::string> GetMaterialPropertySrgMemberFromSlot(
            GraphModel::ConstNodePtr node,
            const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
            const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const;

        // Convert all material input nodes into AZSL lines of variables that can be injected into the material SRG
        AZStd::vector<AZStd::string> GetMaterialPropertySrgMemberFromNodes(const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const;

        // Creates and exports a material type source file by loading an existing template, replacing special tokens, and injecting
        // properties defined in material input nodes.
        // Not const: records the graph's property values and how the generated material type changed, for the rest of the compile.
        bool BuildMaterialTypeFromTemplate(
            GraphModel::ConstNodePtr templateNode,
            const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes,
            const AZStd::string& templateInputPath,
            const AZStd::string& templateOutputPath);

        // Returns the name that will be used to replace material graph name during any substitutions 
        AZStd::string GetUniqueGraphName() const;

        // All slots and nodes will be visited to collect all of the unique include paths.
        AZStd::set<AZStd::string> m_includePaths;

        // There's probably no reason to distinguish between function and class definitions.
        // This could really be any globally defined function, class, struct, define.
        AZStd::vector<AZStd::string> m_classDefinitions;
        AZStd::vector<AZStd::string> m_functionDefinitions;

        // Container of unique node configurations IDs visited on the graph to collect include paths, class definitions, and function definitions.
        AZStd::unordered_set<AZ::Uuid> m_configIdsVisited;

        // Table of values for every slot, on every node, including values redirected from incoming connections, and values upgraded to
        // match types and sizes of values on related slots.
        AZStd::map<GraphModel::ConstSlotPtr, AZStd::any> m_slotValueTable;

        // This counter will be used as a suffix for graph name substitutions in case multiple template nodes are included in the same graph
        int m_templateNodeCount = 0;

        // The output set being written; read by GetOutputPathFromTemplatePath and by the material type builder.
        OutputSet m_currentOutputSet = OutputSet::Production;

        // Whether production output is behind the graph; written by the compile worker and read by the UI, hence atomic.
        AZStd::atomic_bool m_productionOutputStale = false;

        // Container of paths for template files that need to be evaluated and have products generated for the current node.
        AZStd::set<AZStd::string> m_templatePathsForCurrentNode;

        // Container of template source file data and lines they need to be transformed as part of compiling the graph. 
        AZStd::list<AtomToolsFramework::GraphTemplateFileData> m_templateFileDataVecForCurrentNode;

        // A container of all nodes contributing instructions to the current node
        AZStd::mutex m_instructionNodesForCurrentNodeMutex;
        AZStd::vector<GraphModel::ConstNodePtr> m_instructionNodesForCurrentNode;

        // True if any generated file was actually replaced; if none were, no AP jobs will be queued, so don't wait.
        bool m_wroteAnyGeneratedFile = false;

        // Generated files whose content actually changed this compile; only these are worth waiting on.
        AZStd::set<AZStd::string> m_writtenGeneratedFiles;

        // True while every change is confined to property values, which go straight to the viewport without an AP rebuild.
        bool m_onlyMaterialPropertyValuesChanged = true;

        // Every property value the graph describes, sent over MaterialGraphCompilerNotificationBus when the compile succeeds.
        MaterialGraphCompilerNotifications::PropertyValueList m_materialPropertyValues;
    };
} // namespace MaterialCanvas
