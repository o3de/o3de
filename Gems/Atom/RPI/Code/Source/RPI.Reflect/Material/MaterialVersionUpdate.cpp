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
        void MaterialVersionUpdate::MaterialPropertyValueWrapper::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialVersionUpdate::MaterialPropertyValueWrapper>()
                    ->Version(0)
                    ->Field("Value", &MaterialPropertyValueWrapper::m_value)
                    ->Field("NameCache", &MaterialPropertyValueWrapper::m_nameCache)
                    ;
            }
        }

        void MaterialVersionUpdate::Action::Reflect(ReflectContext* context)
        {
            MaterialVersionUpdate::MaterialPropertyValueWrapper::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialVersionUpdate::Action::ArgsMap>();

                serializeContext->Class<MaterialVersionUpdate::Action>()
                    ->Version(3) // Generic actions based on string -> MaterialPropertyValueWrapper map
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

        void MaterialVersionUpdates::Reflect(ReflectContext* context)
        {
            MaterialVersionUpdate::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialVersionUpdateList>();

                serializeContext->Class<MaterialVersionUpdates>()
                    ->Version(0)
                    ->Field("VersionUpdates", &MaterialVersionUpdates::m_versionUpdates)
                    ;
            }
        }

        MaterialVersionUpdate::MaterialPropertyValueWrapper::MaterialPropertyValueWrapper(const MaterialPropertyValue& value)
            : m_value(value)
        {
            if (m_value.IsValid() && m_value.Is<AZStd::string>())
            {
                m_nameCache = AZ::Name(m_value.GetValue<AZStd::string>());
            }
        }

        const MaterialPropertyValue& MaterialVersionUpdate::MaterialPropertyValueWrapper::Get() const
        {
            return m_value;
        }

        const AZ::Name& MaterialVersionUpdate::MaterialPropertyValueWrapper::GetAsName() const
        {
            AZ_Error(
                "MaterialVersionUpdate", m_value.IsValid() && m_value.Is<AZStd::string>(),
                "GetAsName() expects a valid string value");
            return m_nameCache;
        }

        bool MaterialVersionUpdate::MaterialPropertyValueWrapper::operator==(const MaterialPropertyValueWrapper& other) const
        {
            return m_value == other.m_value;
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
            bool isValid = expectedNum == GetArgCount();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected %zu arguments in '%s' version update (%s), but found %zu",
                            expectedNum, m_operation.GetCStr(), expectedArgs, GetArgCount())
                            .c_str());
            }
            return isValid;
        }

        template <typename T>
        bool MaterialVersionUpdate::Action::HasExpectedArgument(
            const char* expectedArgName, const char* T_str, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue& val = GetArg(AZ::Name{ expectedArgName });
            bool isValid = val.IsValid() && val.Is<T>();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s' of type %s", expectedArgName, m_operation.GetCStr(), T_str)
                            .c_str());
            }
            return isValid;
        }

        bool MaterialVersionUpdate::Action::HasExpectedArgumentAnyType(
            const char* expectedArgName, AZStd::function<void(const char*)> onError) const
        {
            const MaterialPropertyValue& val = GetArg(AZ::Name{ expectedArgName });
            bool isValid = val.IsValid();
            if (!isValid && onError != nullptr)
            {
                onError(AZStd::string::format(
                            "Expected a '%s' field in '%s'", expectedArgName, m_operation.GetCStr())
                            .c_str());
            }
            return isValid;
        }
        bool MaterialVersionUpdate::ValidateActions(
            const PropertyHelper* propertyHelper, AZStd::function<void(const char*)> onError) const
        {
            for (const auto& action : m_actions)
            {
                bool actionValidation;
                if (propertyHelper == nullptr)
                {
                    actionValidation = action.Validate(onError);
                }
                else
                {
                    actionValidation = action.ValidateFully(*propertyHelper, onError);
                }

                if (!actionValidation)
                {
                    return false;
                }
            }

            return true;
        }

        bool MaterialVersionUpdate::Action::Validate(AZStd::function<void(const char*)> onError) const
        {
            bool error = false;
            if (m_operation == AZ::Name("rename") || m_operation == AZ::Name("renamePrefix"))
            {
                error = error || !HasExpectedNumArguments(2, "'from', 'to'", onError);
                error = error || !HasExpectedArgument<AZStd::string>("from", "string", onError);
                error = error || !HasExpectedArgument<AZStd::string>("to", "string", onError);
            }
            else if (m_operation == AZ::Name("setValue"))
            {
                error = error || !HasExpectedNumArguments(2, "'name', 'value'", onError);
                error = error || !HasExpectedArgument<AZStd::string>("name", "string", onError);
                error = error || !HasExpectedArgumentAnyType("value", onError);
            }
            else if (m_operation.IsEmpty())
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                            "Material version update action was not properly initialized: empty operation").c_str());
                }
                return false;
            }
            else
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                            "Unknown operation '%s' in material version update action",
                            m_operation.GetCStr()).c_str());
                }
                return false;
            }

            return !error;
        }

        bool MaterialVersionUpdate::Action::ValidateFully(
            const PropertyHelper& propertyHelper,
            AZStd::function<void(const char*)> onError) const
        {
            if (!Validate(onError))
            {
                return false;
            }

            if (m_operation == AZ::Name("setValue"))
            {
                // Check property name & value type
                const AZ::Name& nameToSet = GetArgAsName(AZ::Name("name"));
                MaterialPropertyValue valueToSet = GetArg(AZ::Name("value"));
                if (!propertyHelper.CastToExpectedType(nameToSet, valueToSet, onError))
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
                if (action.GetOperation() == AZ::Name("rename"))
                {
                    const AZ::Name& from = action.GetArgAsName(AZ::Name("from"));
                    if (propertyId == from)
                    {
                        propertyId = action.GetArgAsName(AZ::Name("to"));
                        renamed = true;
                    }
                }
                else if (action.GetOperation() == AZ::Name("renamePrefix"))
                {
                    const AZ::Name& from = action.GetArgAsName(AZ::Name("from"));
                    if (propertyId.GetStringView().starts_with(from.GetCStr()))
                    {
                        AZStd::string renamedProperty = propertyId.GetCStr();
                        AzFramework::StringFunc::Replace(renamedProperty, action.GetArgAsName(AZ::Name("from")).GetCStr(), action.GetArgAsName(AZ::Name("to")).GetCStr(), true, true);
                        propertyId = Name{renamedProperty};
                        renamed = true;
                    }
                }

            }

            return renamed;
        }

        MaterialVersionUpdate::PropertyHelper::PropertyHelper(
            AZStd::function<bool(AZ::Name&)> applyAllPropertyRenames,
            const MaterialPropertiesLayout* materialPropertiesLayout)
            : m_applyAllPropertyRenames(applyAllPropertyRenames), m_materialPropertiesLayout(materialPropertiesLayout)
        {
        }

        bool MaterialVersionUpdate::PropertyHelper::CastToExpectedType(
            const Name& providedPropertyId, MaterialPropertyValue& value, AZStd::function<void(const char*)> onError) const
        {
            AZ_Assert(m_materialPropertiesLayout != nullptr, "PropertyHelper is not properly initialized");

            // Update property id to latest name
            Name propertyId(providedPropertyId);
            ApplyAllPropertyRenames(propertyId);

            // Check that the property is known
            const auto propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyId);
            if (propertyIndex.IsNull())
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                                "Could not find property %s in the material properties layout",
                                FriendlyPropertyName(providedPropertyId, propertyId).c_str())
                                .c_str());
                }
                return false;
            }

            // Due to the ambiguity in the json parser (e.g. Color vs Vector[3-4]): try to cast
            // the value into the correct type.
            const MaterialPropertyDescriptor *descriptor = m_materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);
            TypeId expectedType = descriptor->GetAssetDataTypeId();
            value = value.CastToType(expectedType);

            // Check if that cast was successful
            if (value.GetTypeId() != expectedType)
            {
                if (onError != nullptr)
                {
                    onError(AZStd::string::format(
                                "Unexpected type for property %s: expected %s but received %s",
                                FriendlyPropertyName(providedPropertyId, propertyId).c_str(),
                                expectedType.ToString<AZStd::string>().c_str(),
                                value.GetTypeId().ToString<AZStd::string>().c_str())
                                .c_str());
                }
                return false;
            }

            return true;
        }

        bool MaterialVersionUpdate::PropertyHelper::ApplyAllPropertyRenames(AZ::Name& propertyId) const
        {
            return m_applyAllPropertyRenames(propertyId);
        }

        AZStd::string MaterialVersionUpdate::PropertyHelper::FriendlyPropertyName(
            const AZ::Name& propertyId, const AZ::Name& finalPropertyId) const
        {
            if (propertyId == finalPropertyId)
            {
                return AZStd::string::format("'%s'", propertyId.GetCStr());
            }
            else
            {
                return AZStd::string::format(
                    "'%s' (final name of this property: '%s')", propertyId.GetCStr(), finalPropertyId.GetCStr());
            }
        }

        bool MaterialVersionUpdate::ApplySetValues(
            AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& rawProperties,
            const PropertyHelper& propertyHelper,
            AZStd::function<void(const char*)> onError) const
        {
            bool valueWasSet = false;
            for (const auto& action : m_actions)
            {
                if (action.GetOperation() != AZ::Name("setValue"))
                {
                    continue;
                }

                const AZ::Name& nameFromSetValueAction = action.GetArgAsName(AZ::Name("name"));

                // Update the name in case our setValue action is still using an older name
                AZ::Name nameToSet(nameFromSetValueAction);
                propertyHelper.ApplyAllPropertyRenames(nameToSet);

                MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                // Due to the ambiguity in the json parser (e.g. Color vs Vector[3-4]): try to cast
                // the value into the correct type. This also checks that the property is actually
                // known.
                if (!propertyHelper.CastToExpectedType(nameToSet, valueToSet, onError))
                {
                    return false;
                }

                // Check if property already exists, in which case we overwrite its value (and warn the user)
                bool propertyFound = false;
                for (auto& [name, value] : rawProperties)
                {
                    if (name == nameToSet)
                    {
                        value = valueToSet;
                        AZ_Warning(
                            "MaterialVersionUpdate", false,
                            "SetValue operation of update to version %u has detected (and overwritten) a previous value for %s.",
                            GetVersion(), propertyHelper.FriendlyPropertyName(nameFromSetValueAction, nameToSet).c_str());

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

        MaterialVersionUpdate::PropertyHelper MaterialVersionUpdates::MakePropertyHelper(
            const MaterialPropertiesLayout* materialPropertiesLayout) const
        {
            return MaterialVersionUpdate::PropertyHelper(
                [this](AZ::Name& propertyId){ return ApplyPropertyRenames(propertyId); },
                materialPropertiesLayout);
        }

        bool MaterialVersionUpdates::ApplyVersionUpdates(
            MaterialAsset& materialAsset,
            AZStd::function<void(const char*)> reportError) const
        {
            // Validate all actions before we begin
            if (!ValidateUpdates(
                materialAsset.GetMaterialTypeAsset()->GetVersion(),
                materialAsset.GetMaterialPropertiesLayout(), reportError))
            {
                return false;
            }

            bool changesWereApplied = false;

            // Apply all renames first, so that the properties names are up
            // to date for the other updates actions (e.g. setValue).
            for (auto versionUpdate : m_versionUpdates)
            {
                // Handle rename
                // Note: we can perform rename updates 'blindly' (i.e. even if m_materialTypeAsset ==
                // UnspecifiedMaterialTypeVersion) without potential conflicts: we determine which 
                // updates to apply by simply checking the property name, and not allowing the
                // same name to ever be used for two different properties (see ValidateUpdates()).
                for (auto& [name, value] : materialAsset.m_rawPropertyValues)
                {
                    changesWereApplied |= versionUpdate.ApplyPropertyRenames(name);
                }
            }

            // We can handle setValue actions *only* if the material type version of the material asset is known!
            if (materialAsset.m_materialTypeVersion != MaterialAsset::UnspecifiedMaterialTypeVersion)
            {
                MaterialVersionUpdate::PropertyHelper propertyHelper = MakePropertyHelper(
                    materialAsset.GetMaterialPropertiesLayout());
                for (auto versionUpdate : m_versionUpdates)
                {
                    if (materialAsset.m_materialTypeVersion >= versionUpdate.GetVersion())
                    {
                        continue; // These updates are outdated and thus not needed
                    }

                    changesWereApplied |= versionUpdate.ApplySetValues(
                        materialAsset.m_rawPropertyValues, propertyHelper, reportError);
                }
            }

            // Update the material asset's associated materialTypeVersion
            if (!m_versionUpdates.empty())
            {
                materialAsset.m_materialTypeVersion = m_versionUpdates.back().GetVersion();
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
            if (action.Validate() && sourceDataResolver != nullptr)
            {
                if (action.GetOperation() == AZ::Name("setValue"))
                {
                    const AZ::Name& nameToSet = action.GetArgAsName(AZ::Name("name"));
                    MaterialPropertyValue valueToSet = action.GetArg(AZ::Name("value"));
                    // Resolve the value and overwrite it in resolvedAction:
                    resolvedAction.AddArg(AZ::Name("value"), sourceDataResolver(nameToSet, valueToSet));

                    AZ_Assert(resolvedAction.Validate(), "Resolving value led to invalid action");
                }
            }

            m_actions.push_back(resolvedAction);
        }

        bool MaterialVersionUpdates::ApplyPropertyRenames(AZ::Name& propertyId) const
        {
            bool renamed = false;
            for (auto versionUpdate : m_versionUpdates)
            {
                renamed |= versionUpdate.ApplyPropertyRenames(propertyId);
            }
            return renamed;
        }

        MaterialVersionUpdate::Action::Action(const ActionDefinition& fullActionDefinition)
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

        MaterialVersionUpdate::Action::Action(
            const AZStd::initializer_list<AZStd::pair<AZStd::string, MaterialPropertyValue>>& fullActionDefinition)
            : MaterialVersionUpdate::Action::Action(ActionDefinition(fullActionDefinition))
        {
        }

        MaterialVersionUpdate::Action::Action(
            const AZ::Name& operation,
            const AZStd::initializer_list<AZStd::pair<AZ::Name, MaterialPropertyValue>>& args)
            : m_operation(operation)
        {
            for (const auto& arg : args)
            {
                AddArg(arg.first, arg.second);
            }
        }

        size_t MaterialVersionUpdate::Action::GetArgCount() const
        {
            return m_argsMap.size();
        }

        void MaterialVersionUpdate::Action::AddArg(const AZ::Name& key, const MaterialPropertyValue& argument)
        {
            m_argsMap[key] = MaterialPropertyValueWrapper(argument);
        }

        const MaterialPropertyValue& MaterialVersionUpdate::Action::GetArg(const AZ::Name& key) const
        {
            const auto it = m_argsMap.find(key);
            if (it == m_argsMap.end())
            {
                return s_invalidValue;
            }
            return it->second.Get();
        }

        const AZ::Name& MaterialVersionUpdate::Action::GetArgAsName(const AZ::Name& key) const
        {
            const auto it = m_argsMap.find(key);
            if (it == m_argsMap.end())
            {
                return MaterialPropertyValueWrapper::s_invalidName;
            }
            return it->second.GetAsName();
        }

        const AZ::Name& MaterialVersionUpdate::Action::GetOperation() const
        {
            return m_operation;
        }

        bool MaterialVersionUpdate::Action::operator==(const Action& other) const
        {
            return m_argsMap == other.m_argsMap;
        }

        bool MaterialVersionUpdates::ValidateUpdates(
            uint32_t materialTypeVersion, const MaterialPropertiesLayout* materialPropertiesLayout,
            AZStd::function<void(const char*)> onError) const
        {
            if (m_versionUpdates.empty())
            {
                return true;
            }

            uint32_t prevVersion = 0;

            // Do an initial 'light' validation pass without a propertyHelper
            // to check basic consistency (e.g. check rename actions).
            for (const MaterialVersionUpdate& versionUpdate : m_versionUpdates)
            {
                if (!versionUpdate.ValidateActions(nullptr, onError))
                {
                    return false;
                }
            }

            // We succeeded in our 'light' validation, make a PropertyHelper that
            // points back to us for the 'full' validation.
            MaterialVersionUpdate::PropertyHelper propertyHelper = MakePropertyHelper(materialPropertiesLayout);

            AZStd::unordered_set<AZ::Name> renamedPropertyNewNames; // Collect final names of any renamed properties
            for (const MaterialVersionUpdate& versionUpdate : m_versionUpdates)
            {
                // Validate internal consistency, 'full' version with propertyHelper
                if (!versionUpdate.ValidateActions(&propertyHelper, onError))
                {
                    return false;
                }

                if (versionUpdate.GetVersion() <= prevVersion)
                {
                    onError(AZStd::string::format(
                        "Version updates are not sequential. See version update '%u'.",
                        versionUpdate.GetVersion()).c_str());
                    return false;
                }

                if (versionUpdate.GetVersion() > materialTypeVersion)
                {
                    onError(AZStd::string::format(
                        "Version updates go beyond the current material type version. See version update '%u'.",
                        versionUpdate.GetVersion()).c_str());
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
                            "There was a material property named '%s' at material type version %u. "
                            "This name cannot be reused for another property.",
                            originalPropertyName.GetCStr(), versionUpdate.GetVersion())
                            .c_str());
                        return false;
                    }
                }

                // Collect any rename 'endpoints'
                for (const auto& action : versionUpdate.GetActions())
                {
                    if (action.GetOperation() != AZ::Name("rename"))
                    {
                        continue;
                    }
                    // if we come from a name that was renamed previously: remove that previous new name
                    const Name& from = action.GetArgAsName(AZ::Name{ "from" });
                    renamedPropertyNewNames.erase(from);

                    // and keep track of the new name
                    const Name& to = action.GetArgAsName(AZ::Name{ "to" });
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
                        propertyName.GetCStr()).c_str());
                    return false;
                }
            }
            return true;
        }

        void MaterialVersionUpdates::AddVersionUpdate(
            const MaterialVersionUpdate& versionUpdate)
        {
            m_versionUpdates.push_back(versionUpdate);
        }

        size_t MaterialVersionUpdates::GetVersionUpdateCount() const
        {
            return m_versionUpdates.size();
        }

        const MaterialVersionUpdate& MaterialVersionUpdates::GetVersionUpdate(size_t i) const
        {
            return m_versionUpdates[i];
        }
    } // namespace RPI
} // namespace AZ
