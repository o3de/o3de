/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/GraphCompiler.h>
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
        AZ_CLASS_ALLOCATOR(MaterialGraphCompiler, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(MaterialGraphCompiler);

        static void Reflect(AZ::ReflectContext* context);

        MaterialGraphCompiler() = default;
        MaterialGraphCompiler(const AZ::Crc32& toolId, const AZ::Uuid& documentId);
        virtual ~MaterialGraphCompiler();

        // AtomToolsFramework::GraphCompiler overrides...
        AZStd::string GetGraphPath() const override;
        bool CompileGraph() override;

    private:
        // Convert the template file path into a save file path based on the document name.
        AZStd::string GetOutputPathFromTemplatePath(const AZStd::string& templatePath) const;

        // Find and replace a whole word or symbol using regular expressions.
        void ReplaceSymbolsInContainer(
            const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container) const;

        void ReplaceSymbolsInContainer(
            const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols,
            AZStd::vector<AZStd::string>& container) const;

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
        AZStd::vector<AZStd::string> GetMaterialInputsFromSlot(
            GraphModel::ConstNodePtr node,
            const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
            const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const;

        // Convert all material input nodes into AZSL lines of variables that can be injected into the material SRG
        AZStd::vector<AZStd::string> GetMaterialInputsFromNodes(const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const;

        // Creates and exports a material type source file by loading an existing template, replacing special tokens, and injecting
        // properties defined in material input nodes
        bool BuildMaterialTypeFromTemplate(
            GraphModel::ConstNodePtr templateNode,
            const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes,
            const AZStd::string& templateInputPath,
            const AZStd::string& templateOutputPath) const;

        void BuildSlotValueTable();

        AZStd::map<GraphModel::ConstSlotPtr, AZStd::any> m_slotValueTable;

        // Utility type wrapping repeated load and save logic for most template files that only require basic insertions and substitutions.
        // Files will be read in and then tokenized into a vector of strings for each line in the file. This allows for easier and
        // individual processing of each line.
        struct TemplateFileData
        {
            AZStd::string m_inputPath;
            AZStd::string m_outputPath;
            AZStd::vector<AZStd::string> m_lines;

            bool Load();
            bool Save() const;

            using LineGenerationFn = AZStd::function<AZStd::vector<AZStd::string>(const AZStd::string&)>;

            // Search for marked up blocks of text from a template and replace lines between them with lines provided by a function.
            void ReplaceLinesInBlock(
                const AZStd::string& blockBeginToken, const AZStd::string& blockEndToken, const LineGenerationFn& lineGenerationFn);
        };
    };
} // namespace MaterialCanvas
