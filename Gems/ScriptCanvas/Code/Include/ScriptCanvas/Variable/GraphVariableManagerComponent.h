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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    // Implements methods to add/remove/find Script Canvas Data objects associated with the Script Canvas graph
    // The VariableRequestBus is address by VariableId
    // The VariableGraphIequestBus bus is addressed using the UniqueId of the ScriptCanvas Graph Component at runtime and editor time
    // (NOTE: this is not the EntityId that the Graph is attached to, but an ID that is tied only to the Graph Component)
    // In addition at Editor time the VariableGraphRequestBus can be address using the EntityId that this component is attached.
    class GraphVariableManagerComponent
        : public AZ::Component
        , protected GraphConfigurationNotificationBus::Handler
        , protected GraphVariableManagerRequestBus::Handler
        , protected VariableRequestBus::MultiHandler
    {
    public:
        AZ_COMPONENT(GraphVariableManagerComponent, "{825DC28D-667D-43D0-AF11-73681351DD2F}");

        static void Reflect(AZ::ReflectContext* context);

        GraphVariableManagerComponent();
        GraphVariableManagerComponent(ScriptCanvasId scriptCanvasId);
        ~GraphVariableManagerComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // GraphConfigurationNotificationBus
        void ConfigureScriptCanvasId(const ScriptCanvasId& scriptCanvasId) override;
        ////

        ScriptCanvasId GetScriptCanvasId() const { return m_scriptCanvasId; }

        //// VariableRequestBus
        GraphVariable* GetVariable() override;
        const GraphVariable* GetVariableConst() const override { return const_cast<GraphVariableManagerComponent*>(this)->GetVariable(); }

        Data::Type GetType() const override;
        AZStd::string_view GetName() const override;
        AZ::Outcome<void, AZStd::string> RenameVariable(AZStd::string_view newVarName) override;

        //// GraphVariableManagerRequestBus
        AZ::Outcome<VariableId, AZStd::string> CloneVariable(const GraphVariable& variableConfiguration) override;
        AZ::Outcome<VariableId, AZStd::string> RemapVariable(const GraphVariable& variableConfiguration) override;
        AZ::Outcome<VariableId, AZStd::string> AddVariable(AZStd::string_view name, const Datum& value) override;
        AZ::Outcome<VariableId, AZStd::string> AddVariablePair(const AZStd::pair<AZStd::string_view, Datum>& nameValuePair) override;

        VariableValidationOutcome IsNameValid(AZStd::string_view key) override;

        bool RemoveVariable(const VariableId& variableId) override;
        AZStd::size_t RemoveVariableByName(AZStd::string_view variableName) override;

        GraphVariable* FindVariable(AZStd::string_view propName) override;
        GraphVariable* FindVariableById(const VariableId& variableId) override;
        GraphVariable* FindFirstVariableWithType(const Data::Type& dataType, const AZStd::unordered_set< ScriptCanvas::VariableId >& blacklistId) override;

        Data::Type GetVariableType(const VariableId& variableId) override;
        
        const GraphVariableMapping* GetVariables() const override;
        AZStd::string_view GetVariableName(const VariableId&) const override;
        AZ::Outcome<void, AZStd::string> RenameVariable(const VariableId&, AZStd::string_view) override;

        bool IsRemappedId(const VariableId& remappedId) const override;
        ////

        GraphVariableMapping* GetVariables();

        const VariableData* GetVariableDataConst() const override { return &m_variableData; }
        VariableData* GetVariableData() override { return &m_variableData; }
        void SetVariableData(const VariableData& variableData) override;
        void DeleteVariableData(const VariableData& variableData) override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasVariableService", 0x819c8460));
        }
        
        void RegisterCopiedVariableRemapping(const VariableId& originalValue, const VariableId& remappedId);
        void UnregisterUncopiedVariableRemapping(const VariableId& remappedId);

        VariableId FindCopiedVariableRemapping(const VariableId& variableId) const;

        VariableData m_variableData;
    private:
        ScriptCanvasId m_scriptCanvasId;

        AZStd::unordered_map< VariableId, VariableId > m_copiedVariableRemapping;
    };
}
