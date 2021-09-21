/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
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

        MaterialVersionUpdate::Action::Action(const AZStd::string& operation, const AZStd::initializer_list<AZStd::pair<AZStd::string, AZStd::string>>& args)
            : m_operation(operation)
        {
            for (const auto& arg : args)
            {
                AddArgs(arg.first, arg.second);
            }
        }

        void MaterialVersionUpdate::Action::AddArgs(const AZStd::string& key, const AZStd::string& argument)
        {
            m_argsMap[key] = argument;
        }
    } // namespace RPI
} // namespace AZ
