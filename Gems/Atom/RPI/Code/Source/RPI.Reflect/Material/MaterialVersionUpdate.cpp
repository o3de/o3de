/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/Trace.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialVersionUpdate::Action::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialVersionUpdate::Action::ArgsMap>();

                serializeContext->Class<MaterialVersionUpdate::Action>()
                    ->Version(2) // Generic actions based on string -> MaterialPropertyValue map
                    ->Field("ArgsMap", &Action::m_argsMap)
                    ->Field("Operation", &Action::m_operation)
                    ;
            }
        }

        void MaterialVersionUpdate::Reflect(ReflectContext* context)
        {
            MaterialVersionUpdate::Action::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialVersionUpdate::Actions>();

                serializeContext->Class<MaterialVersionUpdate>()
                    ->Version(2) // Generic actions based on string -> MaterialPropertyValue map
                    ->Field("ToVersion", &MaterialVersionUpdate::m_toVersion)
                    ->Field("Actions", &MaterialVersionUpdate::m_actions)
                    ;
            }
        }

        MaterialVersionUpdate::MaterialVersionUpdate(uint32_t toVersion)
            : m_toVersion(toVersion)
        {
        }

        uint32_t MaterialVersionUpdate::GetVersion() const
        {
            return m_toVersion;
        }

        void MaterialVersionUpdate::SetVersion(uint32_t toVersion)
        {
            m_toVersion = toVersion;
        }

        bool MaterialVersionUpdate::Action::HasExpectedNumArguments(
            size_t expectedNum, const char* expectedArgs, AZStd::function<void(const char*)> onError) const
        {
            if (expectedNum != GetNumArgs())
            {
                onError(AZStd::string::format(
                            "Expected %zu arguments in '%s' version update (%s), but found %zu",
                            expectedNum, m_operation.GetCStr(), expectedArgs, GetNumArgs())
                            .data());
                return false;
            }
            return true;
        }
        template <typename T>
        bool MaterialVersionUpdate::Action::HasExpectedArgument(
            const char* expectedArgName, const char* T_str, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue val = GetArg(AZ::Name{ expectedArgName });
            if (!val.IsValid() || !val.Is<T>())
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s' of type %s", expectedArgName, m_operation.GetCStr(), T_str)
                            .data());
                return false;
            }
            return true;
        }
        bool MaterialVersionUpdate::Action::HasExpectedArgumentAnyType(
            const char* expectedArgName, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue val = GetArg(AZ::Name{ expectedArgName });
            if (!val.IsValid())
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s'", expectedArgName, m_operation.GetCStr())
                            .data());
                return false;
            }
            return true;
        }

        bool MaterialVersionUpdate::ValidateActions(
            const MaterialPropertiesLayout* materialPropertiesLayout, AZStd::function<void(const char*)> onError) const
        {
            for (const auto& action : m_actions)
            {
                bool error = false;
                if (action.m_operation == AZ::Name("rename"))
                {
                    error = error || !action.HasExpectedNumArguments(2, "'from', 'to'", onError);
                    error = error || !action.HasExpectedArgument<AZStd::string>("from", "AZStd::string", onError);
                    error = error || !action.HasExpectedArgument<AZStd::string>("to", "AZStd::string", onError);
                }
                else if (action.m_operation == AZ::Name("setValue"))
                {
                    error = error || !action.HasExpectedNumArguments(2, "'name', 'value'", onError);
                    error = error || !action.HasExpectedArgument<AZStd::string>("name", "AZStd::string", onError);
                    error = error || !action.HasExpectedArgumentAnyType("value", onError);
                    if (error)
                    {
                        return false;
                    }
                    const AZ::Name nameToSet = AZ::Name(action.GetArg(AZ::Name("name")).GetValue<AZStd::string>());
                    const auto propertyIndex = materialPropertiesLayout->FindPropertyIndex(nameToSet);
                    if (propertyIndex.IsNull())
                    {
                        onError(AZStd::string::format(
                                    "setValue material version update: Could not find property '%s' in the material properties layout",
                                    nameToSet.GetCStr()).data());
                        return false;
                    }
                    // Check given type with the correct type ID for that property
                    const MaterialPropertyDescriptor *descriptor = materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);
                    const MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                    if (!ValidateMaterialPropertyDataType(
                            valueToSet.GetTypeId(), nameToSet, descriptor,
                            [&](const char* message)
                            {
                                onError(AZStd::string::format("setValue material version update: Received value of wrong type: %s", message)
                                            .data());
                            }))
                    {
                        return false;
                    }
                }
                else if (action.m_operation.IsEmpty())
                {
                    onError(AZStd::string::format(
                                "Material version update action was not properly initialized: empty operation").data());
                    error = true;
                }
                else
                {
                    onError(AZStd::string::format(
                                "Unknown operation '%s' in material version update action",
                                action.m_operation.GetCStr()).data());
                    error = true;
                }

                if (error)
                {
                    return false;
                }
            }

            return true;
        }

        bool MaterialVersionUpdate::ApplyPropertyRenames(AZ::Name& propertyId) const
        {
            bool renamed = false;

            for (const auto& action : m_actions)
            {
                if (action.m_operation != AZ::Name("rename"))
                {
                    continue;
                }

                const AZ::Name from = AZ::Name(action.GetArg(AZ::Name{ "from" }).GetValue<AZStd::string>());
                if (propertyId == from)
                {
                    propertyId = AZ::Name(action.GetArg(AZ::Name{ "to" }).GetValue<AZStd::string>());
                    renamed = true;
                }
            }

            return renamed;
        }

        bool MaterialVersionUpdate::ApplySetValues(
                AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& rawProperties) const
        {
            bool valueWasSet = false;
            for (const auto& action : m_actions)
            {
                if (action.m_operation != AZ::Name("setValue"))
                {
                    continue;
                }

                const AZ::Name nameToSet = AZ::Name(action.GetArg(AZ::Name{ "name" }).GetValue<AZStd::string>());
                const MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));

                // Check if property exists, in which case we overwrite its value
                bool propertyFound = false;
                for (auto& [name, value] : rawProperties)
                {
                    if (name == nameToSet)
                    {
                        value = valueToSet;

                        AZ_WarningOnce("MaterialVersionUpdate", !propertyFound, "Found property %s more than once!", name.GetCStr());
                        propertyFound = true;
                    }
                }
                if (!propertyFound)
                {
                    // Property did not exist yet, add it explicitly
                    rawProperties.push_back({ nameToSet, valueToSet });
                }

                valueWasSet = true;
            }

            return valueWasSet;
        }

        bool MaterialVersionUpdate::ApplyVersionUpdates(
            MaterialAsset& materialAsset,
            AZStd::function<void(const char*)> reportError) const
        {
            if (!ValidateActions(materialAsset.GetMaterialPropertiesLayout(), reportError))
            {
                return false;
            }

            bool changesWereApplied = false;

            // Handle rename
            for (auto& [name, value] : materialAsset.m_rawPropertyValues)
            {
                changesWereApplied |= ApplyPropertyRenames(name);
            }

            // Handle setValue
            changesWereApplied |= ApplySetValues(materialAsset.m_rawPropertyValues);

            return changesWereApplied;
        }

        const AZ::RPI::MaterialVersionUpdate::Actions& MaterialVersionUpdate::GetActions() const
        {
            return m_actions;
        }

        void MaterialVersionUpdate::AddAction(const Action& action)
        {
            m_actions.push_back(action);
        }

        MaterialVersionUpdate::Action::Action(
                const AZ::Name& operation, const AZStd::initializer_list<AZStd::pair<AZ::Name, MaterialPropertyValue>>& args)
            : m_operation(operation)
        {
            for (const auto& arg : args)
            {
                AddArg(arg.first, arg.second);
            }
        }

        MaterialVersionUpdate::Action::Action(
                const AZStd::map<AZStd::string, MaterialPropertyValue>& fullActionDefinition)
        {
            for (auto& [key, value] : fullActionDefinition)
            {
                if (key == "op")
                {
                    if (value.Is<AZStd::string>())
                    {
                        m_operation = value.GetValue<AZStd::string>();
                    }
                    else
                    {
                        AZ_Error("MaterialVersionUpdate", false, "The operation type under the 'op' key should be a string");
                        return;
                    }
                }
                else
                {
                    AddArg(AZ::Name(key), value);
                }
            }

            // Verify that we got an "op" key for our operation type
            if (m_operation.IsEmpty())
            {
                AZ_Error("MaterialVersionUpdate", false, "The operation type under the 'op' key was missing or empty");
            }
        }

        size_t MaterialVersionUpdate::Action::GetNumArgs() const
        {
            return m_argsMap.size();
        }

        void MaterialVersionUpdate::Action::AddArg(const AZ::Name& key, const MaterialPropertyValue& argument)
        {
            m_argsMap[key] = argument;
        }

        MaterialPropertyValue MaterialVersionUpdate::Action::GetArg(const AZ::Name& key) const
        {
            const auto it = m_argsMap.find(key);
            if (it == m_argsMap.end())
            {
                AZ_Warning("MaterialVersionUpdate", false, "Requested argument that was not in map: %s", key.GetCStr());
                return MaterialPropertyValue();
            }
            return it->second;
        }

        bool MaterialVersionUpdate::Action::operator==(const Action& other) const
        {
            const auto &map1 = m_argsMap;
            const auto &map2 = other.m_argsMap;

            bool different = false;
            different = different || m_operation != other.m_operation;
            different = different || map1.size() != map2.size();
            if (different)
            {
                return false;
            }

            for(const auto& elem : map1)
            {
                different = different || map2.find(elem.first) == map2.end();
                different = different || map2.find(elem.first)->second != elem.second;
                if (different)
                {
                    return false;
                }
            }
            return !different;
        }
    } // namespace RPI
} // namespace AZ
