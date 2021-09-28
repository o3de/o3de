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
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;

        // This class contains a toVersion and a list of actions to specify what operations were performed to upgrade a materialType. 
        class MaterialVersionUpdate
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate, "{B36E7712-AED8-46AA-AFE0-01F8F884C44A}");

            static void Reflect(ReflectContext* context);

            struct Action
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate::Action, "{A1FBEB19-EA05-40F0-9700-57D048DF572B}");

                AZ::Name m_operation;
                AZStd::unordered_map<AZ::Name, AZ::Name> m_argsMap;

                Action() = default;
                Action(const AZ::Name& operation, const AZStd::initializer_list<AZStd::pair<AZ::Name, AZ::Name>>& args);
                void AddArg(const AZ::Name& key, const AZ::Name& argument);
            };

            explicit MaterialVersionUpdate() = default;
            explicit MaterialVersionUpdate(uint32_t toVersion);

            uint32_t GetVersion() const;
            void SetVersion(uint32_t toVersion);

            void ApplyVersionUpdates(MaterialAsset& materialAsset) const;

            using Actions = AZStd::vector<Action>;
            const Actions& GetActions() const;
            void AddAction(const Action& action);

        private:
            uint32_t m_toVersion;
            Actions m_actions;
        };

        using MaterialVersionUpdateMap = AZStd::map<uint32_t, MaterialVersionUpdate>;
    } // namespace RPI
} // namespace AZ
