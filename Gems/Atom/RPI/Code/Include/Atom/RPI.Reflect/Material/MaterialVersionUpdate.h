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
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

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
                static void Reflect(ReflectContext* context);

                using ArgsMap = AZStd::unordered_map<AZ::Name, MaterialPropertyValue>;
                AZ::Name m_operation;
                ArgsMap m_argsMap;

                Action() = default;
                Action(const AZ::Name& operation, const AZStd::initializer_list<AZStd::pair<AZ::Name, MaterialPropertyValue>>& args);
                //! The operation type is given as a string under the "op" key, the remaining items define the operation's arguments.
                Action(const AZStd::map<AZStd::string, MaterialPropertyValue>& fullActionDefinition);

                void AddArg(const AZ::Name& key, const MaterialPropertyValue& argument);
                MaterialPropertyValue GetArg(const AZ::Name& key) const;
                size_t GetNumArgs() const;

                bool HasExpectedNumArguments(
                    size_t expectedNum, const char* expectedArgs, AZStd::function<void(const char*)> onError) const;

                template <typename T>
                bool HasExpectedArgument(
                    const char* expectedArgName, const char* T_str, AZStd::function<void(const char*)> onError) const;

                bool HasExpectedArgumentAnyType(
                    const char* expectedArgName, AZStd::function<void(const char*)> onError) const;

                bool operator==(const Action& other) const;
            };

            explicit MaterialVersionUpdate() = default;
            explicit MaterialVersionUpdate(uint32_t toVersion);

            uint32_t GetVersion() const;
            void SetVersion(uint32_t toVersion);
            
            //! Possibly renames @propertyId based on the material version update steps.
            //! @return true if the property was renamed
            bool ApplyPropertyRenames(AZ::Name& propertyId) const;

            //! Possibly changes or adds values in @rawProperties based on the material version update steps.
            //! @return true if a property was set
            bool ApplySetValues(AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& rawProperties) const;

            //! Apply version updates to the given material asset.
            //! @return true if any changes were made
            bool ApplyVersionUpdates(
                MaterialAsset& materialAsset,
                AZStd::function<void(const char*)> reportError) const;

            bool ValidateActions(
                const MaterialPropertiesLayout* materialPropertiesLayout, AZStd::function<void(const char*)> onError) const;

            using Actions = AZStd::vector<Action>;
            const Actions& GetActions() const;
            void AddAction(const Action& action);

        private:
            uint32_t m_toVersion;
            Actions m_actions;
        };

    } // namespace RPI
} // namespace AZ
