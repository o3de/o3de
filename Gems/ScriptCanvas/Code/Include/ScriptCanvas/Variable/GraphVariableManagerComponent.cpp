/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Graph.h>

// Version Conversion Maintenance
#include <ScriptCanvas/Deprecated/VariableDatumBase.h>
////

namespace ScriptCanvas
{
    const size_t k_maximumVariableNameSize = 200;
    const char* CopiedVariableData::k_variableKey = "ScriptCanvas::CopiedVariableData";

    bool GraphVariableManagerComponentVersionConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& componentElementNode)
    {
        if (componentElementNode.GetVersion() < 3)
        {
            componentElementNode.RemoveElementByName(AZ_CRC_CE("m_uniqueId"));
        }

        return true;
    }

    GraphVariableManagerComponent::GraphVariableManagerComponent()
    {
    }

    GraphVariableManagerComponent::GraphVariableManagerComponent(ScriptCanvasId scriptCanvasId)        
    {
        ConfigureScriptCanvasId(scriptCanvasId);
    }

    GraphVariableManagerComponent::~GraphVariableManagerComponent()
    {
        GraphVariableManagerRequestBus::Handler::BusDisconnect();
        VariableRequestBus::MultiHandler::BusDisconnect();
    }

    void GraphVariableManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        VariableId::Reflect(context);        
        GraphVariable::Reflect(context);        
        VariableData::Reflect(context);
        EditableVariableConfiguration::Reflect(context);
        EditableVariableData::Reflect(context);        

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CopiedVariableData>()
                ->Version(2)
                ->Field("Mapping", &CopiedVariableData::m_variableMapping)
                ;

            serializeContext->Class<GraphVariableManagerComponent, AZ::Component>()
                ->Version(3, &GraphVariableManagerComponentVersionConverter)
                ->Field("m_variableData", &GraphVariableManagerComponent::m_variableData)                
                ->Field("CopiedVariableRemapping", &GraphVariableManagerComponent::m_copiedVariableRemapping)
                ;
        }
    }

    void GraphVariableManagerComponent::Init()
    {        
        GraphConfigurationNotificationBus::Handler::BusConnect(GetEntityId());        
    }

    void GraphVariableManagerComponent::Activate()
    {
        if (!GraphConfigurationNotificationBus::Handler::BusIsConnectedId(GetEntityId()))
        {
            GraphConfigurationNotificationBus::Handler::BusConnect(GetEntityId());
        }
    }

    void GraphVariableManagerComponent::Deactivate()
    {
        GraphVariableManagerRequestBus::Handler::BusDisconnect();
    }

    void GraphVariableManagerComponent::ConfigureScriptCanvasId(const ScriptCanvasId& scriptCanvasId)
    {
        if (m_scriptCanvasId != scriptCanvasId)
        {
            GraphVariableManagerRequestBus::Handler::BusDisconnect();
            m_scriptCanvasId = scriptCanvasId;

            if (m_scriptCanvasId.IsValid())
            {
                GraphVariableManagerRequestBus::Handler::BusConnect(m_scriptCanvasId);
            }

            for (auto& varPair : m_variableData.GetVariables())
            {
                varPair.second.SetOwningScriptCanvasId(m_scriptCanvasId);
                VariableRequestBus::MultiHandler::BusConnect(varPair.second.GetGraphScopedId());
            }
        }
    }

    GraphVariable* GraphVariableManagerComponent::GetVariable()
    {
        GraphVariable* graphVariable = nullptr;

        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            graphVariable = m_variableData.FindVariable(variableId->m_identifier);
        }

        return graphVariable;
    }

    Data::Type GraphVariableManagerComponent::GetType() const
    {
        const GraphVariable* graphVariable = nullptr;
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            graphVariable = m_variableData.FindVariable(variableId->m_identifier);
        }

        return graphVariable ? graphVariable->GetDatum()->GetType() : Data::Type::Invalid();
    }

    AZStd::string_view GraphVariableManagerComponent::GetName() const
    {
        const GraphVariable* namedVariable = nullptr;
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            namedVariable = m_variableData.FindVariable(variableId->m_identifier);
        }

        return namedVariable ? AZStd::string_view(namedVariable->GetVariableName()) : "";
    }

    AZ::Outcome<void, AZStd::string> GraphVariableManagerComponent::RenameVariable(AZStd::string_view newVarName)
    {
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            return RenameVariable(variableId->m_identifier, newVarName);
        }

        return AZ::Failure(AZStd::string::format("No variable id was found, cannot rename variable to %s", newVarName.data()));
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::CloneVariable(const GraphVariable& variableConfiguration)
    {
        GraphVariable copyConfiguration = variableConfiguration;
        copyConfiguration.GenerateNewId();
        copyConfiguration.SetOwningScriptCanvasId(m_scriptCanvasId);

        AZStd::string variableName = copyConfiguration.GetVariableName();

        if (FindVariable(variableName))
        {
            variableName.append(" (Copy)");

            if (FindVariable(variableName))
            {
                AZStd::string originalName = variableName;

                int counter = 0;

                do
                {
                    ++counter;
                    variableName = AZStd::string::format("%s (%i)", originalName.c_str(), counter);
                } while (FindVariable(variableName));
            }
        }

        auto addOutcome = m_variableData.AddVariable(variableName, copyConfiguration);

        if (!addOutcome)
        {
            return addOutcome;
        }

        const VariableId& newId = addOutcome.GetValue();
        VariableRequestBus::MultiHandler::BusConnect(GraphScopedVariableId(m_scriptCanvasId, newId));
        GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, newId, variableName);

        return AZ::Success(newId);
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::RemapVariable(const GraphVariable& graphVariable)
    {        
        if (FindVariableById(graphVariable.GetVariableId()))
        {
            return AZ::Success(graphVariable.GetVariableId());
        }

        ScriptCanvas::VariableId remappedId = FindCopiedVariableRemapping(graphVariable.GetVariableId());

        if (remappedId.IsValid())
        {
            return AZ::Success(remappedId);
        }

        auto cloneOutcome = CloneVariable(graphVariable);

        if (!cloneOutcome)
        {
            return cloneOutcome;
        }

        const VariableId& newId = cloneOutcome.GetValue();

        // Only register a copied variable if it had a valid datum previously.
        if (graphVariable.GetVariableId().IsValid())
        {
            RegisterCopiedVariableRemapping(graphVariable.GetVariableId(), newId);
        }

        return AZ::Success(newId);
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::AddVariable(AZStd::string_view name, const Datum& value, bool functionScope)
    {
        if (FindVariable(name))
        {
            return AZ::Failure(AZStd::string::format("Variable %s already exists", name.data()));
        }

        GraphVariable newVariable(value);
        newVariable.SetOwningScriptCanvasId(m_scriptCanvasId);

        auto addVariableOutcome = m_variableData.AddVariable(name, newVariable);
        if (!addVariableOutcome)
        {
            return addVariableOutcome;
        }        

        const VariableId& newId = addVariableOutcome.GetValue();

        GraphVariable* variable = m_variableData.FindVariable(newId);
        variable->SetOwningScriptCanvasId(GetScriptCanvasId());
        if (functionScope)
        {
            variable->SetScope(VariableFlags::Scope::FunctionReadOnly);
        }

        VariableRequestBus::MultiHandler::BusConnect(GraphScopedVariableId(m_scriptCanvasId, newId));
        GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, newId, name);

        return AZ::Success(newId);
    }

    AZ::Outcome<VariableId, AZStd::string> GraphVariableManagerComponent::AddVariablePair(const AZStd::pair<AZStd::string_view, Datum>& keyValuePair)
    {
        return AddVariable(keyValuePair.first, keyValuePair.second, false);
    }

    VariableValidationOutcome GraphVariableManagerComponent::IsNameValid(AZStd::string_view varName)
    {
        if (varName.size() == 0 || varName.size() >= k_maximumVariableNameSize)
        {
            return AZ::Failure(GraphVariableValidationErrorCode::Invalid);
        }
        else if (FindVariable(varName) != nullptr)
        {
            return AZ::Failure(GraphVariableValidationErrorCode::Duplicate);
        }
        else
        {
            return AZ::Success();
        }
    }

    bool GraphVariableManagerComponent::RemoveVariable(const VariableId& variableId)
    {
        auto varNamePair = m_variableData.FindVariable(variableId);

        if (varNamePair)
        {
            VariableRequestBus::MultiHandler::BusDisconnect(GraphScopedVariableId(m_scriptCanvasId, variableId));
            VariableNotificationBus::Event(GraphScopedVariableId(m_scriptCanvasId, variableId), &VariableNotifications::OnVariableRemoved);
            GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableRemovedFromGraph, variableId, varNamePair->GetVariableName());

            // Bookkeeping for the copied Variable remapping
            UnregisterUncopiedVariableRemapping(variableId);

            return m_variableData.RemoveVariable(variableId);
        }

        return false;
    }

    AZStd::size_t GraphVariableManagerComponent::RemoveVariableByName(AZStd::string_view varName)
    {
        AZStd::size_t removedVars = 0U;
        for (auto varIt = m_variableData.GetVariables().begin(); varIt != m_variableData.GetVariables().end();)
        {
            if (varIt->second.GetVariableName() == varName)
            {
                ScriptCanvas::VariableId variableId = varIt->first;

                // Bookkeeping for the copied Variable remapping
                UnregisterUncopiedVariableRemapping(variableId);

                ++removedVars;
                VariableRequestBus::MultiHandler::BusDisconnect(GraphScopedVariableId(m_scriptCanvasId, variableId));

                VariableNotificationBus::Event(GraphScopedVariableId(m_scriptCanvasId, variableId), &VariableNotifications::OnVariableRemoved);
                GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableRemovedFromGraph, variableId, varName);
                varIt = m_variableData.GetVariables().erase(varIt);
            }
            else
            {
                ++varIt;
            }
        }
        return removedVars;
    }

    GraphVariable* GraphVariableManagerComponent::FindVariable(AZStd::string_view varName)
    {
        return m_variableData.FindVariable(varName);
    }

    GraphVariable* GraphVariableManagerComponent::FindFirstVariableWithType(const Data::Type& dataType, const AZStd::unordered_set< ScriptCanvas::VariableId >& excludedVariableIds)
    {
        for (auto& variablePair : m_variableData.GetVariables())
        {
            if (variablePair.second.GetDataType().IS_A(dataType))
            {
                if (excludedVariableIds.count(variablePair.first) == 0)
                {
                    return &variablePair.second;
                }
            }
        }

        return nullptr;
    }

    GraphVariable* GraphVariableManagerComponent::FindVariableById(const VariableId& variableId)
    {
        return m_variableData.FindVariable(variableId);
    }

    Data::Type GraphVariableManagerComponent::GetVariableType(const VariableId& variableId)
    {
        auto graphVariable = FindVariableById(variableId);
        return graphVariable ? graphVariable->GetDatum()->GetType() : Data::Type::Invalid();
    }

    const GraphVariableMapping* GraphVariableManagerComponent::GetVariables() const
    {
        return &m_variableData.GetVariables();
    }

    GraphVariableMapping* GraphVariableManagerComponent::GetVariables()
    {
        return &m_variableData.GetVariables();
    }

    AZStd::string_view GraphVariableManagerComponent::GetVariableName(const VariableId& variableId) const
    {
        auto foundIt = m_variableData.GetVariables().find(variableId);

        return foundIt != m_variableData.GetVariables().end() ? foundIt->second.GetVariableName() : AZStd::string_view();
    }

    AZ::Outcome<void, AZStd::string> GraphVariableManagerComponent::RenameVariable(const VariableId& variableId, AZStd::string_view newVarName)
    {
        auto varDatumPair = FindVariableById(variableId);

        if (!varDatumPair)
        {
            return AZ::Failure(AZStd::string::format("Unable to find variable with Id %s on Entity %s. Cannot rename",
                variableId.ToString().data(), GetEntityId().ToString().data()));
        }        

        GraphVariable* graphVariable = FindVariable(newVarName);
        if (graphVariable && graphVariable->GetVariableId() != variableId)
        {
            return AZ::Failure(AZStd::string::format("A variable with name %s already exists on Entity %s. Cannot rename",
                newVarName.data(), GetEntityId().ToString().data()));
        }

        if (!IsNameValid(newVarName))
        {
            return AZ::Failure(AZStd::string::format("%s is an invalid variable name. Cannot Rename", newVarName.data()));
        }

        if (!m_variableData.RenameVariable(variableId, newVarName))
        {
            return AZ::Failure(AZStd::string::format("Unable to rename variable with id %s to %s.",
                variableId.ToString().data(), newVarName.data()));
        }

        GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableNameChangedInGraph, variableId, newVarName);
        VariableNotificationBus::Event(GraphScopedVariableId(m_scriptCanvasId, variableId), &VariableNotifications::OnVariableRenamed, newVarName);

        return AZ::Success();
    }

    bool GraphVariableManagerComponent::IsRemappedId(const VariableId& sourceId) const
    {
        VariableId remappedId = FindCopiedVariableRemapping(sourceId);

        return remappedId.IsValid();
    }

    void GraphVariableManagerComponent::SetVariableData(const VariableData& variableData)
    {
        VariableRequestBus::MultiHandler::BusDisconnect();
        DeleteVariableData(m_variableData);

        GraphVariableMapping& variableMapping = m_variableData.GetVariables();

        for (const auto& varPair : variableData.GetVariables())
        {
            variableMapping.emplace(varPair.first, varPair.second);
        }

        for (auto& varPair : variableMapping)
        {
            varPair.second.SetOwningScriptCanvasId(GetScriptCanvasId());

            VariableRequestBus::MultiHandler::BusConnect(varPair.second.GetGraphScopedId());

            if (GetEntity())
            {
                GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableAddedToGraph, varPair.first, varPair.second.GetVariableName());
            }
        }

        if (GetEntity())
        {
            GraphVariableManagerNotificationBus::Event(GetScriptCanvasId(), &GraphVariableManagerNotifications::OnVariableDataSet);
        }
    }

    void GraphVariableManagerComponent::DeleteVariableData(const VariableData& variableData)
    {
        // Temporary vector to store the VariableIds in case the &variableData == &m_variableData
        AZStd::vector<VariableId> variableIds;
        variableIds.reserve(variableData.GetVariables().size());

        for (const auto& varPair : variableData.GetVariables())
        {
            variableIds.push_back(varPair.first);
        }
        for (const auto& variableId : variableIds)
        {
            RemoveVariable(variableId);
        }
    }

    VariableId GraphVariableManagerComponent::FindCopiedVariableRemapping(const VariableId& variableId) const
    {
        VariableId retVal;

        auto mapIter = m_copiedVariableRemapping.find(variableId);

        if (mapIter != m_copiedVariableRemapping.end())
        {
            retVal = mapIter->second;
        }

        return retVal;
    }

    void GraphVariableManagerComponent::RegisterCopiedVariableRemapping(const VariableId& originalValue, const VariableId& remappedId)
    {
        AZ_Error("ScriptCanvas", m_copiedVariableRemapping.find(originalValue) == m_copiedVariableRemapping.end(), "GraphVariableManagerComponent is trying to remap an original value twice");
        m_copiedVariableRemapping[originalValue] = remappedId;
    }

    void GraphVariableManagerComponent::UnregisterUncopiedVariableRemapping(const VariableId& remappedId)
    {
        auto eraseIter = AZStd::find_if(m_copiedVariableRemapping.begin(), m_copiedVariableRemapping.end(), [remappedId](const AZStd::pair<VariableId, VariableId>& otherId) { return remappedId == otherId.second; });

        if (eraseIter != m_copiedVariableRemapping.end())
        {
            m_copiedVariableRemapping.erase(eraseIter);
        }
    }
}
