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
            bool isValid = expectedNum == GetNumArgs();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected %zu arguments in '%s' version update (%s), but found %zu",
                            expectedNum, m_operation.GetCStr(), expectedArgs, GetNumArgs())
                            .data());
            }
            return isValid;
        }

        template <typename T>
        bool MaterialVersionUpdate::Action::HasExpectedArgument(
            const char* expectedArgName, const char* T_str, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue val = GetArg(AZ::Name{ expectedArgName }, false);
            bool isValid = val.IsValid() && val.Is<T>();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s' of type %s", expectedArgName, m_operation.GetCStr(), T_str)
                            .data());
            }
            return isValid;
        }

        bool MaterialVersionUpdate::Action::HasExpectedArgumentAnyType(
            const char* expectedArgName, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue val = GetArg(AZ::Name{ expectedArgName }, false);
            bool isValid = val.IsValid();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s'", expectedArgName, m_operation.GetCStr())
                            .data());
            }
            return isValid;
        }
        bool MaterialVersionUpdate::ValidateActions(
            const MaterialPropertiesLayout* materialPropertiesLayout, AZStd::function<void(const char*)> onError) const
        {
            for (const auto& action : m_actions)
            {
                if (!ValidateAction(action, materialPropertiesLayout, onError))
                {
                    return false;
                }
            }

            return true;
        }

        bool MaterialVersionUpdate::ValidateAction(
            const Action& action,
            const MaterialPropertiesLayout* materialPropertiesLayout,
            AZStd::function<void(const char*)> onError)
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

                // If we know the materialPropertiesLayout, we can check the property name & value type
                if (materialPropertiesLayout && onError != nullptr)
                {
                    const AZ::Name nameToSet = AZ::Name(action.GetArg(AZ::Name("name")).GetValue<AZStd::string>());
                    MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                    CastToExpectedValue(nameToSet, valueToSet, materialPropertiesLayout, onError);
                }
            }
            else if (action.m_operation.IsEmpty())
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                            "Material version update action was not properly initialized: empty operation").data());
                }
                error = true;
            }
            else
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                            "Unknown operation '%s' in material version update action",
                            action.m_operation.GetCStr()).data());
                }
                error = true;
            }

            return !error;
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

                const AZ::Name from = AZ::Name(action.GetArg(AZ::Name("from")).GetValue<AZStd::string>());
                if (propertyId == from)
                {
                    propertyId = AZ::Name(action.GetArg(AZ::Name("to")).GetValue<AZStd::string>());
                    renamed = true;
                }
            }

            return renamed;
        }

        MaterialPropertyValue MaterialVersionUpdate::CastToExpectedValue(
            const Name& propertyId, const MaterialPropertyValue& value, const MaterialPropertiesLayout* materialPropertiesLayout,
            AZStd::function<void(const char*)> onError)
        {
            // Check that the property is known
            const auto propertyIndex = materialPropertiesLayout->FindPropertyIndex(propertyId);
            if (propertyIndex.IsNull())
            {
                onError(AZStd::string::format(
                            "setValue material version update: Could not find property '%s' in the material properties layout",
                            propertyId.GetCStr())
                            .data());
                return value;
            }

            // Due to the ambiguity in the json parser (e.g. Color vs Vector[3-4]): try to cast
            // the value into the correct type.
            const MaterialPropertyDescriptor *descriptor = materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);
            TypeId expectedType = descriptor->GetAssetDataTypeId();
            MaterialPropertyValue castValue = value.CastToType(expectedType);

            // Check if that cast was successful
            if (castValue.GetTypeId() != expectedType)
            {
                onError(AZStd::string::format(
                            "Unexpected type for property %s in a setValue version update: expected %s but received %s",
                            propertyId.GetCStr(), expectedType.ToString<AZStd::string>().data(),
                            value.GetTypeId().ToString<AZStd::string>().data())
                            .data());
            }
            return castValue;
        }

        bool MaterialVersionUpdate::ApplySetValues(
            AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& rawProperties,
            const MaterialPropertiesLayout* materialPropertiesLayout,
            AZStd::function<void(const char*)> onError) const
        {
            bool valueWasSet = false;
            for (const auto& action : m_actions)
            {
                if (action.m_operation != AZ::Name("setValue"))
                {
                    continue;
                }

                const AZ::Name nameToSet = AZ::Name(action.GetArg(AZ::Name("name")).GetValue<AZStd::string>());
                MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                // Due to the ambiguity in the json parser (e.g. Color vs Vector[3-4]): try to cast
                // the value into the correct type. This also checks that the property is actually
                // known.
                valueToSet = CastToExpectedValue(nameToSet, valueToSet, materialPropertiesLayout, onError);

                // Check if property already exists, in which case we overwrite its value (and warn the user)
                bool propertyFound = false;
                for (auto& [name, value] : rawProperties)
                {
                    if (name == nameToSet)
                    {
                        value = valueToSet;
                        AZ_Warning(
                            "MaterialVersionUpdate", false,
                            "SetValue version update has detected (and overwritten) a previous value for property %s.", name.GetCStr());

                        AZ_Warning("MaterialVersionUpdate", !propertyFound, "Found property %s more than once!", name.GetCStr());
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
            MaterialAsset& materialAsset, AZStd::function<void(const char*)> reportError) const
        {
            if (!ValidateActions(materialAsset.GetMaterialPropertiesLayout(), reportError))
            {
                return false;
            }

            bool versionIsKnown = materialAsset.m_materialTypeVersion != MaterialAsset::UnspecifiedMaterialTypeVersion;

            if (versionIsKnown && materialAsset.m_materialTypeVersion >= GetVersion())
            {
                return false; // Our updates are not needed
            }

            bool changesWereApplied = false;

            // Handle rename
            // Note: we can perform rename updates 'blindly' (i.e. even if m_materialTypeAsset ==
            // UnspecifiedMaterialTypeVersion) without potential conflicts: we determine which updates to apply by
            // simply checking the property name, and not allowing the same name to ever be used for two different
            // properties (see ValidateUpdates()).
            for (auto& [name, value] : materialAsset.m_rawPropertyValues)
            {
                changesWereApplied |= ApplyPropertyRenames(name);
            }

            // We can handle setValue actions *only* if the material type version of the material asset is known!
            if (versionIsKnown)
            {
                changesWereApplied |= ApplySetValues(
                    materialAsset.m_rawPropertyValues, materialAsset.GetMaterialPropertiesLayout(), reportError);
            }

            return changesWereApplied;
        }

        const AZ::RPI::MaterialVersionUpdate::Actions& MaterialVersionUpdate::GetActions() const
        {
            return m_actions;
        }

        void MaterialVersionUpdate::AddAction(
            const Action& action,
            AZStd::function<MaterialPropertyValue(const Name&, const MaterialPropertyValue&)> sourceDataResolver)
        {
            Action resolvedAction(action);
            if (ValidateAction(action) && sourceDataResolver != nullptr)
            {
                if (action.m_operation == AZ::Name("setValue"))
                {
                    const AZ::Name nameToSet = AZ::Name(action.GetArg(AZ::Name("name")).GetValue<AZStd::string>());
                    MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                    // Resolve the value and overwrite it in resolvedAction:
                    resolvedAction.AddArg(AZ::Name("value"), sourceDataResolver(nameToSet, valueToSet));

                    AZ_Assert(ValidateAction(resolvedAction), "Resolving value led to invalid action");
                }
            }

            m_actions.push_back(resolvedAction);
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

        MaterialVersionUpdate::Action::Action(const AZStd::initializer_list<AZStd::pair<AZStd::string, MaterialPropertyValue>>& fullActionDefinition)
            : MaterialVersionUpdate::Action::Action(ActionDefinition(fullActionDefinition))
        {
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

        size_t MaterialVersionUpdate::Action::GetNumArgs() const
        {
            return m_argsMap.size();
        }

        void MaterialVersionUpdate::Action::AddArg(const AZ::Name& key, const MaterialPropertyValue& argument)
        {
            m_argsMap[key] = argument;
        }

        MaterialPropertyValue MaterialVersionUpdate::Action::GetArg(const AZ::Name& key, bool verbose) const
        {
            const auto it = m_argsMap.find(key);
            if (it == m_argsMap.end())
            {
                if (verbose)
                {
                    AZ_Warning("MaterialVersionUpdate", false, "Requested argument that was not in map: %s", key.GetCStr());
                }
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

        bool MaterialVersionUpdate::ValidateUpdates(
            const AZStd::vector<MaterialVersionUpdate>& versionUpdates, uint32_t materialTypeVersion,
            const MaterialPropertiesLayout* materialPropertiesLayout, AZStd::function<void(const char*)> onError)
        {
            if (versionUpdates.empty())
            {
                return true;
            }

            uint32_t prevVersion = 0;

            AZStd::unordered_set<AZ::Name> renamedPropertyNewNames; // Collect final names of any renamed properties
            for (const MaterialVersionUpdate& versionUpdate : versionUpdates)
            {
                // Validate internal consistency
                if (!versionUpdate.ValidateActions(materialPropertiesLayout, onError))
                {
                    return false;
                }

                if (versionUpdate.GetVersion() <= prevVersion)
                {
                    onError(AZStd::string::format(
                        "Version updates are not sequential. See version update '%u'.",
                        versionUpdate.GetVersion()).data());
                    return false;
                }

                if (versionUpdate.GetVersion() > materialTypeVersion)
                {
                    onError(AZStd::string::format(
                        "Version updates go beyond the current material type version. See version update '%u'.",
                        versionUpdate.GetVersion()).data());
                    return false;
                }

                // We don't allow previously renamed property names to be reused for new properties.
                // This would just complicate too many things, as every use of every property name
                // (like in Material Component, or in scripts, for example) would have to have a
                // version number associated with it, in order to know whether or which rename to apply.
                for (size_t propertyIndex = 0; propertyIndex < materialPropertiesLayout->GetPropertyCount(); ++propertyIndex)
                {
                    MaterialPropertyIndex idx(propertyIndex);
                    Name originalPropertyName = materialPropertiesLayout->GetPropertyDescriptor(idx)->GetName();
                    Name newPropertyName = originalPropertyName;
                    if (versionUpdate.ApplyPropertyRenames(newPropertyName))
                    {
                        onError(AZStd::string::format(
                            "There was a material property named '%s' at material type version %d. "
                            "This name cannot be reused for another property.",
                            originalPropertyName.GetCStr(), versionUpdate.GetVersion())
                            .data());
                        return false;
                    }
                }

                // Collect any rename 'endpoints'
                for (const auto& action : versionUpdate.GetActions())
                {
                    if (action.m_operation != AZ::Name("rename"))
                    {
                        continue;
                    }
                    // if we come from a name that was renamed previously: remove that previous new name
                    Name from = Name(action.GetArg(AZ::Name{ "from" }).GetValue<AZStd::string>());
                    renamedPropertyNewNames.erase(from);

                    // and keep track of the new name
                    Name to = Name(action.GetArg(AZ::Name{ "to" }).GetValue<AZStd::string>());
                    renamedPropertyNewNames.emplace(to);
                }

                prevVersion = versionUpdate.GetVersion();
            }

            // Verify that we indeed have all new names.
            for (const auto& propertyName : renamedPropertyNewNames)
            {
                const auto propertyIndex = materialPropertiesLayout->FindPropertyIndex(AZ::Name{ propertyName });
                if (!propertyIndex.IsValid())
                {
                    onError(AZStd::string::format(
                        "Renamed property '%s' not found in material property layout. "
                        "Check that the property name has been upgraded to the correct version",
                        propertyName.GetCStr()).data());
                    return false;
                }
            }
            return true;
        }
    } // namespace RPI
} // namespace AZ
