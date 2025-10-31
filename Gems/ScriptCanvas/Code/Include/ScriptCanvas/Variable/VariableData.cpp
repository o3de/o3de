/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Variable/VariableData.h>

// Version Conversion
#include <ScriptCanvas/Deprecated/VariableHelpers.h>
////

namespace ScriptCanvas
{
    /////////////////
    // VariableData
    /////////////////

    static bool VariableDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() < VariableData::Version::UUID_To_Variable)
        {
            AZStd::unordered_map<AZ::Uuid, Deprecated::VariableNameValuePair > uuidToVariableMap;
            if (!rootElementNode.GetChildData(AZ_CRC_CE("m_nameVariableMap"), uuidToVariableMap))
            {
                AZ_Error("Script Canvas", false, "Variable id in version 0 VariableData element should be AZ::Uuid");
                return false;
            }

            rootElementNode.RemoveElementByName(AZ_CRC_CE("m_nameVariableMap"));
            AZStd::unordered_map<VariableId, GraphVariable> idToVariableMap;
            for (auto& uuidToVariableNamePair : uuidToVariableMap)
            {
                idToVariableMap.emplace(uuidToVariableNamePair.first, GraphVariable(AZStd::move(uuidToVariableNamePair.second)));
            }

            rootElementNode.AddElementWithData(context, "m_nameVariableMap", idToVariableMap);            
        }
        else if (rootElementNode.GetVersion() < VariableData::Version::VariableDatumSimplification)
        {
            AZStd::unordered_map<VariableId, Deprecated::VariableNameValuePair> idToPairMap;
            if (!rootElementNode.GetChildData(AZ_CRC_CE("m_nameVariableMap"), idToPairMap))
            {
                return false;
            }

            rootElementNode.RemoveElementByName(AZ_CRC_CE("m_nameVariableMap"));

            AZStd::unordered_map<VariableId, GraphVariable> idToVariableMap;

            for (auto& idPair : idToPairMap)
            {
                idToVariableMap.emplace(idPair.first, GraphVariable(AZStd::move(idPair.second)));
            }

            rootElementNode.AddElementWithData(context, "m_nameVariableMap", idToVariableMap);            
        }

        return true;
    }
    void VariableData::Reflect(AZ::ReflectContext* context)
    {
        Deprecated::VariableNameValuePair::Reflect(context);        

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Version Conversion Reflection
            {
                auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<AZ::Uuid, Deprecated::VariableNameValuePair>>::GetGenericInfo();
                if (genericInfo)
                {
                    genericInfo->Reflect(serializeContext);
                }
            }

            {
                auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<VariableId, Deprecated::VariableNameValuePair>>::GetGenericInfo();
                if (genericInfo)
                {
                    genericInfo->Reflect(serializeContext);
                }
            }
            ////


            serializeContext->Class<VariableData>()
                ->Version(Version::Current, &VariableDataVersionConverter)
                ->Field("m_nameVariableMap", &VariableData::m_variableMap)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<VariableData>("Variables", "Variables exposed by the attached Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Variable Fields")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariableData::m_variableMap, "Variables", "Table of Variables within the Script Canvas Graph")
                    ;
            }
        }
    }

    VariableData::VariableData(VariableData&& other)
        : m_variableMap(AZStd::move(other.m_variableMap))
    {
        other.m_variableMap.clear();
    }

    VariableData& VariableData::operator=(VariableData&& other)
    {
        if (this != &other)
        {
            m_variableMap = AZStd::move(other.m_variableMap);
            other.m_variableMap.clear();
        }

        return *this;
    }

    AZ::Outcome<VariableId, AZStd::string> VariableData::AddVariable(AZStd::string_view varName, const GraphVariable& graphVariable)
    {
        auto insertIt = m_variableMap.emplace(graphVariable.GetVariableId(), graphVariable);

        if (insertIt.second)
        {
            insertIt.first->second.SetVariableName(varName);
            return AZ::Success(insertIt.first->first);
        }

        return AZ::Failure(AZStd::string::format("Variable with id %s already exist in variable map. The Variable name is %s", insertIt.first->first.ToString().c_str(), insertIt.first->second.GetVariableName().data()));
    }

    GraphVariable* VariableData::FindVariable(AZStd::string_view variableName)
    {
        auto foundIt = AZStd::find_if(m_variableMap.begin(), m_variableMap.end(), [&variableName](const AZStd::pair<VariableId, GraphVariable>& variablePair)
        {
            return variableName == variablePair.second.GetVariableName();
        });

        return foundIt != m_variableMap.end() ? &foundIt->second : nullptr;
    }

    GraphVariable* VariableData::FindVariable(VariableId variableId)
    {
        auto foundIt = m_variableMap.find(variableId);
        return foundIt != m_variableMap.end() ? &foundIt->second : nullptr;
    }

    void VariableData::Clear()
    {
        m_variableMap.clear();
    }

    size_t VariableData::RemoveVariable(AZStd::string_view variableName)
    {
        size_t removedVars = 0U;
        for (auto varIt = m_variableMap.begin(); varIt != m_variableMap.end();)
        {
            if (varIt->second.GetVariableName() == variableName)
            {
                ++removedVars;
                varIt = m_variableMap.erase(varIt);
            }
            else
            {
                ++varIt;
            }
        }
        return removedVars;
    }

    bool VariableData::RemoveVariable(const VariableId& variableId)
    {
        return m_variableMap.erase(variableId) != 0;
    }

    bool VariableData::RenameVariable(const VariableId& variableId, AZStd::string_view newVarName)
    {
        auto foundIt = m_variableMap.find(variableId);
        if (foundIt != m_variableMap.end())
        {
            foundIt->second.SetVariableName(newVarName);
            return true;
        }

        return false;
    }

    //////////////////////////////////
    // EditableVariableDataCovnerter
    //////////////////////////////////

    static bool EditableVariableDataConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() <= 1)
        {
            AZStd::list<Deprecated::VariableNameValuePair> varNameValueVariableList;
            if (!rootElementNode.GetChildData(AZ_CRC_CE("m_properties"), varNameValueVariableList))
            {
                AZ_Error("ScriptCanvas", false, "Unable to find m_properties list of VariableNameValuePairs on EditableVariableData version %d", rootElementNode.GetVersion());
                return false;
            }

            AZStd::list<EditableVariableConfiguration> editableVariableConfigurationList;
            for (auto varNameValuePair : varNameValueVariableList)
            {
                editableVariableConfigurationList.push_back({ GraphVariable(AZStd::move(varNameValuePair)),  });
            }

            rootElementNode.RemoveElementByName(AZ_CRC_CE("m_properties"));
            rootElementNode.AddElementWithData(serializeContext, "m_variables", editableVariableConfigurationList);
        }

        return true;
    }

    void EditableVariableData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::list<Deprecated::VariableNameValuePair>>::GetGenericInfo())
            {
                genericClassInfo->Reflect(serializeContext);
            }            

            serializeContext->Class<EditableVariableData>()
                ->Version(2, &EditableVariableDataConverter)
                ->Field("m_variables", &EditableVariableData::m_variables)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditableVariableData>("Variables", "Variables exposed by the attached Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Variable Fields")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableVariableData::m_variables, "Variables", "Array of Variables within Script Canvas Graph")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditableVariableData::EditableVariableData()
    {
    }

    AZ::Outcome<void, AZStd::string> EditableVariableData::AddVariable(AZStd::string_view varName, const GraphVariable& graphVariable)
    {
        if (FindVariable(graphVariable.GetVariableId()))
        {
            return AZ::Failure(AZStd::string::format("Variable %s already exist", varName.data()));
        }

        m_variables.emplace_back();

        EditableVariableConfiguration& newVarConfig = m_variables.back();
        newVarConfig.m_graphVariable.DeepCopy(graphVariable);
        newVarConfig.m_graphVariable.SetVariableName(varName);
        return AZ::Success();
    }

    EditableVariableConfiguration* EditableVariableData::FindVariable(AZStd::string_view variableName)
    {
        auto foundIt = AZStd::find_if(m_variables.begin(), m_variables.end(), [&variableName](const EditableVariableConfiguration& variablePair)
        {
            return variableName == variablePair.m_graphVariable.GetVariableName();
        });

        return foundIt != m_variables.end() ? &*foundIt : nullptr;
    }

    EditableVariableConfiguration* EditableVariableData::FindVariable(VariableId variableId)
    {
        auto foundIt = AZStd::find_if(m_variables.begin(), m_variables.end(), [&variableId](const EditableVariableConfiguration& variablePair)
        {
            return variableId == variablePair.m_graphVariable.GetVariableId();
        });

        return foundIt != m_variables.end() ? &*foundIt : nullptr;
    }

    void EditableVariableData::Clear()
    {
        m_variables.clear();
    }

    size_t EditableVariableData::RemoveVariable(AZStd::string_view variableName)
    {
        size_t removedCount = 0;
        auto removeIt = m_variables.begin();
        while (removeIt != m_variables.end())
        {
            if (removeIt->m_graphVariable.GetVariableName() == variableName)
            {
                ++removedCount;
                removeIt = m_variables.erase(removeIt);
            }
            else
            {
                ++removeIt;
            }
        }

        return removedCount;
    }

    bool EditableVariableData::RemoveVariable(const VariableId& variableId)
    {
        for (auto removeIt = m_variables.begin(); removeIt != m_variables.end(); ++removeIt)
        {
            if (removeIt->m_graphVariable.GetVariableId() == variableId)
            {
                m_variables.erase(removeIt);
                return true;
            }
        }

        return false;
    }

    //////////////////////////////////
    // EditableVariableConfiguration
    //////////////////////////////////

    bool EditableVariableConfiguration::VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() < Version::VariableDatumSimplification)
        {
            Deprecated::VariableNameValuePair varNameValuePair;
            if (!rootElementNode.GetChildData(AZ_CRC_CE("m_variableNameValuePair"), varNameValuePair))
            {
                return false;
            }

            rootElementNode.RemoveElementByName(AZ_CRC_CE("m_variableNameValuePair"));

            GraphVariable variable(AZStd::move(varNameValuePair));

            rootElementNode.AddElementWithData(serializeContext, "GraphVariable", variable);
        }

        if (rootElementNode.GetVersion() < Version::RemoveUnusedDefaultValue)
        {
            rootElementNode.RemoveElementByName(AZ_CRC_CE("m_defaultValue"));
        }

        return true;
    }

    void EditableVariableConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditableVariableConfiguration>()
                ->Version(Version::Current, &EditableVariableConfiguration::VersionConverter)
                ->Field("GraphVariable", &EditableVariableConfiguration::m_graphVariable)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditableVariableConfiguration>("Variable Element", "Represents a mapping of name to value")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditableVariableConfiguration::m_graphVariable, "Name,Value", "Variable Name and value")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }
}
