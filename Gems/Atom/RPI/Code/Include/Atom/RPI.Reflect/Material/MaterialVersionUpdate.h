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

                using ActionDefinition = AZStd::map<AZStd::string, MaterialPropertyValue>;

                Action() = default;
                Action(const AZ::Name& operation, const AZStd::initializer_list<AZStd::pair<AZ::Name, MaterialPropertyValue>>& args);
                //! The operation type is given as a string under the "op" key, the remaining items define the operation's arguments.
                Action(const ActionDefinition& fullActionDefinition);
                Action(const AZStd::initializer_list<AZStd::pair<AZStd::string, MaterialPropertyValue>>& fullActionDefinition);

                void AddArg(const AZ::Name& key, const MaterialPropertyValue& argument);
                MaterialPropertyValue GetArg(const AZ::Name& key, bool verbose = true) const;
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
            
            //! Possibly renames @propertyId based on the material version update actions.
            //! @return true if the property was renamed
            bool ApplyPropertyRenames(AZ::Name& propertyId) const;

            //! Possibly changes or adds values in @rawProperties based on the material version update actions.
            //! @return true if a property was set
            bool ApplySetValues(
                AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& rawProperties,
                const MaterialPropertiesLayout* materialPropertiesLayout,
                AZStd::function<void(const char*)> onError) const;

            //! Apply version updates to the given material asset.
            //! @return true if any changes were made
            bool ApplyVersionUpdates(MaterialAsset& materialAsset, AZStd::function<void(const char*)> reportError) const;

            //! Validates the internal consistency of the given action
            static bool ValidateAction(
                const Action& action,
                const MaterialPropertiesLayout* materialPropertiesLayout = NULL,
                AZStd::function<void(const char*)> onError = nullptr);
            //! Validates the internal consistency of our update actions
            bool ValidateActions(
                const MaterialPropertiesLayout* materialPropertiesLayout, AZStd::function<void(const char*)> onError) const;

            using Actions = AZStd::vector<Action>;
            const Actions& GetActions() const;
            //! Add the given action. Optionally, @sourceDataResolver may be given to resolve a
            //! MaterialPropertyValue from a source type into an asset-ready type (e.g. from an
            //! image filename string to an ImageAsset).
            void AddAction(
                const Action& action,
                AZStd::function<MaterialPropertyValue(const Name&, const MaterialPropertyValue&)> sourceDataResolver = nullptr);

            //! Validate a series of material updates against a MaterialTypeAsset with version
            //! @materialTypeVersion and properties layout @materialPropertiesLayout.
            static bool ValidateUpdates(
                const AZStd::vector<MaterialVersionUpdate>& versionUpdates,
                uint32_t materialTypeVersion,
                const MaterialPropertiesLayout* materialPropertiesLayout,
                AZStd::function<void(const char*)> onError);

        private:
            static MaterialPropertyValue CastToExpectedValue(
                const Name& propertyId,
                const MaterialPropertyValue& value,
                const MaterialPropertiesLayout* materialPropertiesLayout,
                AZStd::function<void(const char*)> onError = nullptr);

            uint32_t m_toVersion;
            Actions m_actions;
        };

    } // namespace RPI
} // namespace AZ
