/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvas
{
    //! Variable Data structure for storing mappings of variable names to variable objects
    class VariableData
    {
    public:
        AZ_TYPE_INFO(VariableData, "{4F80659A-CD11-424E-BF04-AF02ABAC06B0}");
        AZ_CLASS_ALLOCATOR(VariableData, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        enum Version : AZ::s32
        {
            InitialVersion = 0,
            UUID_To_Variable,
            VariableDatumSimplification,

            // Should always be last
            Current
        };        

        VariableData() = default;
        VariableData(const VariableData&) = default;
        VariableData& operator=(const VariableData&) = default;
        VariableData(VariableData&&);
        VariableData& operator=(VariableData&&);

        inline GraphVariableMapping& GetVariables() { return m_variableMap; }
        inline const GraphVariableMapping& GetVariables() const { return m_variableMap; }

        AZ::Outcome<VariableId, AZStd::string> AddVariable(AZStd::string_view varName, const GraphVariable& graphVariable);

        // returns GraphVariable* if found otherwise a nullptr is returned
        GraphVariable* FindVariable(AZStd::string_view variableName);
        GraphVariable* FindVariable(VariableId variableId);

        const GraphVariable* FindVariable(AZStd::string_view variableName) const { return const_cast<VariableData*>(this)->FindVariable(variableName); }
        const GraphVariable* FindVariable(VariableId variableId) const { return const_cast<VariableData*>(this)->FindVariable(variableId); }

        void Clear();

        // Remove all variables with supplied name
        size_t RemoveVariable(AZStd::string_view variableName);
        // Remove variable with supplied id
        bool RemoveVariable(const VariableId& variableId);

        // Rename variable with the supplied id
        bool RenameVariable(const VariableId& variableId, AZStd::string_view newVarName);

    private:
        GraphVariableMapping m_variableMap;
    };

    struct EditableVariableConfiguration
    {
    private:
        enum Version : AZ::s32             
        {
            InitialVersion,
            VariableDatumSimplification,
            RemoveUnusedDefaultValue,
            // Should always be last
            Current
        };

    public:
        AZ_TYPE_INFO(EditableVariableConfiguration, "{96D2F031-DEA0-44DF-82FB-2612AFB1DACF}");
        AZ_CLASS_ALLOCATOR(EditableVariableConfiguration, AZ::SystemAllocator);

        static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode);
        static void Reflect(AZ::ReflectContext* context);

        GraphVariable m_graphVariable;
    };

    //! Variable Data structure which uses the VariableNameValuePair struct to provide editor specific UI visualization
    //! for the variables within a graph. It stores uses vector instead of a map to maintain the order for that the variable values
    //! were added
    class EditableVariableData
    {
    public:
        AZ_TYPE_INFO(EditableVariableData, "{D335AEC5-D118-443D-B85C-FEB17C0B26D6}");
        AZ_CLASS_ALLOCATOR(EditableVariableData, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        EditableVariableData();

        // Returns reference to VariableNameValuePair Container
        AZStd::list<EditableVariableConfiguration>& GetVariables() { return m_variables; }
        const AZStd::list<EditableVariableConfiguration>& GetVariables() const { return m_variables; }

        // Adds variable with the supplied name and VariableDatum
        // The VariableId is retrieved from the VariableDatum
        AZ::Outcome<void, AZStd::string> AddVariable(AZStd::string_view varName, const GraphVariable& varDatum);

        // returns the pointer to the specified variable in m_variables. Returns nullptr if not found.
        EditableVariableConfiguration* FindVariable(AZStd::string_view variableName);
        EditableVariableConfiguration* FindVariable(VariableId variableId);

        const EditableVariableConfiguration* FindVariable(AZStd::string_view variableName) const { return const_cast<EditableVariableData*>(this)->FindVariable(variableName); }
        const EditableVariableConfiguration* FindVariable(VariableId variableId) const { return const_cast<EditableVariableData*>(this)->FindVariable(variableId); }

        // Remove all variables
        void Clear();
        // Remove all variables with supplied name
        size_t RemoveVariable(AZStd::string_view variableName);
        // Remove variable with supplied id
        bool RemoveVariable(const VariableId& variableId);

    private:
        AZStd::string m_name;
        AZStd::list<EditableVariableConfiguration> m_variables;
    };
}
