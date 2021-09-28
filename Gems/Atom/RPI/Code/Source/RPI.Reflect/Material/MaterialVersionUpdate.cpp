/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialVersionUpdate::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialVersionUpdate::Action>()
                    ->Version(1)
                    ->Field("ArgsMap", &Action::m_argsMap)
                    ->Field("Operation", &Action::m_operation)
                    ;

                serializeContext->RegisterGenericType<MaterialVersionUpdate::Actions>();

                serializeContext->Class<MaterialVersionUpdate>()
                    ->Version(1)
                    ->Field("ToVersion", &MaterialVersionUpdate::m_toVersion)
                    ->Field("Actions", &MaterialVersionUpdate::m_actions)
                    ;

                serializeContext->RegisterGenericType<MaterialVersionUpdateMap>();
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

        void MaterialVersionUpdate::ApplyVersionUpdates(MaterialAsset& materialAsset) const
        {
            // collect all renames within this version update
            AZStd::unordered_map<AZ::Name, AZ::Name> renameToFrom;
            for (const auto& action : m_actions)
            {
                const AZ::Name from = action.m_argsMap.find(AZ::Name{ "from" })->second;
                const AZ::Name to = action.m_argsMap.find(AZ::Name{ "to" })->second;

                renameToFrom[from] = to;
            }

            // apply rename actions
            for (auto& propertyName : materialAsset.m_propertyNames)
            {
                const auto toFromIterator = renameToFrom.find(propertyName);
                if (toFromIterator != renameToFrom.end())
                {
                    propertyName = AZ::Name{ toFromIterator->second };
                }
            }
        }

        const AZ::RPI::MaterialVersionUpdate::Actions& MaterialVersionUpdate::GetActions() const
        {
            return m_actions;
        }

        void MaterialVersionUpdate::AddAction(const Action& action)
        {
            m_actions.push_back(action);
        }

        MaterialVersionUpdate::Action::Action(const AZ::Name& operation, const AZStd::initializer_list<AZStd::pair<AZ::Name, AZ::Name>>& args)
            : m_operation(operation)
        {
            for (const auto& arg : args)
            {
                AddArg(arg.first, arg.second);
            }
        }

        void MaterialVersionUpdate::Action::AddArg(const AZ::Name& key, const AZ::Name& argument)
        {
            m_argsMap[key] = argument;
        }
    } // namespace RPI
} // namespace AZ
