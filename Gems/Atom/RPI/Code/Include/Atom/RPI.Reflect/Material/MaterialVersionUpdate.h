/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        struct MaterialVersionUpdate
        {
            AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate, "{B36E7712-AED8-46AA-AFE0-01F8F884C44A}");

            static void Reflect(ReflectContext* context);

            struct Action
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate::Action, "{A1FBEB19-EA05-40F0-9700-57D048DF572B}");

                AZStd::string m_operation;
                AZStd::map<AZStd::string, AZStd::string> m_argsMap;

                Action() = default;
                Action(const AZStd::string& operation, const AZStd::initializer_list<AZStd::pair<AZStd::string, AZStd::string>>& args);
                void AddArgs(const AZStd::string& key, const AZStd::string& argument);
            };

            MaterialVersionUpdate() = default;
            MaterialVersionUpdate(uint32_t toVersion);

            uint32_t m_toVersion;

            using Actions = AZStd::vector<Action>;
            Actions m_actions;
        };

        using MaterialVersionUpdateMap = AZStd::map<uint32_t, MaterialVersionUpdate>;
    } // namespace RPI
} // namespace AZ
