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

        //! Single root, relative to the project, that all preview output is written into. Every graph's preview output goes under here,
        //! mirroring the graph's own folder, rather than into a sibling folder next to each graph. One root means one line in
        //! .gitignore, one rule for the bundler and the reference check, and one folder to delete when the cache has to go.
        //!
        //! The preview and production material types share a file name and are told apart by this root alone, which also keeps their
        //! generated shaders in separate intermediate asset folders.
        static constexpr const char* PreviewOutputRootFolderName = "MaterialCanvasPreview";
        static constexpr const char* PreviewOutputRootRelativePath = "Assets/MaterialCanvasPreview";

        //! Whether @path lies inside the preview output root. This is the one predicate for the question "is this a preview asset",
        //! shared by the viewport, and by anything that later has to refuse a preview asset where a real one was meant.
        static bool IsPreviewOutputPath(AZStd::string_view path);

        //! Whether graphs currently generate a reduced preview output set alongside the production one. Public because the viewport has
        //! to resolve the same choice when deciding which generated material to display.
        static bool IsPreviewOutputEnabled();

        //! Whether the viewport builds the preview material itself instead of waiting for the Asset Processor. When this is on the
        //! preview .material file is not written at all, because nothing reads it, and its builder job therefore never runs.
        static bool IsInMemoryPreviewMaterialEnabled();

        //! Whether the production output is older than the graph it was built from, as of the last compile. False when there is nothing
        //! to compare against, which is every case where preview output is off and the only output is the production one.
        bool IsProductionOutputStale() const override
        {
            return m_productionOutputStale;
        }

        //! The two sets of files a compile can produce from one graph.
        //!
        //! Production is what the rest of the engine consumes: the material type as the graph describes it, built through the project's
        //! normal material pipelines. Preview is the same graph built through MaterialCanvasPreview alone, with the fidelity reductions
        //! that make an edit compile in well under a second, and it exists only to feed the Material Canvas viewport.
        //!
        //! They are separated by folder rather than by content so that an edit cannot degrade a material a level is already using: an
        //! edit writes Preview only, and Production is rewritten when the graph is saved.
        enum class OutputSet
        {
            Production,
            Preview
        };

    private:
        //! The output sets this compile owes, in the order they should be written. Preview comes first when both are due, so the viewport
        //! can resolve its material while the production shaders are still building.
        AZStd::vector<OutputSet> GetOutputSetsForThisCompile() const;

        //! Writes one output set for the current node. Everything before this point in the compile is set independent and is done once.
        bool ExportOutputSetForCurrentNode(const GraphModel::ConstNodePtr& currentNode, OutputSet outputSet);

        //! True while building the output set the viewport displays, which is the only one whose material property values are worth
        //! collecting and sending on.
        bool IsViewportOutputSet() const;

        //! Creates the preview output folder if this compile is about to write into it. No-op for the production set, which is written
        //! beside the graph.
        bool EnsureOutputFolderExists() const;

        //! Removes preview output left behind by an earlier compile once preview output is turned off, so the Asset Processor stops
        //! building preview shaders for a graph that no longer asks for any.
        void DeleteStalePreviewOutputForCurrentNode();

        //! The folder this graph's preview output is written into: the preview root, with the graph's own folder mirrored underneath so
        //! that two graphs with the same file name in different folders cannot write over each other.
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

        //! Returns true when @newText and @existingText describe the same material type except for property default values, which is the
        //! shape every edit to a material input node's value produces. Both are the fully substituted text of a generated material type.
        static bool MaterialTypeTextsDifferOnlyByPropertyValues(
            const AZStd::string& existingText, const AZStd::string& newText);

        //! Replaces @value with a placeholder of the same type, so that the material type's content stops depending on what the graph's
        //! material inputs are currently set to. The real values are written into the generated material instead.
        //!
        //! Returns false, leaving @value alone, for the alternatives that have no placeholder: RHI::SamplerState and
        //! Data::Instance<Image>. That is also the answer to whether the generated material could carry the real value, since a material
        //! stores property values as bare JSON and its serializer infers the alternative from the shape, so callers should record a value
        //! as moved only when this returns true.
        //!
        //! Not for enum properties, which the caller has to exclude: an enum's value is a name out of the property's enumValues list and
        //! that set has no blank member either, but the value itself is an ordinary string that this function cannot tell apart.
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
        // Not const: it records the property values the graph describes and classifies how the generated material type changed, both of
        // which the rest of the compile reads back.
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

        // The output set currently being written. Read by GetOutputPathFromTemplatePath and by the material type builder, which declares
        // the preview material pipeline for one set and not the other.
        OutputSet m_currentOutputSet = OutputSet::Production;

        // Whether the production output is behind the graph, as of the end of the last compile. Read from the UI thread to decide
        // whether there is anything for Apply to do, written by the compile worker, so it is atomic.
        AZStd::atomic_bool m_productionOutputStale = false;

        // Container of paths for template files that need to be evaluated and have products generated for the current node.
        AZStd::set<AZStd::string> m_templatePathsForCurrentNode;

        // Container of template source file data and lines they need to be transformed as part of compiling the graph. 
        AZStd::list<AtomToolsFramework::GraphTemplateFileData> m_templateFileDataVecForCurrentNode;

        // A container of all nodes contributing instructions to the current node
        AZStd::mutex m_instructionNodesForCurrentNodeMutex;
        AZStd::vector<GraphModel::ConstNodePtr> m_instructionNodesForCurrentNode;

        // True if any generated file was actually replaced on disk during this compile. A compile that writes nothing has given the Asset
        // Processor no reason to run, so waiting on it for status would block on jobs that will never be queued.
        bool m_wroteAnyGeneratedFile = false;

        // True while every change made during this compile is confined to material property values in a generated material type. Those are
        // delivered straight to the viewport as property overrides, so the preview does not have to wait for the Asset Processor to rebuild
        // the material type and every shader behind it.
        bool m_onlyMaterialPropertyValuesChanged = true;

        // Every material property value the graph currently describes, gathered while the material types are built and sent over
        // MaterialGraphCompilerNotificationBus once the compile succeeds.
        MaterialGraphCompilerNotifications::PropertyValueList m_materialPropertyValues;
    };
} // namespace MaterialCanvas
